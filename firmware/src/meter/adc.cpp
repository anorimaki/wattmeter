#include "util/trace.h"
#include "util/indicator.h"
#include "meter/sampler.h"
#include <esp_task.h>
#include <freertos/task.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>

namespace adc {

static const adc_bits_width_t WIDTH = ADC_WIDTH_BIT_12;
static const adc_atten_t ATTEN_DB = ADC_ATTEN_DB_0;
static const adc1_channel_t CHANNEL1 = ADC1_CHANNEL_0;
static const adc_channel_t CHANNEL = ADC_CHANNEL_0;
static const int N_OF_SAMPLES = 2048;

static const uint32_t VREF = 1108;	 //Use adc2_vref_to_gpio() to obtain a better estimate (default 1100)

static void route_vref() {
	esp_err_t status = adc2_vref_to_gpio(GPIO_NUM_25);
	if ( status != ESP_OK ) {
		TRACE_ERROR( "Failed to route v_ref: %d", status );
	}
}

static esp_adc_cal_characteristics_t adc_cal_characteristics;
uint32_t rawValues[N_OF_SAMPLES];

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

typedef meter::Sampler<CHANNEL, ADC_CHANNEL_7> Sampler;
Sampler sampler;

static void dmaRead(void *pvParameters) {
	sampler.start();
	while(1) {
		uint32_t average = sampler.readAndAverage<CHANNEL>(4);
		uint32_t voltage = esp_adc_cal_raw_to_voltage(average, &adc_cal_characteristics);
		printf("Avg: %d\t%dmV\n",  average, voltage);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

/**********************************************/
/**********************************************/

void initCharacterization() {
	adc1_config_width(WIDTH);
	adc1_config_channel_atten(CHANNEL1,ATTEN_DB);
	esp_adc_cal_value_t type = esp_adc_cal_characterize(ADC_UNIT_1,
											 ATTEN_DB, WIDTH, VREF, &adc_cal_characteristics);
	TRACE( F("Characterization type: %d"), type );
}


void init() {
	initCharacterization();
	xTaskCreate(&dmaRead, "readAdc", 4096, NULL, 5, NULL);
}

}