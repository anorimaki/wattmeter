#ifndef INDICATOR_H
#define INDICATOR_H

#include <driver/gpio.h>

class Indicator {
public: 
	Indicator( gpio_num_t gpio_num ): m_num(gpio_num) {
        gpio_reset_pin(gpio_num);
		gpio_pad_select_gpio(gpio_num);
		gpio_set_direction(gpio_num, GPIO_MODE_OUTPUT);
        low();
	}

    void toggle() {
        if ( on ) {
            low();
        }
        else {
            hight();
        }
    }

	void low() {
		gpio_set_level(m_num, 0);
        on = false;
	}

	void hight() {
		gpio_set_level(m_num, 1);
        on = true;
	}

private:
    bool on;
	gpio_num_t m_num;
};

#endif