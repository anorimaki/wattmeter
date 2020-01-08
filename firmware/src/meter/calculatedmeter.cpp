#include "meter/calculatedmeter.h"
#include <cmath>

namespace meter {

PowerMeasure::PowerMeasure( float scaleFactor, float active, float apparent ) {
    m_active = active * scaleFactor;
    m_apparent = apparent * scaleFactor;
    m_reactive = std::sqrt( apparent*apparent - active*active ) * scaleFactor;
    m_factor = active / apparent;
}

static const size_t MeasuresPerSecond = adc::SampleRate / adc::SamplesGroupSize;
static const size_t SamplesInChunk = MeasuresPerSecond * 1 ;


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
        m_periodAccumulator.accumulate( voltage, current );        

        if ( (m_lastVoltage > 0) && (voltage <= 0) ) {
            ++m_sampledPeriods;
            m_accumulator.accumulate( m_periodAccumulator );
            m_periodAccumulator.reset();
        }
        m_lastVoltage = voltage;

        if ( ++m_processedSamples > SamplesInChunk ) {
            fetch();
            chunkCompleted = true;
        }
    });

    return chunkCompleted;
}


void CalculatorBasedMeter::reset() {
    m_periodAccumulator.reset();
    m_accumulator.reset();
    
    m_lastVoltage = 0;
    m_processedSamples = 0;
    m_sampledPeriods = 0;
}


void CalculatorBasedMeter::fetch() {
    if ( m_sampledPeriods == 0 ) {
        m_accumulator.accumulate( m_periodAccumulator );
        m_periodAccumulator.reset();
    }

    const impl::VariableAccumulator& voltageAccum = m_accumulator.voltage();
    float unscaledVoltageRms = std::sqrt(voltageAccum.squaredSum() / m_processedSamples);

    VariableMeasure voltage( m_voltageScaleFactor, voltageAccum.min(), voltageAccum.max(), 
                            voltageAccum.sum() / m_processedSamples, 
                            unscaledVoltageRms );
    const impl::VariableAccumulator& currentAccum =  m_accumulator.current();
    float unscaledCurrentRms = std::sqrt(currentAccum.squaredSum() / m_processedSamples);
    VariableMeasure current( m_currentScaleFactor, currentAccum.min(), currentAccum.max(), 
                            currentAccum.sum() / m_processedSamples, 
                            unscaledCurrentRms );

    PowerMeasure power = PowerMeasure( m_voltageScaleFactor * m_currentScaleFactor,
                                        m_accumulator.activePowerSum() / m_processedSamples, 
                                        unscaledVoltageRms * unscaledCurrentRms ); 

    int32_t sampleRate, signalFrequency;
    std::tie(sampleRate, signalFrequency) = fetchTimes();

    Measures measures = Measures(sampleRate, signalFrequency, voltage, current, power); 
    xQueueOverwrite( m_valueQueue, &measures );
    reset();
}


std::pair<uint32_t, uint32_t> CalculatorBasedMeter::fetchTimes() {
    uint64_t lastTime = m_lastTimeFetched;
    m_lastTimeFetched = esp_timer_get_time();           // In us
    uint32_t interval = m_lastTimeFetched-lastTime;
    interval /= 1000;                                   // In ms

    uint32_t samples = m_processedSamples * (adc::SamplesGroupSize * 1000UL);
    
    uint32_t periods = 0;
    if ( m_sampledPeriods > 0 ) {       // Is AC
        periods = m_sampledPeriodsAccumulator.accumulate(m_sampledPeriods);
        if ( m_sampledPeriodsAccumulator.full() ) {
            // signalFrequency is measured in cents of Hz. So multiply by 100000 instead of 1000
            periods = periods * (100000UL / SampledPeriodsAccumulator::HistorySize) ;
        }
        else {
            periods = m_sampledPeriods * 100000UL;
        }
    }
    else {
        m_sampledPeriodsAccumulator.reset();
    }

    return std::make_pair(samples / interval, periods / interval);
}


}