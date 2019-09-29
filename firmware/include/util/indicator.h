#ifndef INDICATOR_H
#define INDICATOR_H

#include <driver/gpio.h>

class Indicator {
public: 
	Indicator( gpio_num_t gpio_num ): m_num(gpio_num) {
		gpio_pad_select_gpio(gpio_num);
		gpio_set_direction(gpio_num, GPIO_MODE_OUTPUT);
	}

	void low() {
		gpio_set_level(m_num, 0);
	}

	void hight() {
		gpio_set_level(m_num, 1);
	}
private:
	gpio_num_t m_num;
};

#endif