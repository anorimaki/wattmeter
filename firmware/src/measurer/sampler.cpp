#include "measurer/sampler.h"
#include "util/trace.h"

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
#include "driver/i2s.h"
#include "driver/adc.h"
#include "driver/gpio.h"
#include "soc/rtc.h"
#include "soc/syscon_reg.h"
#include "esp_event_loop.h"
#include "esp_adc_cal.h"

extern "C" {
#include "soc/syscon_struct.h"
}

#include <algorithm>
#include <iterator>


extern portMUX_TYPE rtc_spinlock;

namespace sampler {

static uint32_t adcVref = 1108;			// default value. Value in mV

void setAdcVref( uint16_t value ) {
	adcVref = value;
}

void routeVref( gpio_num_t gpio ) {
	TRACE_ESP_ERROR_CHECK( adc2_vref_to_gpio(gpio) );
}

namespace _ {

static QueueHandle_t i2sEventQueue;
static esp_adc_cal_characteristics_t adcCalCharacteristics;


static esp_err_t adc_set_i2s_data_len(adc_unit_t adc_unit, int patt_len) {
    portENTER_CRITICAL(&rtc_spinlock);
    if(adc_unit & ADC_UNIT_1) {
        SYSCON.saradc_ctrl.sar1_patt_len = patt_len - 1;
    }
    if(adc_unit & ADC_UNIT_2) {
        SYSCON.saradc_ctrl.sar2_patt_len = patt_len - 1;
    }
    portEXIT_CRITICAL(&rtc_spinlock);
    return ESP_OK;
}


static esp_err_t adc_set_i2s_data_pattern(adc_unit_t adc_unit, 
									int seq_num, adc_channel_t channel, 
									adc_bits_width_t bits, adc_atten_t atten) {
    portENTER_CRITICAL(&rtc_spinlock);
    //Configure pattern table, each 8 bit defines one channel
    //[7:4]-channel [3:2]-bit width [1:0]- attenuation
    //BIT WIDTH: 3: 12BIT  2: 11BIT  1: 10BIT  0: 9BIT
    //ATTEN: 3: ATTEN = 11dB 2: 6dB 1: 2.5dB 0: 0dB
    uint8_t val = (channel << 4) | (bits << 2) | (atten << 0);
    if (adc_unit & ADC_UNIT_1) {
        SYSCON.saradc_sar1_patt_tab[seq_num / 4] &= (~(0xff << ((3 - (seq_num % 4)) * 8)));
        SYSCON.saradc_sar1_patt_tab[seq_num / 4] |= (val << ((3 - (seq_num % 4)) * 8));
    }
    if (adc_unit & ADC_UNIT_2) {
        SYSCON.saradc_sar2_patt_tab[seq_num / 4] &= (~(0xff << ((3 - (seq_num % 4)) * 8)));
        SYSCON.saradc_sar2_patt_tab[seq_num / 4] |= (val << ((3 - (seq_num % 4)) * 8));
    }
    portEXIT_CRITICAL(&rtc_spinlock);
    return ESP_OK;
}


static void clearRxBuffer() {
    Buffer buffer;
    size_t bytesRead = 0;
    for( int i=0; i<_::BufferCount; ++i ) {
        TRACE_ESP_ERROR_CHECK(i2s_read(I2S_NUM_0, reinterpret_cast<char*>(buffer.data()),
							buffer.size(), &bytesRead, portMAX_DELAY));
    }
}


void start( const nonstd::span<const adc_channel_t>& channels ) {
	esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_0, ADC_WIDTH_BIT_12, adcVref, &adcCalCharacteristics);

	i2s_config_t i2s_config =  {
		.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
		.sample_rate = _::SampleRate,
		.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
		.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
		.communication_format = I2S_COMM_FORMAT_I2S,
		.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
		.dma_buf_count = _::BufferCount,
		.dma_buf_len = _::BufferSize,   // in samples
		.use_apll = false, 
		.tx_desc_auto_clear = true,
		.fixed_mclk = 0,
	};

    //install and start i2s driver
    #if 0
    i2s_driver_install(I2S_NUM_0, &i2s_config, 1, &i2sEventQueue);
    #else
    i2s_driver_install(I2S_NUM_0, &i2s_config, 1, NULL);
    #endif

	nonstd::span<adc_channel_t>::const_iterator it = channels.begin();
	i2s_set_adc_mode(ADC_UNIT_1, static_cast<adc1_channel_t>(*it));
	std::for_each( ++it, channels.cend(), [](adc_channel_t channel) {
		adc_gpio_init(ADC_UNIT_1, channel);
	});
    
    // The raw ADC data is written in DMA in inverted form.
	SYSCON.saradc_ctrl2.sar1_inv = 1;

    i2s_adc_enable(I2S_NUM_0);

	// i2s_adc_enable resets pattern table. So it must be set after enable it
	adc_set_i2s_data_len(ADC_UNIT_1, channels.size());
	int index = 0;
	std::for_each(channels.begin(), channels.end(), [&](adc_channel_t channel) {
		adc_set_i2s_data_pattern(ADC_UNIT_1, index, channel, ADC_WIDTH_BIT_12, ADC_ATTEN_DB_0);
		++index;
	});

    clearRxBuffer();
}


void stop() {
	i2s_adc_disable(I2S_NUM_0);
    clearRxBuffer();
    i2s_driver_uninstall(I2S_NUM_0);
}


bool receivedData() {
	system_event_t evt;
	if (xQueueReceive(i2sEventQueue, &evt, 100/portTICK_RATE_MS) == pdPASS) {
		if (evt.event_id==2) {
			return true;
		}
	}
	return false;	
}


size_t readData( Buffer& buffer ) {
	size_t bytesRead = 0;
	TRACE_ESP_ERROR_CHECK(i2s_read(I2S_NUM_0, reinterpret_cast<char*>(buffer.data()),
							buffer.size() * 2, &bytesRead, portMAX_DELAY));
	return bytesRead >> 1;
}


uint16_t rawToMilliVolts( uint16_t value ) {
	return esp_adc_cal_raw_to_voltage(value, &adcCalCharacteristics);
}

}

}