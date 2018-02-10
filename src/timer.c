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

#include "timer.h"
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>

void timer_init(void) {
	systick_set_frequency(1000, 120000000);
	systick_clear();
	systick_counter_enable();
	systick_interrupt_enable();
}

static volatile uint32_t ticks;

void sys_tick_handler() {
	ticks++;
}

// return the system clock as milliseconds
uint32_t millis() {
	return ticks;
}

void delay_ms(uint32_t t) {
	uint32_t start, end;
	start = millis();
	end = start + t;
	if (start < end) {
		while ((millis() >= start) && (millis() < end))
			;
	} else {
		while ((millis() >= start) || (millis() < end))
			;
	}
}
