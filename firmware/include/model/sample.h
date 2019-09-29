#ifndef MODEL_SAMPLE_H
#define MODEL_SAMPLE_H

#include <cmath>
#include <array>


namespace model {

const size_t SamplesGroupSize = 16;

const size_t SamplesSize = 1024 / SamplesGroupSize;

class Sample {
public:
    Sample(): m_voltage(NAN), m_current(NAN) {}
    Sample( float voltage, float current ): m_voltage(voltage), m_current(current) {}

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

typedef std::array<Sample, SamplesSize> Samples;

}

#endif