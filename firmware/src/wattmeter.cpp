#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/portmacro.h"
#include "freertos/queue.h"

#include "wifi/wifi.h"
#include "wifi/ota.h"

#include "commands/commands.h"
#include "io/display.h"
#include "web/server.h"
#include "meter/sampledmeter.h"
#include "meter/calculatedmeter.h"
#include "util/trace.h"
#include <algorithm>
#include <Arduino.h>

static const char* hostname="wattmeter";
static const adc_channel_t ZERO_ADC_CHANNEL = ADC_CHANNEL_6;

meter::SampleBasedMeter sampledMeter;
meter::CalculatorBasedMeter calculatedMeter;

web::Server webServer(8080);
io::Display display;


uint16_t defaultZero() {
    typedef meter::Sampler<ZERO_ADC_CHANNEL> ZeroSampler;
    ZeroSampler zeroSampler;

	zeroSampler.start();
	uint16_t ret = zeroSampler.readAndAverage<ZERO_ADC_CHANNEL>();
	zeroSampler.stop();

	TRACE("DUT GND at %d mV", ret);

	return ret;
}


void showInfo( void* ) {
    while(!ota::inProgress) {
TRACE_TIME_INTERVAL_BEGIN(update);
        display.update( calculatedMeter.get() );
TRACE_TIME_INTERVAL_END(update);
    }
    TRACE( "showInfo task finished" );
    vTaskDelete(NULL);
}


void readSamples( void* ) {
    meter::SampleBasedMeter::Measures sampledMeasures;

    sampledMeter.start();
    std::pair<float, float> scaleFactors = sampledMeter.scaleFactors();
    calculatedMeter.scaleFactors( scaleFactors );

    while(!ota::inProgress) {
trace::traceTimeInterval("read"); 
        uint64_t time = sampledMeter.read( sampledMeasures );

TRACE_TIME_INTERVAL_BEGIN(readOp);
        webServer.send( time, scaleFactors, sampledMeasures );
        if ( calculatedMeter.process( time, sampledMeasures ) ) {
            if ( sampledMeter.autoRange() ) {
                scaleFactors = sampledMeter.scaleFactors();
                calculatedMeter.scaleFactors( scaleFactors );
            }
        }
TRACE_TIME_INTERVAL_END(readOp);
    }
    TRACE( "readSamples task finished" );
    vTaskDelete(NULL);
}

TaskHandle_t readSamplesTask;
void setup()
{
	Serial.begin(115200);

    esp_log_level_set("*", ESP_LOG_VERBOSE);

#ifdef DEBUG_ESP_CORE
	Serial.setDebugOutput(true);
#endif

    display.init(); 

	wifi::init(hostname);
	ota::init(hostname);

    commands::init();

	uint16_t zero = defaultZero();
    sampledMeter.init( zero );
	
    delay(500);
    webServer.begin();

    TaskHandle_t readSamplesTask;
    xTaskCreatePinnedToCore( readSamples, "readSamples", 7168, NULL, 1, &readSamplesTask, 1 );

    TaskHandle_t showInfoTask;
    xTaskCreatePinnedToCore( showInfo, "showInfo", 2048, NULL, 1, &showInfoTask, 0 );
}


void loop()
{
#if 0
    trace::traceTimeInterval( "loop" );

    UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark(readSamplesTask);
    printf( "Stack w: %u\n", uxHighWaterMark );
#endif

    ota::handle();

	if ( commands::isZerosCalibrationRequest() ) {
        sampledMeter.calibrateZeros();
	}

	if ( commands::isFactorsCalibrationRequest() ) {
        sampledMeter.calibrateFactors();
	}

    vTaskDelay( 10 / portTICK_PERIOD_MS );
}

