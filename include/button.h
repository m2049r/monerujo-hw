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

#ifndef __BUTTON_H__
#define __BUTTON_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef void (*button_callback)(void);
#define DEBOUNCE_DELAY 20

typedef struct {
	uint32_t gpioport;
	button_callback pressed;
	button_callback released;
	uint32_t lastTime;
	uint16_t gpios;
	bool lastValue;
	bool executed;
} button;

void button_setup(button *aButton, uint32_t gpioport, uint16_t gpios);

void button_poll(button* aButton);

#endif
