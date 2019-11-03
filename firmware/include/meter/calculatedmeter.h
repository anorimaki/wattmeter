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
        m_min = std::numeric_limits<float>::max();
        m_max = std::numeric_limits<float>::min();
        m_sum = 0;
        m_squaredSum = 0;
    }

    float sum() const {
        return m_sum;
    }

    float squaredSum() const {
        return m_squaredSum;
    }

    float min() const {
        return m_min;
    }

    float max() const {
        return m_max;
    }

    void process( float value ) {
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
    float m_sum;
    float m_squaredSum;
    float m_min;
    float m_max;
};


class VariableMeasure {
public:
    VariableMeasure(): m_min(0.0), m_max(0.0), m_mean(0.0), m_rms(0.0) {}
    VariableMeasure( float min, float max, float mean, float rms ): 
        m_min(min), m_max(max), m_mean(mean), m_rms(rms) {}

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
    PowerMeasure(): m_active(0.0), m_apparent(0.0), m_reactive(0.0), m_factor(0.0) {}
    PowerMeasure( float active, float apparent );

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
    CalculatedMeasures(): m_sampleRate(0), m_signalFrequency(0) {}
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

    Measures get();
    void process( uint64_t time, const SampleBasedMeter::Measures& samples );

private:
    void reset();
    void fetch();
    std::pair<uint32_t, uint32_t> fetchTimes();
    
private:
    QueueHandle_t m_valueQueue;
    VariableProcessor m_voltage;
    VariableProcessor m_current;
    float m_activePowerSum;
    float m_lastVoltage;
    size_t m_processedSamples;
    size_t m_sampledPeriods;
    int64_t m_lastTimeFetched;
    CalculatedMeasures m_currentMeasures;
};

}

#endif
