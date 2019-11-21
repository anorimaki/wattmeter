#include "meter/calculatedmeter.h"
#include <cmath>

namespace meter {

PowerMeasure::PowerMeasure( float scaleFactor, float active, float apparent ) {
    m_active = active * scaleFactor;
    m_apparent = apparent * scaleFactor;
    m_reactive = std::sqrt( apparent*apparent - active*active ) * scaleFactor;
    m_factor = active / apparent;
}


inline bool endPeriod( float lastValue, float currentValue ) {
    return (lastValue > 0) && (currentValue <= 0);
}

static const size_t SamplesInChunk = _::SampleRate * 3 / _::SamplesGroupSize ;

CalculatorBasedMeter::CalculatorBasedMeter() {
    m_valueQueue = xQueueCreate( 1, sizeof(Measures) );
}

CalculatorBasedMeter::~CalculatorBasedMeter() {
    vQueueDelete(m_valueQueue);
}

CalculatorBasedMeter::Measures CalculatorBasedMeter::get() {
    Measures ret;
    xQueueReceive( m_valueQueue, &ret, portMAX_DELAY );
    return ret;
}

void CalculatorBasedMeter::scaleFactors( const std::pair<float, float>& factors ) {
    m_voltageScaleFactor = factors.first;
    m_currentScaleFactor = factors.second;
    reset();
}

bool CalculatorBasedMeter::process( uint64_t time, const SampleBasedMeter::Measures& samples ) {
    typedef SampleBasedMeter::Measure SampledMeasure;
    bool chunkCompleted = false;
    std::for_each( samples.begin(), samples.end(), 
                [this, &chunkCompleted]( const SampledMeasure& sampledMeasure ) {
        int16_t voltage = sampledMeasure.voltage();
        int16_t current = sampledMeasure.current();
        m_voltage.process(voltage);
        m_current.process(current);
        m_activePowerSum += (voltage * current);
        ++m_processedSamples;

        bool isReadyToFetch = (m_processedSamples >= SamplesInChunk);
        bool isDCVoltage = (m_sampledPeriods == 0);
        if ( isReadyToFetch && isDCVoltage ) {
            fetch();
            chunkCompleted = true;
        }
        else if ( endPeriod( m_lastVoltage, voltage ) ) {
            ++m_sampledPeriods;
            if ( isReadyToFetch ) {
                fetch();
                chunkCompleted = true;
            }
        }
        m_lastVoltage = voltage;
    });

    return chunkCompleted;
}


void CalculatorBasedMeter::reset() {
    m_voltage.reset();
    m_current.reset();
    
    m_activePowerSum = 0;
    m_lastVoltage = 0;
    m_processedSamples = 0;
    m_sampledPeriods = 0;
}


void CalculatorBasedMeter::fetch() {
    float unscaledVoltageRms = std::sqrt(m_voltage.squaredSum() / m_processedSamples);
    VariableMeasure voltage( m_voltageScaleFactor, m_voltage.min(), m_voltage.max(), 
                            m_voltage.sum() / m_processedSamples, unscaledVoltageRms );

    float unscaledCurrentRms = std::sqrt(m_current.squaredSum() / m_processedSamples);
    VariableMeasure current( m_currentScaleFactor, m_current.min(), m_current.max(), 
                            m_current.sum() / m_processedSamples, unscaledCurrentRms );

    PowerMeasure power = PowerMeasure( m_voltageScaleFactor * m_currentScaleFactor,
                                        m_activePowerSum / m_processedSamples, 
                                        unscaledVoltageRms * unscaledCurrentRms ); 

    int32_t sampleRate, signalFrequency;
    std::tie(sampleRate, signalFrequency) = fetchTimes();

    Measures measures = Measures(sampleRate, signalFrequency, voltage, current, power); 
    xQueueOverwrite( m_valueQueue, &measures );
    reset();
}


std::pair<uint32_t, uint32_t> CalculatorBasedMeter::fetchTimes() {
    int64_t lastTime = m_lastTimeFetched;
    m_lastTimeFetched = esp_timer_get_time();            //In us
    int32_t interval = m_lastTimeFetched-lastTime;
    interval /= 1000;

    int32_t samples = m_processedSamples * (_::SamplesGroupSize * 1000UL);
    int32_t periods = m_sampledPeriods * 1000UL;

    return std::make_pair(samples / interval, periods / interval);
}


}