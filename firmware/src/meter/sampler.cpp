#include "meter/sampler.h"
#include "util/trace.h"

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "driver/gpio.h"
#include "driver/dac.h"
#include "soc/rtc.h"
#include "esp_event_loop.h"
#include "esp_adc_cal.h"

#include <algorithm>
#include <iterator>

#define USE_DMA


namespace meter {

static bool adcToVoltageInitialized = false;
static uint16_t adcToVoltage[4096];
static uint32_t adcVref = 1108;			// default value. Value in mV


namespace _ {

static bool adcCharacterized = false;
static esp_adc_cal_characteristics_t adcCalCharacteristics;

uint16_t rawToMilliVolts( uint16_t value ) {
    if ( !adcCharacterized ) {
        esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_0, 
                                    ADC_WIDTH_BIT_12, adcVref, &adcCalCharacteristics);
        adcCharacterized = true;
    }
	return esp_adc_cal_raw_to_voltage(value, &adcCalCharacteristics);
}

uint16_t rawToTenthsOfMilliVolt( uint16_t value ) {
    return adcToVoltageInitialized ? adcToVoltage[value] : rawToMilliVolts(value)*10;
}

}


void setAdcVref( uint16_t value ) {
	adcVref = value;
}

void routeVref( gpio_num_t gpio ) {
	TRACE_ESP_ERROR_CHECK( adc2_vref_to_gpio(gpio) );
}


static uint16_t rawRead( adc1_channel_t channel ) {
    static const int N_BUFFERS = 1;
    uint32_t total = 0;
    for( int i=0; i<N_BUFFERS; ++i ) {
        adc::Buffer values;
        adc::ADC_IMPL::readData( values );
        total = std::accumulate( values.begin(), values.end(), total, 
                            [&](uint32_t total, uint16_t sample) {
            uint16_t sampleChannel = sample >> 12;
            uint16_t sampleValue = sample & 0xFFF;
            if ( sampleChannel != channel ) {
                TRACE( "Data found for unknown channel: %d", sampleChannel );
            }
            return total + sampleValue;
        });
    }
    return total / (adc::BufferSize*N_BUFFERS);
}

static void interpolate( uint16_t* table, uint16_t valueAfter ) {
    uint16_t valueBefore = *table;
    uint16_t diff = valueAfter - valueBefore;
    if ( valueAfter < valueBefore ) {
        TRACE("Read value is lower than reading of previous voltage: %u > %u", 
                valueBefore, valueAfter);
    }
    for( int i=1; i<16; ++i ) {
       table[i] = valueBefore + ((diff * i) >> 4);
    }
}

static void buildDacToAdcMapping( dac_channel_t dac, adc1_channel_t adc, uint16_t* table ) {
    dac_output_enable(dac);
    adc::ADC_IMPL::start( nonstd::span<const adc1_channel_t>( &adc, 1 ) );

    for( int i=0; i<256; ++i ) {
        dac_output_voltage(dac, i);
        vTaskDelay( 1 / portTICK_PERIOD_MS );

        table[16] = rawRead(adc);
        interpolate( table, table[16] );

        table += 16;
    }

    adc::ADC_IMPL::stop();
    interpolate( table, 4096 );
    dac_output_voltage(DAC_CHANNEL_1, 0);
}

__attribute__((used))
static void traceTable( const uint16_t* table ) {
    for (int i=0; i<4096; i+=16) {
        Serial.printf( "%04u:", i );
        for( int j=i; j<i+16; ++j ) {
            Serial.printf(" %05u", table[j]);
        }
        Serial.printf( "\n" );
    }
}

