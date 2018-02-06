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

#include <libopencm3/stm32/gpio.h>
#include "bitmaps.h"
#include "display.h"
#include "setup.h"
#include "usb.h"
#include "util.h"
#include <string.h>
#include <stddef.h>
#include "crypto/sha3.h"
#include "crypto.h"
#include "libopencm3/stm32/rng.h"

//#include "wordlist.h"

#define fromhex(a) fromhexLE(a)
#define tohex(a,b) tohexLE(a,b)

#define KEYSIZE 32
#define PUBSIZE (1+2*KEYSIZE+4)

static void generateWallet(void) {
	uint8_t spendkey[KEYSIZE];
	uint8_t viewkey[KEYSIZE];



	uint32_t rng_seed[8];
	for (int i = 0; i < 8; i++) {
		rng_seed[i] = rng_get_random_blocking();
	}

	uint8_t *seed = (uint8_t*) rng_seed;//why this is uint8_t when it is pointing to uint32_t?
										//it is "bits & memory & array" stuff?

	// for testing, override rng:  to fit example here:
	// https://monero.stackexchange.com/questions/874/what-is-the-checksum-at-the-end-of-a-mnemonic-seed
	memcpy(seed, fromhex("2f5f26c8d75bc043eeec32c813aa3e5ec426256e8daef38a1cf91040dab44b0f"), 32);

	usb_write("\r\nSeed:     ");
	usb_write(tohex(seed, KEYSIZE));

	//mnemonic words
	usb_write("\r\nMnemonic words:\r\n");
	usb_write(bytes_to_words(seed, 32));

	// reduce to spendkey
	memcpy(spendkey, seed, KEYSIZE);
	reduce32(spendkey);
	usb_write("\r\nSpendKey: ");
	usb_write(tohex(spendkey, KEYSIZE));
	// hash on the way to view key
	keccak_256(spendkey, KEYSIZE, viewkey);
	usb_write("\r\nViewKey: (");
	usb_write(tohex(viewkey, KEYSIZE));
	// reduce to view key
	reduce32(viewkey);
	usb_write(")\r\nViewKey:  ");
	usb_write(tohex(viewkey, KEYSIZE));
	usb_write("\r\n");

	// make public spend key
	uint8_t pubSpendkey[KEYSIZE];
	publickey(pubSpendkey, spendkey);
	// make public view key
	uint8_t pubViewkey[KEYSIZE];
	publickey(pubViewkey, viewkey);

	usb_write("\r\nPSK: ");
	usb_write(tohex(pubSpendkey, KEYSIZE));
	usb_write("\r\nPVK: ");
	usb_write(tohex(pubViewkey, KEYSIZE));
	usb_write("\r\n");

	// now build the public address
	// (network byte = 0x12) + (32-byte public spend key) + (32-byte public view key) + (4-byte hash)
	uint8_t address[PUBSIZE];
	address[0] = 0x12;
	memcpy(address+1, pubSpendkey, KEYSIZE);
	memcpy(address+1+KEYSIZE, pubViewkey, KEYSIZE);
	uint8_t hash[SHA3_256_DIGEST_LENGTH];
	keccak_256(address, 2*KEYSIZE+1, hash);
	memcpy(address+1+2*KEYSIZE, hash, 4);
	usb_write("\r\nPUB: ");
	usb_write(tohex(address, PUBSIZE));

	// base58 it
	char b58[200];
	encode58(b58, address, PUBSIZE);
	usb_write("\r\nB58: ");
	usb_write(b58);
	usb_write("\r\n");
}


int main(void)
{
	setup();
	oled_setup();
	usb_setup();

	oledSplash(&bmp_monerujo_splash);
	oledRefresh();

	bool newPress = true;

	while(1) {
		usb_poll();

		if (!gpio_get(GPIOC, GPIO5)){
			oledSwipeLeft();
			oledSplash(&bmp_monerujo_splash);
			oledRefresh();
		}

		if (!gpio_get(GPIOC, GPIO2)){
			if (newPress) {
				generateWallet();
				newPress = false;
			}
		} else {
			newPress = true;
		}
	}

	return 0;
}
