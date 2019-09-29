#ifndef MEASURER_MEASURER_H
#define MEASURER_MEASURER_H

#include "measurer/ranges.h"
#include "measurer/sampler.h"
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

namespace measurer {


template <adc_channel_t Channel, size_t N_RANGES>
class Measurer {
private:
    typedef Measurer<Channel, N_RANGES> ThisType;
	typedef measurer::Ranges<N_RANGES> Ranges;
	typedef sampler::Sampler<Channel> CalibrationSampler;
	
protected:
	typedef std::function<void(size_t)> GPIORangeSetter;
	typedef std::array<Range, N_RANGES> RangesInitializer;

	struct CalibrationData {
		std::array<uint16_t, N_RANGES> zeros;
	};
	typedef std::function<CalibrationData()> CalibrationZerosReader;

public:
    typedef std::function<void()> AutoRangeAction;
	
public:
    static const adc_channel_t AdcChannel = Channel;
	static const size_t AutoRange = N_RANGES;

public:
	Measurer( GPIORangeSetter gpioRangeSetter ): 
		m_gpioRangeSetter(gpioRangeSetter) {}

	void initRanges( const RangesInitializer& ranges ) {
		m_ranges = Ranges( ranges );
		changeRange(0);		//Start with the widest range
	}

    template <typename Sample>
	float process( const Sample& sample ) {
        uint16_t value = sample.template get<Channel>();
		return m_ranges.process( value );
	}

	void calibrateZeros() {
		std::array<uint16_t, N_RANGES> zeros;
		sampleAllRanges( zeros );
		for( int i=0; i<N_RANGES; ++i ) {
			m_ranges[i].setZero( zeros[i] );
		}
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
                AutoRangeAction( std::bind( &ThisType::changeRange, this, bestRange ) );
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