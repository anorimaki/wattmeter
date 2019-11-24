#ifndef SINGLE_SAMPLED_METER_H
#define SINGLE_SAMPLED_METER_H

#include "meter/ranges.h"
#include "meter/sampler.h"
#include "util/trace.h"

#include "driver/adc.h"
#include "nvs_flash.h"
#include "nvs.h"

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
    struct CalibrationData {
        std::array<uint16_t, N_RANGES> zeros;
    };
	
protected:
	typedef std::function<void(size_t)> GPIORangeSetter;

public:
    typedef std::function<void()> AutoRangeAction;
	
public:
    static const size_t RangesSize = N_RANGES;
    static const adc_channel_t AdcChannel = Channel;
	static const size_t AutoRange = N_RANGES;

public:
	SingleSampleBasedMeter( GPIORangeSetter gpioRangeSetter, const char* calibrationStore ): 
                m_gpioRangeSetter(gpioRangeSetter), m_calibrationStoreName(calibrationStore) {
        changeRange(0);
        m_ranges.setAutoRange(true);
    }

    void init( uint16_t defaultZero, const std::array<float, N_RANGES>& scaleFactors ) {
		CalibrationData calibrationData = loadCalibration();
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
        saveZerosCalibration( zeros );
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
		TRACE("%s range changed to: %d", m_calibrationStoreName, rangeIndx);
    }

	void sampleAllRanges( std::array<uint16_t, N_RANGES>& out ) {
		for( size_t i=0; i<N_RANGES; ++i ) {
			changeRange(i);
			out[i] = takeSample();
            TRACE("%s range %d calibration: %u", m_calibrationStoreName, i, out[i]);
		}
	}

	uint16_t takeSample() {
		m_calibrationSampler.start();
		uint16_t ret = m_calibrationSampler.template readAndAverage<Channel>(10);
		m_calibrationSampler.stop();
		return ret;
	}

private:
    CalibrationData loadCalibration() {
        CalibrationData ret;

        nvs_handle handle;
        TRACE_ESP_ERROR_CHECK(nvs_open("wattmeter", NVS_READONLY, &handle));
        size_t size = sizeof(uint16_t)*N_RANGES;
        esp_err_t err = nvs_get_blob(handle, m_calibrationStoreName, ret.zeros.data(), &size);
	    if ( err != ESP_OK ) {
		    ret.zeros.fill(0);
	    }
        nvs_close(handle);

        return ret;
    }

    void saveZerosCalibration( const std::array<uint16_t, N_RANGES>& zeros ) {
        nvs_handle handle;
        TRACE_ESP_ERROR_CHECK(nvs_open("wattmeter", NVS_READWRITE, &handle));
        TRACE_ESP_ERROR_CHECK(nvs_set_blob(handle, m_calibrationStoreName, zeros.data(),
                                        sizeof(uint16_t)*N_RANGES ) );
	    TRACE_ESP_ERROR_CHECK(nvs_commit(handle));
        nvs_close(handle);
    }

protected:
	Ranges m_ranges;

private:
	CalibrationSampler m_calibrationSampler;
	GPIORangeSetter m_gpioRangeSetter;
    const char* m_calibrationStoreName;
};

}

#endif