#include "meter/current.h"

namespace meter {

namespace current {

BaseMeter::CalibrationData readClibrationData() {
	return {
		.zeros = { 0, 0, 0 }
	};
}

static const std::array<gpio_num_t, N_RANGES> Pins = { GPIO_NUM_5, GPIO_NUM_18, GPIO_NUM_19 };

void init() {
	std::for_each( Pins.begin(), Pins.end(), [](gpio_num_t pin) {
		gpio_set_direction( pin, GPIO_MODE_OUTPUT );
	});
}

void setGPIORange( size_t range ) {
	std::for_each( Pins.begin(), Pins.end(), [](gpio_num_t pin) {
		gpio_set_level( pin, 0 );
	});
	gpio_set_level( Pins[range], 1 );
}

}

}