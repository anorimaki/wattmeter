#ifndef SAMPLE_PROCESSOR_H
#define SAMPLE_PROCESSOR_H

#include "measurer/voltage.h"
#include "measurer/current.h"
#include "model/sample.h"

namespace measurer {

class SamplerProcessor {
public:
    SamplerProcessor( Voltage* voltageMeasurer, Current* currentMeasurer ):
            m_voltageMeasurer(voltageMeasurer), m_currentMeasurer(currentMeasurer) {}

    template <typename IN, typename OUT>
    void process( const IN& samplerOutput, OUT& processedSamples ) {
        typedef typename IN::value_type InputSample;
        std::transform( samplerOutput.begin(), samplerOutput.end(), processedSamples.begin(), 
            [&](const InputSample& inputSample) {
                float voltage = m_voltageMeasurer->process( inputSample );
                float current = m_currentMeasurer->process( inputSample );
                return model::Sample( voltage, current );
            });
    }

private:
    Voltage* m_voltageMeasurer;
    Current* m_currentMeasurer;
};

}

#endif