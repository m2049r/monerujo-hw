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

#include <string.h>
#include "util.h"

// empirical magic number
#define DELAY_FACTOR (HSE_VALUE/270000)

inline void delay(uint32_t wait)
{
	uint32_t t = DELAY_FACTOR * wait;
	while (--t > 0) __asm__("nop");
}

#define FROMHEX_MAXLEN 70
const uint8_t *fromhexBE(const char *str)
{
	static uint8_t buf[FROMHEX_MAXLEN];
	size_t len = strlen(str) / 2;
	if (len > FROMHEX_MAXLEN) len = FROMHEX_MAXLEN;
	for (size_t i = 0; i < len; i++) {
		int c = 0;
		if (str[i * 2] >= '0' && str[i*2] <= '9') c += (str[i * 2] - '0') << 4;
		if ((str[i * 2] & ~0x20) >= 'A' && (str[i*2] & ~0x20) <= 'F') c += (10 + (str[i * 2] & ~0x20) - 'A') << 4;
		if (str[i * 2 + 1] >= '0' && str[i * 2 + 1] <= '9') c += (str[i * 2 + 1] - '0');
		if ((str[i * 2 + 1] & ~0x20) >= 'A' && (str[i * 2 + 1] & ~0x20) <= 'F') c += (10 + (str[i * 2 + 1] & ~0x20) - 'A');
		buf[len-i-1] = (uint8_t) c;
	}
	return buf;
}

#define TOHEX_MAXLEN (2*FROMHEX_MAXLEN)
const char *tohexBE(uint8_t * in, size_t inlen)
{
	static char buf[TOHEX_MAXLEN+1];
    const char * hex = "0123456789abcdef";
	size_t len = inlen * 2;
	if (len > TOHEX_MAXLEN) len = TOHEX_MAXLEN;
	for (size_t i = 0; i < len/2; i++) {
        buf[len-1-2*i-1] = hex[(in[i]>>4) & 0xF];
        buf[len-1-2*i-0] = hex[ in[i]     & 0xF];
    }
    buf[len] = 0;
    return buf;
}

const uint8_t *fromhexLE(const char *str)
{
	static uint8_t buf[FROMHEX_MAXLEN];
	size_t len = strlen(str) / 2;
	if (len > FROMHEX_MAXLEN) len = FROMHEX_MAXLEN;
	for (size_t i = 0; i < len; i++) {
		int c = 0;
		if (str[i * 2] >= '0' && str[i*2] <= '9') c += (str[i * 2] - '0') << 4;
		if ((str[i * 2] & ~0x20) >= 'A' && (str[i*2] & ~0x20) <= 'F') c += (10 + (str[i * 2] & ~0x20) - 'A') << 4;
		if (str[i * 2 + 1] >= '0' && str[i * 2 + 1] <= '9') c += (str[i * 2 + 1] - '0');
		if ((str[i * 2 + 1] & ~0x20) >= 'A' && (str[i * 2 + 1] & ~0x20) <= 'F') c += (10 + (str[i * 2 + 1] & ~0x20) - 'A');
		buf[i] = (uint8_t) c;
	}
	return buf;
}

const char *tohexLE(uint8_t * in, size_t inlen)
{
	static char buf[TOHEX_MAXLEN+1];
    const char * hex = "0123456789abcdef";
	size_t len = inlen * 2;
	if (len > TOHEX_MAXLEN) len = TOHEX_MAXLEN;
	for (size_t i = 0; i < len/2; i++) {
        buf[2*i+0] = hex[(in[i]>>4) & 0xF];
        buf[2*i+1] = hex[ in[i]     & 0xF];
    }
    buf[len] = 0;
    return buf;
}

void __attribute__((noreturn)) system_halt(void)
{
	for (;;) {} // loop forever
}

void __attribute__((noreturn)) system_reset(void)
{
	scb_reset_system();
}
