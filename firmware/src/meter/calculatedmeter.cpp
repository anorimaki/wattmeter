#include "meter/calculatedmeter.h"
#include <cmath>

namespace meter {

PowerMeasure::PowerMeasure( float active, float apparent ): 
        m_active( active ), 
        m_apparent(apparent),
        m_reactive( std::sqrt( apparent*apparent - active*active ) ),
        m_factor( active / apparent ) {}


inline bool endPeriod( float lastValue, float currentValue ) {
    return (lastValue > 0) && (currentValue <= 0);
}

static const size_t SamplesInChunk = _::SampleRate * 3 / _::GroupedSamplesSize ;

CalculatorBasedMeter::CalculatorBasedMeter() {
    m_valueQueue = xQueueCreate( 1, sizeof( unsigned long ) );
}

CalculatorBasedMeter::~CalculatorBasedMeter() {
    vQueueDelete(m_valueQueue);
}

CalculatorBasedMeter::Measures CalculatorBasedMeter::get() {
    CalculatorBasedMeter::Measures ret;
    xQueueReceive( m_valueQueue, &ret, portMAX_DELAY );
    return ret;
}

void CalculatorBasedMeter::process( uint64_t time, const SampleBasedMeter::Measures& samples ) {
    typedef SampleBasedMeter::Measure SampledMeasure;
    std::for_each( samples.begin(), samples.end(), [this]( const SampledMeasure& sampledMeasure ) {
        float voltage = sampledMeasure.voltage();
        float current = sampledMeasure.current();
        m_voltage.process(voltage);
        m_current.process(current);
        m_activePowerSum += (voltage * current);
        ++m_processedSamples;

        bool isReadyToFetch = (m_processedSamples >= SamplesInChunk);
        bool isDCVoltage = (m_sampledPeriods == 0);
        if ( isReadyToFetch && isDCVoltage ) {
            fetch();
        }
        else if ( endPeriod( m_lastVoltage, voltage ) ) {
            ++m_sampledPeriods;
            if ( isReadyToFetch ) {
                fetch();
            }
        }
        m_lastVoltage = voltage;
    });
}


void CalculatorBasedMeter::reset() {
    m_voltage.reset();
    m_current.reset();
    
    m_activePowerSum = 0;
    m_lastVoltage = 0;
    m_processedSamples = 0;
    m_sampledPeriods = 0;
}


inline VariableMeasure convert( const VariableProcessor& processor, size_t size ) {
    return VariableMeasure( processor.min(), 
                            processor.max(), 
                            processor.sum() / size,
                            std::sqrt(processor.squaredSum() / size) );
}


void CalculatorBasedMeter::fetch() {
    int32_t sampleRate, signalFrequency;
    std::tie(sampleRate, signalFrequency) = fetchTimes();
    VariableMeasure voltage = convert( m_voltage, m_processedSamples );
    VariableMeasure current = convert( m_current, m_processedSamples );
    PowerMeasure power = PowerMeasure( m_activePowerSum / m_processedSamples, 
                                        voltage.rms() * current.rms() ); 
    CalculatedMeasures measures = 
                CalculatedMeasures(sampleRate, signalFrequency, voltage, current, power); 
    xQueueOverwrite( m_valueQueue, &measures );
    reset();
}


std::pair<uint32_t, uint32_t> CalculatorBasedMeter::fetchTimes() {
    int64_t lastTime = m_lastTimeFetched;
    m_lastTimeFetched = esp_timer_get_time();            //In us
    int32_t interval = (m_lastTimeFetched-lastTime) / 1000;

    int32_t samples = m_processedSamples * (_::SamplesGroupSize * 1000UL);
    int32_t periods = m_sampledPeriods * 1000UL;

    return std::make_pair(samples / interval, periods / interval);
}


}