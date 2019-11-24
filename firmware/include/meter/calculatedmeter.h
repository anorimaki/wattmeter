#ifndef CALCULATOR_METER_H
#define CALCULATOR_METER_H

#include "meter/sampledmeter.h"
#include "esp_timer.h"
#include <stdint.h>
#include <numeric>
#include <limits>

namespace meter {

namespace impl {

class VariableAccumulator {
public:
    VariableAccumulator() {
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

    void accumulate( int16_t value ) {
        m_max = std::max( m_max, value );
        m_min = std::min( m_min, value );
        m_sum += value;
        m_squaredSum += (value * value);
    }

    void accumulate( const VariableAccumulator& other ) {
        m_max = std::max( m_max, other.max() );
        m_min = std::min( m_min, other.min() );
        m_sum += other.sum();
        m_squaredSum += other.squaredSum();
    }

private:
    int64_t m_sum;
    int64_t m_squaredSum;
    int16_t m_min;
    int16_t m_max;
};


class Accumulator {
public:
    Accumulator() {
        reset();
    }

    const VariableAccumulator& voltage() const {
        return m_voltage;
    }

    const VariableAccumulator& current() const {
        return m_current;
    }

    int64_t activePowerSum() const {
        return m_activePowerSum;
    }

    void accumulate( const Accumulator& other ) {
        m_voltage.accumulate( other.voltage() );
        m_current.accumulate( other.current() );
        m_activePowerSum += other.activePowerSum();
    }

    void accumulate( int16_t voltage, int16_t current ) {
        m_voltage.accumulate( voltage );
        m_current.accumulate( current );
        m_activePowerSum += (voltage * current);
    }

    void reset() {
        m_voltage.reset();
        m_current.reset();
        m_activePowerSum = 0;
    }

private:
    VariableAccumulator m_voltage;
    VariableAccumulator m_current;
    int64_t m_activePowerSum;
};


template<typename T, size_t HL>
class IntegerAccumulator {
public:
    static const size_t HistorySizeLog2 = HL;
    static const size_t HistorySize = 1 << HistorySizeLog2;
    typedef std::array<T, HistorySize> History; 

public:
    IntegerAccumulator() {
        reset();
    }

    void reset() {
        m_sum = 0;
        m_last = 0;
        m_first = 0;
        m_size = 0;
    }

    bool full() const {
        return m_size == HistorySize;
    }

    T accumulate( T v ) {
        if ( m_size == HistorySize ) {
            m_sum -= m_history[m_first];
            m_first = inc(m_first);
        }
        else {
            ++m_size;
        }

        m_history[m_last] = v;
        m_last = inc(m_last);

        m_sum += v;
        
        return m_sum;
    }

private:
    static size_t inc( size_t index ) {
        return (++index == HistorySize) ? 0 : index;
    }

private:
    T m_sum;
    size_t m_last;
    size_t m_first;
    size_t m_size;
    History m_history;
};

}


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

    // In tenths of Hz
    uint32_t signalFrequency() const {
        return m_signalFrequency;
    }

    const VariableMeasure& voltage() const {
        return m_voltage;
    }

    const VariableMeasure& current() const {
        return m_current;
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
    typedef impl::IntegerAccumulator<uint32_t, 4> SampledPeriodsAccumulator;

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
    impl::Accumulator m_periodAccumulator;
    impl::Accumulator m_accumulator;
    int16_t m_lastVoltage;
    size_t m_processedSamples;
    size_t m_sampledPeriods;
    SampledPeriodsAccumulator m_sampledPeriodsAccumulator;
    int64_t m_lastTimeFetched;
    CalculatedMeasures m_currentMeasures;
};

}

#endif
