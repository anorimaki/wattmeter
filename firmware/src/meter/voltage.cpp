#include "meter/voltage.h"
#include "driver/gpio.h"

namespace meter {

namespace voltage {

CalibrationData readClibrationData() {
	return {
		.zeros = { 0, 0, 0 }
	};
}

static const gpio_num_t HighIOPin = GPIO_NUM_33;
static const gpio_num_t LowIOPin = GPIO_NUM_32;

void init() {
	gpio_set_direction( HighIOPin, GPIO_MODE_OUTPUT );
	gpio_set_direction( LowIOPin, GPIO_MODE_OUTPUT );
}

void setGPIORange( size_t range ) {
	gpio_set_level( HighIOPin, !!(range & 0b10) );
	gpio_set_level( LowIOPin, !!(range & 0b01) );
}

}

}