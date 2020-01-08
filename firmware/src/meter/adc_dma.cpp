#include "meter/adc.h"
#include "util/trace.h"
#include "driver/i2s.h"
#include "soc/syscon_reg.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

extern "C" {
#include "soc/syscon_struct.h"
//#include "soc/rtc_io_struct.h"
//#include "soc/sens_struct.h"
//#include "soc/sens_reg.h"
}

extern portMUX_TYPE rtc_spinlock;

namespace adc {

namespace dma {

// Copied from ESP-IDF rtc_module.c
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

// Copied from ESP-IDF rtc_module.c
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
    for( int i=0; i<adc::BufferCount*2; ++i ) {
        TRACE_ESP_ERROR_CHECK(i2s_read(I2S_NUM_0, reinterpret_cast<char*>(buffer.data()),
							buffer.size(), &bytesRead, portMAX_DELAY));
    }
}


void start( const nonstd::span<const adc1_channel_t>& channels ) {
	i2s_config_t i2s_config =  {
		.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
		.sample_rate = adc::SampleRate,
		.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
		.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
		.communication_format = I2S_COMM_FORMAT_I2S,
		.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
		.dma_buf_count = adc::BufferCount,
		.dma_buf_len = adc::BufferSize,   // in samples
		.use_apll = false, 
		.tx_desc_auto_clear = true,
		.fixed_mclk = 0,
	};

    //install and start i2s driver
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);

	nonstd::span<adc1_channel_t>::const_iterator it = channels.begin();
	i2s_set_adc_mode(ADC_UNIT_1, static_cast<adc1_channel_t>(*it));
	std::for_each( ++it, channels.cend(), [](adc1_channel_t channel) {
		adc_gpio_init(ADC_UNIT_1, (adc_channel_t)channel);
	});
    
    i2s_adc_enable(I2S_NUM_0);

    portENTER_CRITICAL(&rtc_spinlock);

    // The raw ADC data is written in DMA in inverted form.
	SYSCON.saradc_ctrl2.sar1_inv = 1;

	// i2s_adc_enable resets pattern table. So it must be set after enable it
	adc_set_i2s_data_len(ADC_UNIT_1, channels.size());
	int index = 0;
	std::for_each(channels.begin(), channels.end(), [&](adc1_channel_t channel) {
		adc_set_i2s_data_pattern(ADC_UNIT_1, index, (adc_channel_t)channel,
                                ADC_WIDTH_BIT_12, ADC_ATTEN_DB_0);
		++index;
	});

    portEXIT_CRITICAL(&rtc_spinlock);

    clearRxBuffer();
}


void stop() {
	i2s_adc_disable(I2S_NUM_0);
    clearRxBuffer();
   // i2s_stop(I2S_NUM_0);
    i2s_driver_uninstall(I2S_NUM_0);
}


int64_t readData( Buffer& buffer ) {
    static const size_t BufferBytes  = adc::BufferSize * sizeof(uint16_t);
	size_t bytesRead = 0;
	TRACE_ESP_ERROR_CHECK(i2s_read( I2S_NUM_0, 
                                    reinterpret_cast<char*>(buffer.data()),
							        BufferBytes, 
                                    &bytesRead, 
                                    portMAX_DELAY ));
    assert( bytesRead == BufferBytes );
	return esp_timer_get_time() - (1000000ULL * adc::BufferSize / SampleRate);
}

}

}