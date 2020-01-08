#include "meter/adc_direct.h"
#include "util/trace.h"
#include "util/indicator.h"
#include "driver/timer.h"
#include "soc/sens_struct.h"
#include "esp_timer.h"
#include "sdkconfig.h"
#include <list>
#include <algorithm>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

namespace adc {

namespace direct {

static const timer_group_t timerGroup = TIMER_GROUP_0;
static const timer_idx_t timerIndex = TIMER_0;

struct TimedBuffer {
    uint64_t startTime;
    adc::Buffer buffer;
};

static std::list<adc1_channel_t> channels;
static intr_handle_t timerIsrHandle;
static TimedBuffer buffers[BufferCount+1];

inline uint16_t local_adc1_read(int channel) {
    SENS.sar_meas_start1.sar1_en_pad = (1 << channel); // only one channel is selected
    while (SENS.sar_slave_addr1.meas_status != 0);
    SENS.sar_meas_start1.meas1_start_sar = 0;
    SENS.sar_meas_start1.meas1_start_sar = 1;
    while (SENS.sar_meas_start1.meas1_done_sar == 0);
    return SENS.sar_meas_start1.meas1_data_sar;
}

static uint16_t* bufferWritePtr;
static uint16_t* bufferWriteEnd;
static int writeBufferIndex;
static int readBufferIndex;
static QueueHandle_t readBufferQueue;

inline int nextBuffer( int currentIndex ) {
    if ( ++currentIndex == BufferCount+1 ) {
        return 0;
    }
    return currentIndex;
}

inline void sendToQueue( int bufferIndex ) {
    if ( xQueueIsQueueFullFromISR(readBufferQueue) ) {
        int dummy1;
        BaseType_t dummy2; 
        xQueueReceiveFromISR(readBufferQueue, &dummy1, &dummy2);
    }
    BaseType_t higherPriorityTaskWoken;
    xQueueSendToBackFromISR( readBufferQueue, &bufferIndex, &higherPriorityTaskWoken );
    if( higherPriorityTaskWoken ) {
     //   taskYIELD_FROM_ISR();
    }
}

inline void setWriteBuffer( int index ) {
    TimedBuffer& writeBuffer = buffers[index];
    writeBuffer.startTime = esp_timer_get_time();
    bufferWritePtr = writeBuffer.buffer.data();
    bufferWriteEnd = bufferWritePtr + BufferSize;
    writeBufferIndex = index;
}

inline void changeWriteBuffer() {
    sendToQueue(writeBufferIndex);

    int nextWriteBuffer = nextBuffer(writeBufferIndex);
    if ( nextWriteBuffer == readBufferIndex ) {
        nextWriteBuffer = nextBuffer(nextWriteBuffer);
    }

    setWriteBuffer(nextWriteBuffer);
}

static void IRAM_ATTR timerIsr(void* arg) {
    TIMERG0.int_clr_timers.t0 = 1;
    TIMERG0.hw_timer[0].config.alarm_en = 1;

    for( std::list<adc1_channel_t>::const_iterator it = channels.begin(); it != channels.end(); ++it ) {
        uint16_t channel = *it;
        uint16_t value = local_adc1_read(channel);
        *bufferWritePtr = (channel << 12) | value;
        if ( ++bufferWritePtr == bufferWriteEnd ) {
            changeWriteBuffer();
        }
    }
}


static void startTimer( int nChannels ) {
    timer_config_t config = {
            .alarm_en = true,				//Alarm Enable
            .counter_en = false,			//If the counter is enabled it will start incrementing / decrementing immediately after calling timer_init()
            .intr_type = TIMER_INTR_LEVEL,	//Is interrupt is triggered on timer’s alarm (timer_intr_mode_t)
            .counter_dir = TIMER_COUNT_UP,	//Does counter increment or decrement (timer_count_dir_t)
            .auto_reload = true,			//If counter should auto_reload a specific initial value on the timer’s alarm, or continue incrementing or decrementing.
            .divider = 80     				//Divisor of the incoming 80 MHz (12.5nS) APB_CLK clock. E.g. 80 = 1uS per timer tick
    };

    timer_init(timerGroup, timerIndex, &config);
    timer_set_counter_value(timerGroup, timerIndex, 0);
    timer_set_alarm_value(timerGroup, timerIndex, (1000000LL * nChannels) / SampleRate);
    timer_enable_intr(timerGroup, timerIndex);
    timer_isr_register(timerGroup, timerIndex, &timerIsr, 
                        NULL, ESP_INTR_FLAG_IRAM, &timerIsrHandle);
    timer_start(timerGroup, timerIndex);
}


void start( const nonstd::span<const adc1_channel_t>& channels ){
    adc::direct::channels = std::list<adc1_channel_t>( channels.begin(), channels.end() );
    std::for_each( channels.begin(), channels.end(), []( adc1_channel_t channel ) {
        adc1_config_width(ADC_WIDTH_BIT_12);
        adc1_config_channel_atten(channel,ADC_ATTEN_DB_0);
        adc1_get_raw(channel);
    });

    setWriteBuffer(0);
    readBufferQueue = xQueueCreate( BufferCount, sizeof(int) );
    startTimer( channels.size() );
}

void stop() {
    timer_pause( timerGroup, timerIndex );
    timer_disable_intr( timerGroup, timerIndex );
  //  timer_deinit( timerGroup, timerIndex );
    esp_intr_free(timerIsrHandle);

    vQueueDelete( readBufferQueue );
}



// Returns time in us when the first value was sampled
int64_t readData( Buffer& buffer ) {
    if( !xQueueReceive( readBufferQueue, &readBufferIndex, portMAX_DELAY ) ) {
        TRACE_ERROR_AND_RETURN(-1);
    }
 //   TRACE("Read: %d", readBufferIndex);

    const TimedBuffer& readBuffer = buffers[readBufferIndex];
    memcpy( buffer.data(), readBuffer.buffer.data(), BufferSize * sizeof(Buffer::value_type) );
    return readBuffer.startTime;
}

}

}
