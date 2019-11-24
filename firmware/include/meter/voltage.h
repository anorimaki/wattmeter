#ifndef VOLTAGE_H
#define VOLTAGE_H

#include "meter/singlesampledmeter.h"
#include "driver/adc.h"
#include <array>


namespace meter {

namespace voltage {

static const adc_channel_t INPUT_CHANNEL = ADC_CHANNEL_0;
static const uint N_RANGES = 3;

typedef SingleSampleBasedMeter<voltage::INPUT_CHANNEL, voltage::N_RANGES> BaseMeter;

void init();
void setGPIORange( size_t range );

}


class VoltageMeter: public voltage::BaseMeter {
private:
#if 0
	static const uint RH = 1000000;
	static const uint RL_1 = 1100;
	static const uint RL_2 = 12000;
	static const uint RL_3 = 68000;
#elif 0
	static const uint32_t RH = 1012000;
	static const uint32_t RL_1 = 1110;
	static const uint32_t RL_2 = 11950;
	static const uint32_t RL_3 = 67600;
#else //Multimetro
	static const uint32_t RH = 1010000;
	static const uint32_t RL_1 = 1089;
	static const uint32_t RL_2 = 11830;
	static const uint32_t RL_3 = 66600;
#endif
	
public:
	VoltageMeter(): voltage::BaseMeter(voltage::setGPIORange, "voltage") {
		voltage::init();
	}

	void init( uint16_t defaultZero ) {
        const std::array<float, RangesSize> scaleFactors = {
                                        scaleFactorForR(RL_1), 
                                        scaleFactorForR(RL_2+RL_1), 
                                        scaleFactorForR(RL_3+RL_2+RL_1) };
        voltage::BaseMeter::init( defaultZero, scaleFactors );
	}

	void calibrateFactors() {
		std::array<uint16_t, RangesSize> fiveVoltsValue;
		sampleAllRanges( fiveVoltsValue );

        std::array<float, RangesSize> scaleFactors;
        for( uint i = 0; i<RangesSize; ++i ) {
            scaleFactors[i] = 5.0 / float(m_ranges[i].applyOffset(fiveVoltsValue[i]));
        }
        m_ranges.setScaleFactors(scaleFactors);
	}

private:
    static const float scaleFactorForR(uint rl ) {
        return (float(RH+rl)/float(rl)) / 1000.0;
    }
};


}

#endif