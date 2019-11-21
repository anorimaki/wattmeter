#ifndef CALCULATOR_METER_H
#define CALCULATOR_METER_H

#include "meter/sampledmeter.h"
#include "esp_timer.h"
#include <stdint.h>
#include <numeric>
#include <limits>

namespace meter {

class VariableProcessor {
public:
    VariableProcessor() {
        reset();
    }

    void reset() {
        m_min = std::numeric_limits<int16_t>::max();
        m_max = std::numeric_limits<int16_t>::min();
        m_sum = 0;
        m_squaredSum = 0;
    }

    int64_t sum() const {
        return m_sum;
    }

    int64_t squaredSum() const {
        return m_squaredSum;
    }

    int16_t min() const {
        return m_min;
    }

    int16_t max() const {
        return m_max;
    }

    void process( int16_t value ) {
        if ( value > m_max ) {
            m_max = value;
        }

        if ( value < m_min ) {
            m_min = value;
        }

        m_sum += value;
        m_squaredSum += (value * value);
    }

private:
    int64_t m_sum;
    int64_t m_squaredSum;
    int16_t m_min;
    int16_t m_max;
};


class VariableMeasure {
public:
    VariableMeasure() {}
    VariableMeasure( float scaleFactor, int16_t min, int16_t max, int16_t mean, float rms ): 
            m_min(min*scaleFactor), 
            m_max(max*scaleFactor), 
            m_mean(mean*scaleFactor),
            m_rms(rms*scaleFactor) {}

    float min() const {
        return m_min;
    }
    
    float max() const {
        return m_max;
    }
    
    float mean() const {
        return m_mean;
    }
    
    float rms() const {
        return m_rms;
    }

private:
    float m_min;
    float m_max;
    float m_mean;
    float m_rms;
};


class PowerMeasure {
public:
    PowerMeasure() {}
    PowerMeasure( float scaleFactor, float active, float apparent );

    // P = sum[i=1..N]( u(i) * i(i) ) / N
    float active() const {
        return m_active;
    }

    // S = Urms * Irms
    float apparent() const {
        return m_apparent;
    }

    // Q = sqrt( S^2 - P^2 )
    float reactive() const {
        return m_reactive;
    }

    // PF = P/S
    float factor() const {
        return m_factor;
    }

private:
    float m_active;
    float m_apparent;
    float m_reactive;
    float m_factor;
};


class CalculatedMeasures {
public:
    CalculatedMeasures() {}
    CalculatedMeasures( uint32_t sampleRate, 
                        uint32_t signalFrequency,
                        const VariableMeasure& voltage,
                        const VariableMeasure& current,
                        const PowerMeasure& power ): 
            m_sampleRate(sampleRate), 
            m_signalFrequency(signalFrequency),
            m_voltage(voltage), 
            m_current(current),
            m_power(power) {}
    
    uint32_t sampleRate() const {
        return m_sampleRate;
    }

    uint32_t signalFrequency() const {
        return m_signalFrequency;
    }

    const VariableMeasure& voltage() const {
        return m_voltage;
    }

    const VariableMeasure& current() const {
        return m_voltage;
    }

    const PowerMeasure& power() const {
        return m_power;
    }

private:
    uint32_t m_sampleRate;
    uint32_t m_signalFrequency;
    VariableMeasure m_voltage;
    VariableMeasure m_current;
    PowerMeasure m_power;
};


class CalculatorBasedMeter {
private:
    static const uint8_t CALLS_TO_UPDATE_SAMPLE_RATE = 5;

public:
    typedef CalculatedMeasures Measures;

public:
    CalculatorBasedMeter();
    ~CalculatorBasedMeter();

    void scaleFactors( const std::pair<float, float>& factors );
    bool process( uint64_t time, const SampleBasedMeter::Measures& samples );

    Measures get();

private:
    void reset();    
    void fetch();
    std::pair<uint32_t, uint32_t> fetchTimes();
    
private:
    float m_voltageScaleFactor;
    float m_currentScaleFactor;
    QueueHandle_t m_valueQueue;
    VariableProcessor m_voltage;
    VariableProcessor m_current;
    int32_t m_activePowerSum;
    int16_t m_lastVoltage;
    size_t m_processedSamples;
    size_t m_sampledPeriods;
    int64_t m_lastTimeFetched;
    CalculatedMeasures m_currentMeasures;
};

}

#endif
