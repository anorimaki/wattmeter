#ifndef SAMPLED_METER_H
#define SAMPLED_METER_H

#include "meter/voltage.h"
#include "meter/current.h"
#include "meter/sampler.h"

namespace meter {

class SampleBasedMeter {
public:
    static const size_t MeasuresSize = adc::GroupedSamplesSize;

    class Measure {
    public:
        Measure(): m_voltage(0), m_current(0) {}
        Measure( float voltage, float current ): m_voltage(voltage), m_current(current) {}

        int16_t voltage() const {
            return m_voltage;
        }

        int16_t current() const {
            return m_current;
        }

    private:
        int16_t m_voltage;
        int16_t m_current;
    };

    typedef std::array<Measure, MeasuresSize> Measures; 

private:
    typedef meter::Sampler<VoltageMeter::AdcChannel, 
						    CurrentMeter::AdcChannel> Sampler;

public:
    void init( uint16_t defaultZero ) {
        m_voltageMeasurer.init(defaultZero);
        m_currentMeasurer.init(defaultZero);
    }

    VoltageMeter& voltageMeter() {
        return m_voltageMeasurer;
    }

    CurrentMeter& currentMeter() {
        return m_currentMeasurer;
    }

    std::pair<float, float> scaleFactors() {
        return std::make_pair( m_voltageMeasurer.scaleFactor(), m_currentMeasurer.scaleFactor() );
    }

    void start() {
        m_sampler.start();
    }

    void stop() {
        m_sampler.stop();
    }

    uint64_t read( Measures& result ) {
        Sampler::Samples samples;
        uint64_t time = m_sampler.read( samples );
        process( samples, result );
        return time;
    }

    void calibrateZeros() {
        m_sampler.pauseWhileAction( [&]() {
            characterizeAdc( DAC_CHANNEL_1, ADC1_CHANNEL_4, ADC1_CHANNEL_6, ADC1_CHANNEL_7 );
            m_voltageMeasurer.calibrateZeros();
            m_currentMeasurer.calibrateZeros();
        });
    }

    void calibrateFactors() {
        m_sampler.pauseWhileAction( [&]() {
		    m_voltageMeasurer.calibrateFactors();
        });
    }

    bool autoRange() {
        VoltageMeter::AutoRangeAction voltageAutoRangeAction = m_voltageMeasurer.autoRangeAction();
        CurrentMeter::AutoRangeAction currentAutoRangeAction = m_currentMeasurer.autoRangeAction();

        if ( !voltageAutoRangeAction && !currentAutoRangeAction ) {
            return false;
        }
        m_sampler.pauseWhileAction( [&]() {
            if ( voltageAutoRangeAction ) {
                voltageAutoRangeAction();
            }
            if ( currentAutoRangeAction ) {
                currentAutoRangeAction();
            }
        } ); 
        return true;
    }

private:
    void process( const Sampler::Samples& samples, Measures& result ) {
        typedef Sampler::Samples::value_type InputSample;
        std::transform( samples.begin(), samples.end(), result.begin(), 
            [&](const InputSample& inputSample) {
                int16_t voltage = m_voltageMeasurer.process( inputSample );
                int16_t current = m_currentMeasurer.process( inputSample );
                return Measure( voltage, current );
            });
    }

private:
    Sampler m_sampler;
    VoltageMeter m_voltageMeasurer;
    CurrentMeter m_currentMeasurer;
};

}

#endif