#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/portmacro.h"
#include "freertos/queue.h"

#include "wifi/wifi.h"
#include "wifi/ota.h"
#include "measurer/voltage.h"
#include "measurer/current.h"
#include "measurer/sampler.h"
#include "commands/commands.h"
#include "io/display.h"
#include "web/server.h"
#include "measurer/sampler_processor.h"
#include "model/sample.h"
#include "util/trace.h"
#include <algorithm>
#include <Arduino.h>

static const char* hostname="wattmeter";
static const adc_channel_t ZERO_ADC_CHANNEL = ADC_CHANNEL_6;

typedef sampler::Sampler<measurer::Voltage::AdcChannel, 
						measurer::Current::AdcChannel> MainSampler;
MainSampler mainSampler;
measurer::Voltage voltageMeasurer;
measurer::Current currentMeasurer;

web::Server webServer(8080);
io::Display display;


uint16_t defaultZero() {
    typedef sampler::Sampler<ZERO_ADC_CHANNEL> ZeroSampler;
    ZeroSampler zeroSampler;

	zeroSampler.start();
	uint16_t ret = zeroSampler.readAndAverage<ZERO_ADC_CHANNEL>();
	zeroSampler.stop();

	TRACE("DUT GND at %d mV", ret);

	return ret;
}


void autoRange() {
    measurer::Voltage::AutoRangeAction voltageAutoRangeAction = voltageMeasurer.autoRangeAction();
    measurer::Current::AutoRangeAction currentAutoRangeAction = currentMeasurer.autoRangeAction();
    if ( voltageAutoRangeAction || currentAutoRangeAction ) {
        mainSampler.pauseWhileAction( [&]() {
            if ( voltageAutoRangeAction ) {
                voltageAutoRangeAction();
            }
            if ( currentAutoRangeAction ) {
                currentAutoRangeAction();
            }
        } ); 
    }
}


void showInfo( const model::Samples& processedSamples ) {
    webServer.send( processedSamples );
}


void readSamples( void* ) {
    measurer::SamplerProcessor samplerProcessor(&voltageMeasurer, &currentMeasurer);
    model::Samples processedSamples;
    MainSampler::Samples samplesContainer;
    for(;;) {
        mainSampler.read( samplesContainer );
        samplerProcessor.process( samplesContainer, processedSamples );

        showInfo( processedSamples );

        autoRange();
    }
}


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

    delay(500);
	uint16_t zero = defaultZero();

	//voltageMeasurer.init( 518 );
	//currentMeasurer.init( 514 );
    voltageMeasurer.init( zero );
	currentMeasurer.init( zero );
	
    webServer.begin();

    mainSampler.start();

    TaskHandle_t readSamplesTask;
    xTaskCreate( readSamples, "readSamples", 5020, NULL, 1, &readSamplesTask );
}


void loop()
{
	ota::handle();

	if ( commands::isZerosCalibrationRequest() ) {
        mainSampler.pauseWhileAction( [&]() {
            voltageMeasurer.calibrateZeros();
            currentMeasurer.calibrateZeros();
        });
	}

	if ( commands::isFactorsCalibrationRequest() ) {
        mainSampler.pauseWhileAction( [&]() {
		    voltageMeasurer.calibrateFactors();
        });
	}
}
