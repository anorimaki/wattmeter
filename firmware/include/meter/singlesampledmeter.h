#ifndef SINGLE_SAMPLED_METER_H
#define SINGLE_SAMPLED_METER_H

#include "meter/ranges.h"
#include "meter/sampler.h"
#include "util/trace.h"

#include "driver/adc.h"

#include <cstdint>
#include <cstddef>
#include <functional>
#include <array>

#if 1
#include <string>
#include <cstdio>
#endif

namespace meter {


template <adc_channel_t Channel, size_t N_RANGES>
class SingleSampleBasedMeter {
private:
    typedef SingleSampleBasedMeter<Channel, N_RANGES> ThisType;
	typedef meter::Ranges<N_RANGES> Ranges;
	typedef Sampler<Channel> CalibrationSampler;
	
protected:
	typedef std::function<void(size_t)> GPIORangeSetter;

public:
    struct CalibrationData {
		std::array<uint16_t, N_RANGES> zeros;
	};
	typedef std::function<CalibrationData()> CalibrationReader;

    typedef std::function<void()> AutoRangeAction;
	
public:
    static const size_t RangesSize = N_RANGES;
    static const adc_channel_t AdcChannel = Channel;
	static const size_t AutoRange = N_RANGES;

public:
	SingleSampleBasedMeter( GPIORangeSetter gpioRangeSetter ): m_gpioRangeSetter(gpioRangeSetter) {
        changeRange(0);
    }

    void init( uint16_t defaultZero, CalibrationReader calibrationReader,
                std::array<float, N_RANGES> scaleFactors ) {
		CalibrationData calibrationData = calibrationReader();

        if ( calibrationData.zeros[0]==0 ) {
            calibrationData.zeros.fill(defaultZero);
        }
        m_ranges.setZeros( calibrationData.zeros );

        m_ranges.setScaleFactors(scaleFactors);
	} 

    template <typename Sample>
	int16_t process( const Sample& sample ) {
        uint16_t value = sample.template get<Channel>();
		return m_ranges.process( value );
	}

	void calibrateZeros() {
		std::array<uint16_t, N_RANGES> zeros;
		sampleAllRanges( zeros );
		m_ranges.setZeros( zeros );
	}

    float scaleFactor() const {
        return m_ranges.scaleFactor();
    }

	void setRange( size_t newRange ) {
        m_ranges.setAutoRange( newRange == AutoRange );
        if ( !m_ranges.autoRange() ) {
            changeRange(newRange);
        }
	}

    AutoRangeAction autoRangeAction() {
        if ( !m_ranges.autoRange() ) {
            return AutoRangeAction();
        }

        size_t bestRange = m_ranges.best();
        return (bestRange == m_ranges.active()) ? 
                AutoRangeAction() :
                std::bind( &ThisType::changeRange, this, bestRange );
    }

protected:
    void changeRange( size_t rangeIndx ) {
        m_ranges.setActive(rangeIndx);
		m_gpioRangeSetter(rangeIndx);
		TRACE("Range changed to: %d", rangeIndx);
    }

	void sampleAllRanges( std::array<uint16_t, N_RANGES>& out ) {
		for( size_t i=0; i<N_RANGES; ++i ) {
			changeRange(i);
			out[i] = takeSample();
		}

#if 1
		std::string buf;
		for ( int i=0; i<N_RANGES; ++i ) {
			char curr[128];
			std::sprintf(curr, "%u", out[i]);
			buf.append( curr);
			buf.append( ", " );
		}
		TRACE("Calibration: %s", buf.c_str());
#endif
	}

	uint16_t takeSample() {
		m_calibrationSampler.start();
		uint16_t ret = m_calibrationSampler.template readAndAverage<Channel>(10);
		m_calibrationSampler.stop();
		return ret;
	}

protected:
	Ranges m_ranges;

private:
	CalibrationSampler m_calibrationSampler;
	GPIORangeSetter m_gpioRangeSetter;
};

}

#endif