/*
 * This file is part of monerujo-hw
 *
 * Copyright (C) 2018 m2049r <m2049r@monerujo.io>
 * Based on https://github.com/trezor/trezor-mcu/blob/2391beb6f43f90bcb78e1a8689334c30137da43f/gen/fonts.h
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

#ifndef __FONTS_H__
#define __FONTS_H__

#include <stdint.h>
#define FONT_MONERO "\x10"
#define FONT_UP   "\x8b"
#define FONT_DOWN "\x8a"
#define FONT_LEFT "\x88"
#define FONT_RIGHT "\x95"
#define FONT_COPYRIGHT "\xa9"

#define FONT_HEIGHT 8

extern const uint8_t * const font_data[256];

int fontCharWidth(char c);
const uint8_t *fontCharData(char c);

#endif
