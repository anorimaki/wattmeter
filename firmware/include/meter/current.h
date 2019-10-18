#ifndef CURRENT_METER_H
#define CURRENT_METER_H

#include "meter/singlesampledmeter.h"
#include "driver/adc.h"
#include <array>


namespace meter {

namespace current {

static const adc_channel_t INPUT_CHANNEL = ADC_CHANNEL_3;
static const uint N_RANGES = 3;

typedef std::array<uint16_t, N_RANGES> Zeros;

struct CalibrationData {
	Zeros zeros;
};

CalibrationData readClibrationData();

void setGPIORange( size_t range );

void init();

}


class CurrentMeter: public SingleSampleBasedMeter<current::INPUT_CHANNEL, current::N_RANGES> {
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
	
	typedef SingleSampleBasedMeter<current::INPUT_CHANNEL, current::N_RANGES> Base;
	
public:
	CurrentMeter(): Base(current::setGPIORange) {
		current::init();
	}

	void init( uint16_t defaultZero ) {
		current::CalibrationData calibrationData = current::readClibrationData();
		typedef typename Base::RangesInitializer RangesInitializer;

		current::Zeros defaultZeros;
		defaultZeros.fill(defaultZero);
		const current::Zeros* zeros = (calibrationData.zeros[0]==0) ?
										&defaultZeros : &calibrationData.zeros;

#if 0
		RangesInitializer ranges = {
			range( zeros->at(0), R1 ), 
			range( zeros->at(1), R2+R1 ), 
			range( zeros->at(2), R3+R2+R1 )
		};
#else
		RangesInitializer ranges = {
			Range( zeros->at(0), 0.1078 ), 
			Range( zeros->at(1), 0.0116 ), 
			range( zeros->at(2), R3+R2+R1 )
		};
#endif
		Base::initRanges( ranges );
	}

private:
	Range range( uint16_t zero, uint32_t r ) {
		float factor =  1000.0 / (AMPLIFIER_GAIN * r);
		TRACE( "Current range: %u %f", zero, factor );
		return Range( zero, factor );
	}
};


}

#endif