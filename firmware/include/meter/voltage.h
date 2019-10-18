#ifndef VOLTAGE_H
#define VOLTAGE_H

#include "meter/singlesampledmeter.h"
#include "driver/adc.h"
#include <array>


namespace meter {

namespace voltage {

static const adc_channel_t INPUT_CHANNEL = ADC_CHANNEL_0;

struct CalibrationData {
	std::array<uint16_t, 3> zeros;
};

CalibrationData readClibrationData();

void init();
void setGPIORange( size_t range );

}


class VoltageMeter: public SingleSampleBasedMeter<voltage::INPUT_CHANNEL, 3> {
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
    typedef SingleSampleBasedMeter<voltage::INPUT_CHANNEL, 3> Base;
	
public:
	VoltageMeter(): Base(voltage::setGPIORange) {
		voltage::init();
	}

	void init( uint16_t defaultZero ) {
		voltage::CalibrationData calibrationData = voltage::readClibrationData();
		typedef typename Base::RangesInitializer RangesInitializer;

		RangesInitializer ranges = (calibrationData.zeros[0]==0) ? 
				(RangesInitializer){ 
					range( defaultZero, RL_1 ), 
					range( defaultZero, RL_2+RL_1 ), 
					range( defaultZero, RL_3+RL_2+RL_1 ) } :
				(RangesInitializer){ 
					range( calibrationData.zeros[0], RL_1 ), 
					range( calibrationData.zeros[1], RL_2+RL_1 ), 
					range( calibrationData.zeros[2], RL_3+RL_2+RL_1 ) };
		Base::initRanges( ranges );
	}

	void calibrateFactors() {
		std::array<uint16_t, 3> fiveVoltsValue;
		Base::sampleAllRanges( fiveVoltsValue );

		for( int i=0; i<3; ++i ) {
			uint16_t fiveVoltValue = fiveVoltsValue[i] - Base::m_ranges[i].zero();
			Base::m_ranges[i].setFactor( 5000.0 / float(fiveVoltValue) );
		}
	}

private:
	Range range( uint16_t zero, uint rl ) {
		return Range( zero, float(RH+rl)/float(rl) );
	}
};


}

#endif