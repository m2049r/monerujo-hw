/*
 * This file is part of monerujo-hw
 *
 * Copyright (C) 2018 m2049r <m2049r@monerujo.io>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "button.h"
#include "libopencm3/stm32/gpio.h"
#include "libopencm3/cm3/systick.h"
#include "timer.h"

void button_setup(button *aButton, uint32_t gpioport, uint16_t gpios) {
	aButton->gpioport = gpioport;
	aButton->gpios = gpios;
	aButton->pressed = NULL;
	aButton->released = NULL;
	aButton->lastTime = 0xffffffff;
	aButton->lastValue = false;
	aButton->executed = true;
}

void button_poll(button* aButton) {
	uint32_t now = millis();
	bool value = !gpio_get(aButton->gpioport, aButton->gpios);

	// check if input value changed
	if (aButton->lastValue != value) {
		aButton->lastValue = value;
		aButton->lastTime = now;
		aButton->executed = false;
		return;
	}

	if (aButton->executed)
		return;

	// wait debouncing time
	if (now - aButton->lastTime > DEBOUNCE_DELAY) {
		aButton->executed = true;
		if (value) {
			if (aButton->pressed)
				aButton->pressed();
		} else {
			if (aButton->released)
				aButton->released();
		}
	}
}
