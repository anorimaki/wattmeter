#ifndef ADC_H
#define ADC_H

#include "util/trace.h"
#include "driver/adc.h"
#include "model/sample.h"
#include "nonstd/span.hpp"
#include <array>
#include <type_traits>
#include <functional>


namespace sampler {

namespace _ {

const size_t SampleRate = 44000;        //Min: 6000
const size_t SamplesGroupSize = model::SamplesGroupSize;
const size_t BufferSize = model::SamplesSize * model::SamplesGroupSize;      //in number of samples (not bytes)
const size_t BufferCount = 2;
const size_t MaxSamples = model::SamplesSize;

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
size_t readData( Buffer& buffer );

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

	typedef std::array<Sample, _::MaxSamples> Samples;

public:
	Sampler(): m_channels({ Channels... }) {}

	void start() {
		_::start( nonstd::span<const adc_channel_t>( m_channels ) );
	}

	void stop() {
		_::stop();
	}

    void pauseWhileAction( std::function<void()> action ) {
        stop();
        action();
        start();
    }

	size_t read( Samples& samples ) {
	//	if ( !_::receivedData() ) {
	//		return 0;
	//	}

		typedef std::array<uint16_t, _::BufferSize> Buffer;
		Buffer buffer;
		size_t valuesRead = _::readData( buffer );

        const nonstd::span<uint16_t> values( buffer.data(), valuesRead );
		nonstd::span<uint16_t>::const_iterator it = values.begin();
		size_t nSamples = 0;
		while( it < values.end() ) {
            size_t blockSize = std::min<size_t>( std::distance( it, values.cend() ),
												 _::SamplesGroupSize );
			samples[nSamples] = process( nonstd::span<const uint16_t>( it, blockSize ) );
            it += blockSize;
			++nSamples;
		}
		
		return nSamples;
	}

	template <adc_channel_t Channel>
	uint16_t readAndAverage( uint nBuffers = 1 ) {
		size_t totalRead = 0;
		uint32_t totalSum = 0;
		for ( uint i=0; i<nBuffers; ++i ) {
			size_t nRead = 0;
			Samples samples;
			while(nRead == 0) {
				nRead = read( samples );
			}

			totalRead += nRead;
			totalSum += std::accumulate( samples.begin(), samples.begin() + nRead, 0, 
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
            abort();
		}
		return std::distance( m_channels.begin(), it );
	}

private:
	const typename ChannelsTraits::Array m_channels;
};

}

#endif