static void buildAdcToDacMapping( const uint16_t* dacToAdc, uint16_t* adcToDac ) {
  /*  uint16_t voltage = 0;
    uint16_t diff = 9999;
    for( int i=0; i<4096; ++i ) {
        uint16_t newDiff = 0;
        while( (voltage<4096) && (newDiff<=diff) ) {
            newDiff = abs( i - readings[voltage++] );
        }
        adcLut[i] = voltage;
    } */

    for( int i=0; i<4096; ++i ) {
        uint16_t voltage = 0;
        uint16_t diff = 9999;
        for( int j=0; j<4096; ++j ) {
            uint16_t newDiff = abs( i - dacToAdc[j] );
            if ( newDiff<diff ) {
                diff = newDiff;
                voltage = j;
            }
        }
        adcToDac[i] = voltage;
    } 
}


// If you use mV as unit as the esp-idf esp_adc_cal_raw_to_voltage function does, you will loose
// more than 2 bits of resolution. With mV as unit, we have 900 possible values (>10 bits) in the
// range of 100mV to 1V.
// To avoid this loss of resolution, we use tenths of mV as unit. We really can't get that
// resolution (we have only 12 bits) but it is more human redeable to apply a factor of 10 than 
// a factor of 4 and we have enough room in 32 bits for overflows to occur.
static void dacValuesToVoltage( uint16_t* table, 
                                adc1_channel_t adcHighRefChannel,
                                adc1_channel_t adcLowRefChannel ) {
    static const uint16_t HighRefVoltage = 9590;         // In tenths of mV
    static const uint16_t LowRefVoltage = 1299;          // In tenths of mV

    ets_delay_us( 100 );
    adc::ADC_IMPL::start( nonstd::span<const adc1_channel_t>( &adcHighRefChannel, 1 ) );
    uint16_t highRefAdcValue = rawRead(adcHighRefChannel);
    adc::ADC_IMPL::stop();
    adc::ADC_IMPL::start( nonstd::span<const adc1_channel_t>( &adcLowRefChannel, 1 ) );
    uint16_t lowRefAdcValue = rawRead(adcLowRefChannel);
    adc::ADC_IMPL::stop();

    uint16_t highRefDacValue = table[highRefAdcValue];
    uint16_t lowRefDacValue = table[lowRefAdcValue];

    uint32_t deltaX = highRefDacValue - lowRefDacValue;
    const static uint32_t deltaY = HighRefVoltage - LowRefVoltage;

    // Point-slope equation: y = y0 + m(x-x0)
    // Where:
    //      m = deltaY / deltaX
    //      y0 = HighRefVoltage
    //      x0 = highRefDacValue
    // For x = 0  -->  y = HighRefVoltage - (deltaY/deltaX) * highRefDacValue
    // We apply an extra +(deltaX/2) before divide by deltaX for rounding.
    const uint16_t dacOffsetVoltage =       
            HighRefVoltage - ( ((deltaY * highRefDacValue) + (deltaX>>1)) / deltaX );

    TRACE("DAC offset: %.1f mV", dacOffsetVoltage / 10.0);
    
    for ( int i=0; i<4096; ++i ) {
        uint32_t v = table[i];
        table[i] = (((v * deltaY) + (deltaX>>1)) / deltaX) + dacOffsetVoltage;
    }
}

// Indexes are DAC values and values are the corresponding ADC value
static uint16_t dacToAdc[4096];

void characterizeAdc( dac_channel_t dac, adc1_channel_t adc, 
                    adc1_channel_t adcHighRef, adc1_channel_t adcLowRef ) {
    TRACE("Characterizing ADC voltage..."); 

    buildDacToAdcMapping( dac, adc, dacToAdc );
    TRACE("DAC value to ADC value table:"); 
 //   traceTable( dacToAdc );
    
    buildAdcToDacMapping( dacToAdc, adcToVoltage );
    TRACE("ADC value to DAC value table:"); 
 //   traceTable( adcToVoltage );

    dacValuesToVoltage( adcToVoltage, adcHighRef, adcLowRef );
    TRACE("ADC value to voltage conversion table (tenths of mV):"); 
 //   traceTable( adcToVoltage );

    adcToVoltageInitialized = true;
}


}