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
    for(;;)  {
        display.mainView( calculatedMeter.fetch() );
        delay(300);
    }
}


void readSamples( void* ) {
    meter::SampleBasedMeter::Measures sampledMeasures;

    sampledMeter.start();

    for(;;) {
        sampledMeter.read( sampledMeasures );

        webServer.send( sampledMeasures );
        calculatedMeter.process( sampledMeasures );
    }
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
    xTaskCreate( readSamples, "readSamples", 7168, NULL, 3, &readSamplesTask );

    TaskHandle_t showInfoTask;
    xTaskCreate( showInfo, "showInfo", 2048, NULL, 1, &showInfoTask );
}


void loop()
{
#if 0
    trace::timeInterval( "loop" );

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
}

