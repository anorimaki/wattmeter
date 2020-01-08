#ifndef ADC_DMA_H
#define ADC_DMA_H

#include "meter/adc.h"

namespace adc {

namespace dma {

void start( const nonstd::span<const adc1_channel_t>& channels );

void stop();

// Returns time in us when the first value was sampled
int64_t readData( Buffer& buffer );

}

}


#endif