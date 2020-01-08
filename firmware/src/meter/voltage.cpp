#include "meter/voltage.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "nvs_flash.h"
#include "nvs.h"

namespace meter {

namespace voltage {

static const gpio_num_t HighIOPin = GPIO_NUM_12;
static const gpio_num_t LowIOPin = GPIO_NUM_14;

void init() {
    gpio_reset_pin(HighIOPin);
 	gpio_set_direction( HighIOPin, GPIO_MODE_OUTPUT );
    
    gpio_reset_pin(LowIOPin);
 	gpio_set_direction( LowIOPin, GPIO_MODE_OUTPUT );
}

void setGPIORange( size_t range ) {
	gpio_set_level( HighIOPin, !!(range & 0b10) );
	gpio_set_level( LowIOPin, !!(range & 0b01) );
}

}

}