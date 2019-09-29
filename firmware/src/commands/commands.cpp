#include "commands/commands.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#include "util/trace.h"

namespace commands {

static const gpio_num_t DO_ZEROS_CALIBRATION_PIN = GPIO_NUM_22;
static const gpio_num_t DO_FACTORS_CALIBRATION_PIN = GPIO_NUM_23;

inline unsigned long millis() {
    return (unsigned long) (esp_timer_get_time() / 1000ULL);
}

template <gpio_num_t PIN>
class Button {
private:
	static const unsigned long debounceDelay = 100; 	//ms

public:
	void init() {
		gpio_set_direction( PIN, GPIO_MODE_INPUT );
		gpio_set_pull_mode( PIN, GPIO_PULLUP_ONLY );
	}

	bool isPresed() {
		static int previousState = 0;
		static int debouncedState = 0;
		static unsigned long lastDebounceTime = 0;

		int currentState = gpio_get_level(PIN);
		if ( previousState != currentState ) {            
			lastDebounceTime = millis();
			previousState = currentState;
		}

		bool changed = false;
		if ((millis() - lastDebounceTime) > debounceDelay) {
			changed = (debouncedState != currentState);
			debouncedState = currentState;
		}

		return changed && (currentState == 0);
	}
};

static Button<DO_ZEROS_CALIBRATION_PIN> doZerosCalibrationButton;
static Button<DO_FACTORS_CALIBRATION_PIN> doFactorsCalibrationButton;

void init() {
	doZerosCalibrationButton.init();
	doFactorsCalibrationButton.init();
}

bool isZerosCalibrationRequest() {
	return doZerosCalibrationButton.isPresed();
}

bool isFactorsCalibrationRequest() {
	return doFactorsCalibrationButton.isPresed();
}

}