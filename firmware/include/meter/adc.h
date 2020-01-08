#ifndef ADC_H
#define ADC_H

#include "driver/adc.h"
#include "nonstd/span.hpp"

namespace adc {

#if 1
const size_t SampleRate = 44000;        //Min: 6000
const size_t SamplesGroupSize = 16;
const size_t BufferSize = 1024;      //in number of samples (not bytes)
const size_t BufferCount = 2;
const size_t GroupedSamplesSize = BufferSize / SamplesGroupSize;


void start( const nonstd::span<const adc1_channel_t>& channels );

void stop();

typedef std::array<uint16_t, BufferSize> Buffer;

// Returns time in us when the first value was sampled
int64_t readData( Buffer& buffer );

#else

const size_t SampleRate = 44000;        //Min: 6000
const size_t SamplesGroupSize = 16;
const size_t BufferSize = 1024;      //in number of samples (not bytes)
const size_t BufferCount = 2;

typedef std::array<uint16_t, BufferSize> Buffer;

void start( const nonstd::span<const adc1_channel_t>& channels );
void stop();




#endif


}


#endif
