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

#ifndef __TIMER_H__
#define __TIMER_H__

#include <stdint.h>

void timer_init(void);

uint32_t millis(void);

void systick_handler(void);

#endif
