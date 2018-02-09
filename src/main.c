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
#include <stdlib.h>
#include "mnemonics/mnemonics.h"
#include "timer.h"
#include "button.h"
#include "libopencm3/cm3/vector.h"

#define fromhex(a) fromhexLE(a)
#define tohex(a,b) tohexLE(a,b)

#define KEYSIZE 32
#define PUBSIZE (1+2*KEYSIZE+4)

button leftButton, rightButton;

#define MNEMONIC_WORDS 25

static uint32_t mnemonics[MNEMONIC_WORDS];
static int currentWord;

static void showWord(int idx) {
	if ((idx<0) || (idx>=99)) return;
	char n[3];
	itoa(idx+1, n, 10);
	oledClear();
	oledDrawStringZoom(0, 0, n, 2);
	const char* word = mnemonic_word(mnemonics[idx]);
	int x = ( OLED_WIDTH - 2*oledStringWidth(word)) / 2;
	int y = (OLED_HEIGHT-FONT_HEIGHT)/2;
	oledDrawStringZoom(x, y, word, 2);
	oledRefresh();
	usb_write(n);
	usb_write(": ");
	usb_write(mnemonic_word(mnemonics[idx]));
	usb_write("\r\n");
}

static void nextWord(void) {
	if (currentWord >= MNEMONIC_WORDS-1)
		return;
	showWord(++currentWord);
}

static void prevWord(void) {
	if (currentWord <= 0)
		return;
	showWord(--currentWord);
}

static void generateWallet(void) {
	uint8_t spendkey[KEYSIZE];
	uint8_t viewkey[KEYSIZE];

	uint32_t rng_seed[8];
	for (int i = 0; i < 8; i++) {
		rng_seed[i] = rng_get_random_blocking();
	}

	uint8_t *seed = (uint8_t*) rng_seed; //why this is uint8_t when it is pointing to uint32_t?
										 //it is "bits & memory & array" stuff?

	// for testing, override rng:  to fit example here:
	// https://monero.stackexchange.com/questions/874/what-is-the-checksum-at-the-end-of-a-mnemonic-seed
	//memcpy(seed, fromhex("2f5f26c8d75bc043eeec32c813aa3e5ec426256e8daef38a1cf91040dab44b0f"), 32);

	usb_write("\r\nSeed: ");
	usb_write(tohex(seed, KEYSIZE));

	usb_write("\r\nMnemonics: ");
	//TODO: check return value of bytes_to_words()
	bytes_to_words(seed, KEYSIZE, mnemonics);
	for (int i = 0; i < MNEMONIC_WORDS; i++) {
		usb_write(mnemonic_word(mnemonics[i]));
		if (i != 24)
			usb_write(" ");
	}
	usb_write("\r\n");

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

	usb_write("\r\nPubSK: ");
	usb_write(tohex(pubSpendkey, KEYSIZE));
	usb_write("\r\nPubVK: ");
	usb_write(tohex(pubViewkey, KEYSIZE));
	usb_write("\r\n");

	// now build the public address
	// (network byte = 0x12) + (32-byte public spend key) + (32-byte public view key) + (4-byte hash)
	uint8_t address[PUBSIZE];
	address[0] = 0x12;
	memcpy(address + 1, pubSpendkey, KEYSIZE);
	memcpy(address + 1 + KEYSIZE, pubViewkey, KEYSIZE);
	uint8_t hash[SHA3_256_DIGEST_LENGTH];
	keccak_256(address, 2 * KEYSIZE + 1, hash);
	memcpy(address + 1 + 2 * KEYSIZE, hash, 4);
	usb_write("\r\nPUB: ");
	usb_write(tohex(address, PUBSIZE));

	// base58 it
	char b58[200];
	encode58(b58, address, PUBSIZE);
	usb_write("\r\nPublic Address: ");
	usb_write(b58);
	usb_write("\r\n");

	oledClear();
	oledDrawStringCenter(OLED_HEIGHT/2, "Wallet generated!");
	oledRefresh();
	currentWord = -1;
	leftButton.pressed = prevWord;
	rightButton.pressed = nextWord;
}

static void setup_buttons(void) {
	button_setup(&leftButton, GPIOC, GPIO5);
	button_setup(&rightButton, GPIOC, GPIO2);
}

int main(void) {
	// point to the exception vector in flash even if "Embedded SRAM" boot mode (e.g. after DFU leave)
	SCB_VTOR = (uint32_t) &vector_table;
	setup();
	oled_setup();
	usb_setup();
	setup_buttons();
	timer_init();

	//oledSplash(&bmp_monerujo_splash);
	oledDrawStringCenter(0,  "ABCDEFGHIJKLM");
	oledDrawStringCenter(8,  "NOPQRSTUVWXYZ");
	oledDrawStringCenter(16, "abcdefghijklm");
	oledDrawStringCenter(24, "nopqrstuvwxyz");
	oledDrawStringCenter(32, "0123456789");
	oledDrawStringCenter(40, "$`+-*/=%\"'#@&_()");
	oledDrawStringCenter(48, ",.:;?!\\|{}<>[]~^");
	oledRefresh();

	leftButton.pressed = NULL;
	rightButton.pressed = generateWallet;

	while (1) {
		usb_poll();
		button_poll(&leftButton);
		button_poll(&rightButton);
	}

	return 0;
}
