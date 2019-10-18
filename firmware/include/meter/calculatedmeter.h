#ifndef CALCULATOR_METER_H
#define CALCULATOR_METER_H

#include "meter/sampledmeter.h"
#include "esp_timer.h"
#include <stdint.h>
#include <numeric>


namespace meter {


class CalculatedMeasures {
public:
    CalculatedMeasures( uint32_t sampleRate, float voltage ): 
            m_sampleRate(sampleRate), 
            m_voltage(voltage) {}
    
    uint32_t sampleRate() const {
        return m_sampleRate;
    }

    float voltage() const {
        return m_voltage;
    }

private:
    uint32_t m_sampleRate;
    float m_voltage;
};


class CalculatorBasedMeter {
private:
    static const uint8_t CALLS_TO_UPDATE_SAMPLE_RATE = 5;

public:
    typedef CalculatedMeasures Measures;

public:
    CalculatorBasedMeter(): m_currentMeasures( 0, 0 ) ,
                            m_callsToUpdateSampleRate(CALLS_TO_UPDATE_SAMPLE_RATE) {
        m_mutexAccess = xSemaphoreCreateMutex();
    }

    ~CalculatorBasedMeter() {
        vSemaphoreDelete(m_mutexAccess);
    }

    Measures fetch() const {
        xSemaphoreTake( m_mutexAccess, portMAX_DELAY );
        CalculatedMeasures ret = m_currentMeasures;
        xSemaphoreGive( m_mutexAccess );
        return ret;
    }

    void process( const SampleBasedMeter::Measures& samples ) {
        float volts = calculateVolts(samples);
        float sampleRate = calculateSampleRate();

        xSemaphoreTake( m_mutexAccess, portMAX_DELAY );
        m_currentMeasures = Measures(sampleRate, volts);
        xSemaphoreGive( m_mutexAccess );
    }

private:
    float calculateVolts( const SampleBasedMeter::Measures& samples ) {
        float voltageSum = std::accumulate( samples.begin(), samples.end(), 0.0,
            [](float sum, const SampleBasedMeter::Measure& sample) {
                return sum + sample.voltage();
            });
        return voltageSum / samples.size();
    }

    uint32_t calculateSampleRate() {
        if  ( --m_callsToUpdateSampleRate > 0 ) {
            return m_currentMeasures.sampleRate();
        }
        m_callsToUpdateSampleRate = CALLS_TO_UPDATE_SAMPLE_RATE;

        int64_t lastTime = m_lastTimeUpdatedSampleRate;
        m_lastTimeUpdatedSampleRate = esp_timer_get_time();            //In us
        int32_t interval = (m_lastTimeUpdatedSampleRate-lastTime) / 1000;

        constexpr int32_t samples = _::BufferSize *  
                                    CALLS_TO_UPDATE_SAMPLE_RATE *
                                    1000UL / 2;

        return samples / interval;
    }

private:
    SemaphoreHandle_t m_mutexAccess;
    Measures m_currentMeasures;
    int64_t m_lastTimeUpdatedSampleRate;
    uint8_t m_callsToUpdateSampleRate;
};

}

#endif
