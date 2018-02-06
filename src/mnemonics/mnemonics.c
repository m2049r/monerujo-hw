/*
 * This file is part of monerujo-hw
 *
 * Copyright (C) 2018 i_a, m2049r <m2049r@monerujo.io>
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
#include <inttypes.h>
#include <libopencm3/stm32/crc.h>

#include "mnemonics.h"
#include "english.h"

// TODO: support other seed languages - beware of utf8 coding for crc
#define wordlist_electrum wordlist_electrum_english

// CRC code which works adapted from https://community.st.com/thread/18626
static uint32_t revbit(uint32_t data) {
	asm("rbit r0,r0");
	return data;
}

static uint32_t calc_crc32(const char *buffer, uint32_t size) {
	// the onboard CRC circuit can only do buffer sizes in multiples of 32 bits / 4 bytes
	if ((size & 3) != 0)
		return 0;

	crc_reset();

	uint32_t i = size >> 2;
	uint32_t ui32 = 0;
	while (i--) {
		ui32 = *((uint32_t *) buffer);
		buffer += 4;
		ui32 = revbit(ui32); //reverse the bit order of input data
		ui32 = crc_calculate(ui32);
	}

	ui32 = revbit(ui32); //reverse the bit order of output data
	ui32 ^= 0xffffffff; //xor with 0xffffffff

	return ui32; //now the output is compatible with windows/winzip/winrar
}

bool bytes_to_words(uint8_t *key, uint8_t len, uint32_t *mnemonic_seed) {
	if (len != 32)
		return false; // we can only deal with 32 byte keys here (hardcoded values below)

	uint16_t word_list_length = sizeof(wordlist_electrum) / sizeof(wordlist_electrum[0]);
	if (word_list_length != 1626) {
		mnemonic_seed[0] = word_list_length;
		return false; // we can only do base 1626
	}

	// 4 bytes -> 3 words.  8 digits base 16 -> 3 digits base 1626
	for (int8_t i = 0; i < len / 4; i++) {
		uint32_t val;        // each iteration are 4 chars from hexadecimal string
		memcpy(&val, key + (i * 4), 4);

		uint32_t w1, w2, w3; //three words
		w1 = val % word_list_length;
		w2 = ((val / word_list_length) + w1) % word_list_length;
		w3 = (((val / word_list_length) / word_list_length) + w2) % word_list_length;

		mnemonic_seed[3 * i] = w1;
		mnemonic_seed[3 * i + 1] = w2;
		mnemonic_seed[3 * i + 2] = w3;
	}

	// now, find the checkword
	// create the string for the CRC-32 (for english seeds - concat first 3 chars of every seed word)
	char checksum_buffer[3 * 24];
	for (int i = 0; i < 24; i++) {
		strncpy(checksum_buffer + (3 * i), wordlist_electrum[mnemonic_seed[i]], 3);
	}

	uint32_t crc32 = calc_crc32(checksum_buffer, 3 * 24);
	mnemonic_seed[24] = mnemonic_seed[crc32 % 24];

	return true;
}

const char* mnemonic_word(uint32_t word_idx) {
	// TODO check word_idx constraints (0, 1626)
	return wordlist_electrum[word_idx];
}
