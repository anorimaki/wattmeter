#if 0

#include "util/trace.h"
#include "util/indicator.h"
#include "meter/sampler.h"
#include "commands/commands.h"
#include <esp_task.h>
#include <freertos/task.h>
#include <driver/adc.h>
#include <driver/i2s.h>
#include <esp_adc_cal.h>
#include <soc/sens_struct.h>
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

extern "C" {
#include "soc/syscon_struct.h"
//#include "soc/rtc_io_struct.h"
//#include "soc/sens_struct.h"
//#include "soc/sens_reg.h"
}

#define SUPPRESS_UNUSED_WARNING __attribute__((used))


extern portMUX_TYPE rtc_spinlock;

namespace experiment {

static const adc_bits_width_t WIDTH = ADC_WIDTH_BIT_12;
static const adc_atten_t ATTEN_DB = ADC_ATTEN_DB_0;
static const adc1_channel_t CHANNEL1 = ADC1_CHANNEL_0;
static const adc_channel_t CHANNEL = ADC_CHANNEL_0;
static const int N_OF_SAMPLES = 2048;

static const uint32_t VREF = 1108;	 //Use adc2_vref_to_gpio() to obtain a better estimate (default 1100)

SUPPRESS_UNUSED_WARNING
static void route_vref() {
	esp_err_t status = adc2_vref_to_gpio(GPIO_NUM_25);
	if ( status != ESP_OK ) {
		TRACE_ERROR( "Failed to route v_ref: %d", status );
	}
}

static esp_adc_cal_characteristics_t adc_cal_characteristics;
uint32_t rawValues[N_OF_SAMPLES];

SUPPRESS_UNUSED_WARNING
static void directRead(void *pvParameters) {
	Indicator indicator(GPIO_NUM_27);
	while(1) {
		uint32_t sum = 0;
		int64_t t1 = esp_timer_get_time();
		indicator.hight();
		for ( int i=0; i<N_OF_SAMPLES; ++i ) {
			rawValues[i] = adc1_get_raw(CHANNEL1); 
			sum += rawValues[i];
		}
		indicator.low();
		int64_t interval = esp_timer_get_time() - t1;

		uint32_t average = sum / N_OF_SAMPLES;

		uint32_t maxValue = 0;
		uint32_t minValue = 4096;
		uint32_t sqrSum = 0;
		for ( int i=0; i<N_OF_SAMPLES; ++i ) {
			int32_t diff = rawValues[i] - average;
			sqrSum += (diff*diff);
			if ( maxValue < rawValues[i] ) {
				maxValue = rawValues[i];
			}
			if ( minValue > rawValues[i] ) {
				minValue = rawValues[i];
			}
		}
		
		sqrSum /= N_OF_SAMPLES;
		uint32_t voltage = esp_adc_cal_raw_to_voltage(average, &adc_cal_characteristics);
		printf("Avg: %d\t%dmV\tMax: %d\tMin: %d\tD: %d\tV: T: %lldus\n", 
				average, voltage, maxValue, minValue, sqrSum,  interval);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}


/**********************************************/
/**********************************************/

typedef meter::Sampler<ADC1_CHANNEL_0, ADC1_CHANNEL_7> Sampler;
Sampler sampler;

SUPPRESS_UNUSED_WARNING
static void samplerRead(void *pvParameters) {
    while(1) {
        uint32_t average = sampler.readAndAverage<ADC1_CHANNEL_7>(4);
		uint32_t voltage = esp_adc_cal_raw_to_voltage(average, &adc_cal_characteristics);
		printf("Avg: %d\t%dmV\n",  average, voltage);
		vTaskDelay(pdMS_TO_TICKS(10000));
	}
}

static uint16_t readAdc( adc1_channel_t channel ) {
    adc::Buffer values;
    adc::readData( values );
    uint32_t total = 0;
    int samples = 0;
    for( int i=0; i<adc::BufferSize; ++i ) {
        uint16_t sample = values[i];
        uint16_t sampleChannel = sample >> 12;
        uint16_t sampleValue = sample & 0xFFF;
        if ( sampleChannel == channel ) {
            total += sampleValue;
            ++samples;
        }
    }
    return (samples==0) ? -1 : total/samples;
}

#if 1
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

void start2( const nonstd::span<const adc1_channel_t>& channels ) {
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
                                ADC_WIDTH_BIT_12, ADC_ATTEN_DB_2_5);
		++index;
	});

    portEXIT_CRITICAL(&rtc_spinlock);
}
#endif

