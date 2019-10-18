#ifndef SAMPLED_METER_H
#define SAMPLED_METER_H

#include "meter/voltage.h"
#include "meter/current.h"
#include "meter/sampler.h"

namespace meter {

class SampleBasedMeter {
public:
    static const size_t MeasuresSize = _::MaxSamples;

    class Measure {
    public:
        Measure(): m_voltage(NAN), m_current(NAN) {}
        Measure( float voltage, float current ): m_voltage(voltage), m_current(current) {}

        float voltage() const {
            return m_voltage;
        }

        float current() const {
            return m_current;
        }

    private:
        float m_voltage;
        float m_current;
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

    void start() {
        m_sampler.start();
    }

    void read( Measures& result ) {
        Sampler::Samples samples;
        m_sampler.read( samples );
        process( samples, result );
        autoRange();
    }

    void calibrateZeros() {
        m_sampler.pauseWhileAction( [&]() {
            m_voltageMeasurer.calibrateZeros();
            m_currentMeasurer.calibrateZeros();
        });
    }

    void calibrateFactors() {
        m_sampler.pauseWhileAction( [&]() {
		    m_voltageMeasurer.calibrateFactors();
        });
    }

private:
    void autoRange() {
        VoltageMeter::AutoRangeAction voltageAutoRangeAction = m_voltageMeasurer.autoRangeAction();
        CurrentMeter::AutoRangeAction currentAutoRangeAction = m_currentMeasurer.autoRangeAction();
        if ( voltageAutoRangeAction || currentAutoRangeAction ) {
            m_sampler.pauseWhileAction( [&]() {
                if ( voltageAutoRangeAction ) {
                    voltageAutoRangeAction();
                }
                if ( currentAutoRangeAction ) {
                    currentAutoRangeAction();
                }
            } ); 
        }
    }

    void process( const Sampler::Samples& samples, Measures& result ) {
        typedef Sampler::Samples::value_type InputSample;
        std::transform( samples.begin(), samples.end(), result.begin(), 
            [&](const InputSample& inputSample) {
                float voltage = m_voltageMeasurer.process( inputSample );
                float current = m_currentMeasurer.process( inputSample );
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