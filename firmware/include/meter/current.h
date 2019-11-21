#ifndef CURRENT_METER_H
#define CURRENT_METER_H

#include "meter/singlesampledmeter.h"
#include "driver/adc.h"
#include <array>


namespace meter {

namespace current {

static const adc_channel_t INPUT_CHANNEL = ADC_CHANNEL_3;
static const uint N_RANGES = 3;

typedef SingleSampleBasedMeter<INPUT_CHANNEL, N_RANGES> BaseMeter;

BaseMeter::CalibrationData readClibrationData();

void setGPIORange( size_t range );

void init();

}


class CurrentMeter: public current::BaseMeter {
private:
#if 0		//teorico
	static const uint32_t AMPLIFIER_R_IN = 10000;
	static const uint32_t AMPLIFIER_R_FB = 75000;
#elif 1    // transistor tester
	static const uint32_t R1 = 930;			//In mOhms
	static const uint32_t R2 = 9240;		//In mOhms
	static const uint32_t R3 = 91200;		//In mOhms
	static constexpr float AMPLIFIER_R_IN = 9964.0;	//In Ohms
	static constexpr float AMPLIFIER_R_FB = 75890.0;	//In Ohms
#else //Multimetro

#endif
	static constexpr float AMPLIFIER_GAIN = 1.0 + (AMPLIFIER_R_FB / AMPLIFIER_R_IN);
	
public:
	CurrentMeter(): current::BaseMeter(current::setGPIORange) {
		current::init();
	}

    void init( uint16_t defaultZero ) {
#if 0
        const std::array<float, RangesSize> scaleFactors = {
                                        scaleFactorForR(R1), 
                                        scaleFactorForR(R2+R1), 
                                        scaleFactorForR(R3+R2+R1) };
#else
        const std::array<float, RangesSize> scaleFactors = {
                                        0.0001078, 
                                        0.0000116, 
                                        scaleFactorForR(R3+R2+R1) };
#endif
        current::BaseMeter::init( defaultZero, current::readClibrationData, scaleFactors );
	}

private:
	static float scaleFactorForR( uint32_t r ) {
		return 1 / (AMPLIFIER_GAIN * r);
	}
};


}

#endif