static void dacSeriesDMARead(void *pvParameters) {
    static const uint8_t values[] = {91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102};

    gpio_set_pull_mode( GPIO_NUM_25, GPIO_FLOATING );
    dac_output_enable(DAC_CHANNEL_1);
    dac_output_voltage(DAC_CHANNEL_1, 0);

    adc1_channel_t adcChannel = ADC1_CHANNEL_4;
    start2( nonstd::span<adc1_channel_t>(&adcChannel, 1) );

    int index = 0;
    while(1) {
        uint8_t value = values[index++];
        if ( index == sizeof(values)/sizeof(uint8_t) ) {
            index = 0;
        }
        dac_output_voltage(DAC_CHANNEL_1, value);
        vTaskDelay( 100 / portTICK_PERIOD_MS );

        uint16_t adcValue = readAdc(adcChannel);
        Serial.printf( "%03u: %u\n", value, adcValue );
	}
}

inline uint16_t local_adc1_read(int channel) {
    Indicator ii(GPIO_NUM_17);
    ii.hight();
    uint16_t adc_value;
    SENS.sar_meas_start1.sar1_en_pad = (1 << channel); // only one channel is selected
    while (SENS.sar_slave_addr1.meas_status != 0);
    SENS.sar_meas_start1.meas1_start_sar = 0;
    SENS.sar_meas_start1.meas1_start_sar = 1;
    while (SENS.sar_meas_start1.meas1_done_sar == 0);
    adc_value = SENS.sar_meas_start1.meas1_data_sar;
    ii.low();
    return adc_value;
}

static uint16_t directReadAdc( adc1_channel_t channel ) {
    static const int N_SAMPLES = 64;
    uint32_t total = 0;
    for( int i=0; i<N_SAMPLES; ++i ) {
        total += local_adc1_read(channel);
    }
    return total / N_SAMPLES;
}


static void dacSeriesDirectRead(void *pvParameters) {
    static const uint8_t values[] = {91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102};
    int index = 0;

    gpio_set_pull_mode( GPIO_NUM_25, GPIO_FLOATING );
    dac_output_enable(DAC_CHANNEL_1);
    dac_output_voltage(DAC_CHANNEL_1, 0);

    adc1_config_width(ADC_WIDTH_BIT_12);
	adc1_config_channel_atten(ADC1_CHANNEL_4,ADC_ATTEN_DB_0);
    adc1_get_raw(ADC1_CHANNEL_4);

    while(1) {
        uint8_t value = values[index++];
        if ( index == sizeof(values)/sizeof(uint8_t) ) {
            index = 0;
        }
        dac_output_voltage(DAC_CHANNEL_1, value);
        vTaskDelay( 100 / portTICK_PERIOD_MS );

        TRACE_TIME_INTERVAL_BEGIN(aa);
        uint16_t adcValue = directReadAdc(ADC1_CHANNEL_4);
        TRACE_TIME_INTERVAL_END(aa);
        TRACE( "DAC value: %03u, ADC -> %u", value, adcValue );
        
        vTaskDelay( 100 / portTICK_PERIOD_MS );
	}
}

/**********************************************/
/**********************************************/

/**********************************************/
/**********************************************/

void initCharacterization() {
	adc1_config_width(WIDTH);
	adc1_config_channel_atten(ADC1_CHANNEL_5,ATTEN_DB);
	esp_adc_cal_value_t type = esp_adc_cal_characterize(ADC_UNIT_1,
											 ATTEN_DB, WIDTH, VREF, &adc_cal_characteristics);
	TRACE( F("Characterization type: %d"), type );
}


void init() {
	initCharacterization();
  //  xTaskCreate(&dmaReadDMA, "readAdcDMA", 4096*2, NULL, 5, NULL);
    xTaskCreate(&dacSeriesDMARead, "dacSeriesDMARead", 4096*2, NULL, 5, NULL);
    xTaskCreate(&dacSeriesDirectRead, "dacSeriesDirectRead", 4096*2, NULL, 5, NULL);
  //  xTaskCreate(&readSimple, "readSimple", 4096*2, NULL, 5, NULL);
   
}

}

#endif