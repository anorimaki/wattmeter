#ifndef METER_SAMPLER_H
#define METER_SAMPLER_H

#include "util/trace.h"
#include "driver/adc.h"
#include "nonstd/span.hpp"
#include "freertos/semphr.h"
#include <array>
#include <type_traits>
#include <functional>



namespace meter {

namespace _ {

const size_t SampleRate = 44000;        //Min: 6000
const size_t SamplesGroupSize = 16;
const size_t BufferSize = 1024;      //in number of samples (not bytes)
const size_t BufferCount = 2;
const size_t GroupedSamplesSize = BufferSize / SamplesGroupSize;

// - Sample rate for various channels: SampleRate / NumChannels. For 2 channels (V and I):
//      22000 samples/s (period = 45.45 us). A 50Hz signal is sampled 440 times per cycle.
// - Sample rate after grouping: SampleRate / SamplesGroupSize = 2750 values/s (period = 36.36 us).
//      For a 50Hz signal, we have 55 values per cycle for V and I. 
// - Each channel value of a group is the average of SamplesGroupSize / NumChannels values.
// - Buffer sample time: BufferSize / SampleRate = 23.272 ms. It's nearly a period of
//   a 50Hz sinusoidal (20ms).

template <adc_channel_t Channel, size_t CurrentPosition, adc_channel_t... Channels>
struct FindChannelPosition {};

template <adc_channel_t Channel, size_t CurrentPosition, adc_channel_t Current>
struct FindChannelPosition<Channel, CurrentPosition, Current>:
		std::conditional<Channel==Current, 
						std::integral_constant<size_t, CurrentPosition>, 
						void>::type {};

template <adc_channel_t Channel, size_t CurrentPosition, 
			adc_channel_t Current, adc_channel_t... Channels>
struct FindChannelPosition<Channel, CurrentPosition, Current, Channels...>: 
		std::conditional<Channel==Current, 
						std::integral_constant<size_t, CurrentPosition>, 
						FindChannelPosition<Channel, CurrentPosition+1, Channels...> >::type {};

template <adc_channel_t First, adc_channel_t... Channels>
struct ChannelsTraits {
	template <adc_channel_t Channel>
	struct ChannelPosition: FindChannelPosition<Channel, 0, First, Channels...> {};

	static const size_t size = 1 + sizeof...(Channels);

	typedef std::array<adc_channel_t, size> Array;
};

bool receivedData();

typedef std::array<uint16_t, _::BufferSize> Buffer;

// Returns time in us when the first value was sampled
int64_t readData( Buffer& buffer );

void start( const nonstd::span<const adc_channel_t>& channels );

void stop();

uint16_t rawToMilliVolts( uint16_t );

}


void setAdcVref( uint16_t value );


template <adc_channel_t... Channels>
class Sampler {
private:
	typedef _::ChannelsTraits<Channels...> ChannelsTraits;

public:
	class Sample {
    public:
        static const uint16_t UNDEFINED_VALUE = 0xFFFF;

	public:
		Sample() {}

		Sample( const std::array<uint16_t, ChannelsTraits::size>& variables ):
			m_variables(variables){}

		template <adc_channel_t Channel>
		uint16_t get() const {
			return m_variables[ChannelsTraits::template ChannelPosition<Channel>::value];
		}

	private:
		std::array<uint16_t, ChannelsTraits::size> m_variables;
	};

	typedef std::array<Sample, _::GroupedSamplesSize> Samples;

public:
	Sampler(): m_channels({ Channels... }) {
        m_accessSemaphore = xSemaphoreCreateBinary();
    }

    ~Sampler() {
        vSemaphoreDelete(m_accessSemaphore);
    }

	void start() {
		_::start( nonstd::span<const adc_channel_t>( m_channels ) );
        xSemaphoreGive( m_accessSemaphore );
	}

	void stop() {
        xSemaphoreTake( m_accessSemaphore, portMAX_DELAY );
		_::stop();
	}

    void pauseWhileAction( std::function<void()> action ) {
        stop();
        action();
        start();
    }

	uint64_t read( Samples& samples ) {
        xSemaphoreTake( m_accessSemaphore, portMAX_DELAY );

		_::Buffer buffer;
		uint64_t firstSampleTime = _::readData( buffer );

        xSemaphoreGive( m_accessSemaphore );

        _::Buffer::const_iterator it = buffer.begin();
		size_t nSamples = 0;
		while( it < buffer.end() ) {
			samples[nSamples] = process( nonstd::span<const uint16_t>( it, _::SamplesGroupSize ) );
            it += _::SamplesGroupSize;
			++nSamples;
		}		
		return firstSampleTime;
	}

	template <adc_channel_t Channel>
	uint16_t readAndAverage( uint nBuffers = 1 ) {
		size_t totalRead = 0;
		uint32_t totalSum = 0;
		for ( uint i=0; i<nBuffers; ++i ) {
			Samples samples;
			read( samples );

			totalRead += _::GroupedSamplesSize;
			totalSum += std::accumulate( samples.begin(), samples.end(), 0, 
					[] (size_t sum, const Sample& sample) {
						return sum + sample.template get<Channel>();
					});
		}
		return totalSum / totalRead;
	}

private:
	class Measure {
	public:
		Measure(): m_sum(0), m_count(0) {}

		void add(uint16_t value) {
			++m_count;
			m_sum += value;
		}

		uint16_t average() const {
			return (m_count==0) ? Sample::UNDEFINED_VALUE : (m_sum / m_count);
		}

	private:
		uint32_t m_sum;
		uint32_t m_count;
	};


	template <typename C>
	Sample process( const C& values ) {
		std::array<Measure, ChannelsTraits::size> measures;
		std::for_each( values.begin(), values.end(), [&](uint16_t sample) {
			uint16_t channel = sample >> 12;
			uint16_t value = sample & 0xFFF;
            size_t pos = findChannelPosition(static_cast<adc_channel_t>(channel));
            if ( pos < measures.size() ) {
                measures[pos].add(value);
            }
		});

		std::array<uint16_t, ChannelsTraits::size> variables;
		std::transform( measures.cbegin(), measures.cend(), variables.begin(), 
				[](const Measure& measure) {
					uint16_t average = measure.average();
                    return _::rawToMilliVolts(average);
				});
		return Sample(variables);
	}

	size_t findChannelPosition(adc_channel_t channel) {
		typename ChannelsTraits::Array::const_iterator it = 
						std::find( m_channels.begin(), m_channels.end(), channel );
		if ( it == m_channels.end() ) {
			TRACE( "Data found for unknown channel: %d", channel );
          //  abort();
		}
		return std::distance( m_channels.begin(), it );
	}

private:
	const typename ChannelsTraits::Array m_channels;
    SemaphoreHandle_t m_accessSemaphore;
};

}

#endif