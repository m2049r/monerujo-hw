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

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <libopencm3/stm32/gpio.h>
#include "libopencm3/stm32/rng.h"
#include "libopencm3/cm3/vector.h"
#include "bitmaps.h"
#include "display.h"
#include "setup.h"
#include "usb.h"
#include "util.h"
#include "crypto/sha3.h"
#include "crypto.h"
#include "mnemonics.h"
#include "timer.h"
#include "button.h"
#include "qrcode.h"

#define NEXT_LINE (FONT_HEIGHT+2)

#define fromhex(a) fromhexLE(a)
#define tohex(a,b) tohexLE(a,b)

#define KEYSIZE 32
#define PUBSIZE (1+2*KEYSIZE+4)

button leftButton, rightButton;

#define MNEMONIC_WORDS 25

static uint32_t mnemonics[MNEMONIC_WORDS];
static int currentWord;
static char public_address[200]; //TODO: correct size + check buffer size on creation

static void showLeftButtonLabel(char* label) {
	int x = 2;
	int y = OLED_HEIGHT - 1 - FONT_HEIGHT - 1;
	oledBox(x - 2, y - 3, x + oledStringWidth(label) - OLED_CHAR_SPACE + 2 - 1 + 1, y + FONT_HEIGHT + 1 + 1, false);
	oledBox(x - 2, y - 2, x + oledStringWidth(label) - OLED_CHAR_SPACE + 2 - 1, y + FONT_HEIGHT + 1, true);
	oledDrawStringInverted(x, y, label);
}

static void showRightButtonLabel(char* label) {
	int x = (OLED_WIDTH - 1) - oledStringWidth(label) - OLED_CHAR_SPACE;
	int y = OLED_HEIGHT - 1 - FONT_HEIGHT - 1;
	oledBox(x - 2, y - 3, x + oledStringWidth(label) - OLED_CHAR_SPACE + 2 - 1 + 1, y + FONT_HEIGHT + 1 + 1, false);
	oledBox(x - 2, y - 2, x + oledStringWidth(label) - OLED_CHAR_SPACE + 2 - 1, y + FONT_HEIGHT + 1, true);
	oledDrawStringInverted(x, y, label);
}

static void nextWord(void);
static void prevWord(void);
static void showAddress(void);

#define OLED_CONTRAST_QR 0

#define MONERO_SCHEMA "monero:"

static void showQrAddress(void) {
	oledClear();
	char* uri[95+7+1];
	strcpy(uri, MONERO_SCHEMA);
	strcat(uri, public_address);
	if (!drawQrCode(uri)) {
		oledDrawStringCenter(28, "QR CODE FAILED");
	} else {
		// Seems to not be needed?
		//oledSetContrast(OLED_CONTRAST_QR);
	}
	showLeftButtonLabel("Address");
	oledRefresh();
	rightButton.pressed = NULL;
	leftButton.pressed = showAddress;
}

static void showAddress(void) {
	char myAddress[96]; // 95+terminator
	oledClear();
	int l = 0;
	oledDrawStringCenter(l, "New Wallet Address:");
	l += NEXT_LINE;
	char* p = myAddress;
	while (p < myAddress + sizeof(myAddress) - 1) {
		strlcpy(myAddress, public_address, sizeof(myAddress));
		char* q = myAddress + sizeof(myAddress) - 1; // point to terminator
		while (oledStringWidth(p) > OLED_WIDTH) {
			*(--q) = '\0';
		}
		// here we have a string pointed to by p which fits on the display
		oledDrawStringCenter(l, p);
		l += NEXT_LINE;
		p = q; // where we stopped
	}
	showRightButtonLabel("QR");
	oledRefresh();
	rightButton.pressed = showQrAddress;
	leftButton.pressed = NULL;
}

static void showWord(int idx) {
	if ((idx < 0) || (idx >= 99))
		return;
	char n[3];
	itoa(idx + 1, n, 10);
	oledClear();
	oledDrawStringZoom(0, 0, n, 2);
	const char* word = mnemonic_word(mnemonics[idx]);
	int y = (OLED_HEIGHT - FONT_HEIGHT) / 2;
	int width = oledStringWidth(word);
	if (2 * width <= OLED_WIDTH) {
		int x = ( OLED_WIDTH - 2 * oledStringWidth(word)) / 2;
		oledDrawStringZoom(x, y, word, 2);
	} else {
		oledDrawStringCenter(y, word);
	}
	if (idx > 0) {
		showLeftButtonLabel(FONT_LEFT);
		leftButton.pressed = prevWord;
	} else {
		leftButton.pressed = NULL;
	}
	if (idx < 24) {
		showRightButtonLabel(FONT_RIGHT);
		rightButton.pressed = nextWord;
	} else {
		showRightButtonLabel("DONE");
		rightButton.pressed = showAddress;
	}
	oledRefresh();
	usb_write(n);
	usb_write(": ");
	usb_write(mnemonic_word(mnemonics[idx]));
	usb_write("\r\n");
}

static void nextWord(void) {
	if (currentWord >= MNEMONIC_WORDS - 1)
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
	encode58(public_address, address, PUBSIZE);
	usb_write("\r\nPublic Address: ");
	usb_write(public_address);
	usb_write("\r\n");

	oledClear();
	int l = 0;
	oledDrawStringCenter(l, "Press right button to");
	l += NEXT_LINE;
	oledDrawStringCenter(l, "show seed words.");
	l += NEXT_LINE;
	oledDrawStringCenter(l, "You will NOT ");
	l += NEXT_LINE;
	oledDrawStringCenter(l, "see them again.");
	l += NEXT_LINE;
	oledDrawStringCenter(l, "Write them down.");
	l += NEXT_LINE;
	oledDrawStringCenter(l, "NOW");
	showRightButtonLabel("SEED");
	oledRefresh();
	currentWord = -1;
	leftButton.pressed = showQrAddress;
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

	oledClear();
	oledSplash(&bmp_monerujo_splash);
	oledRefresh();
	delay_ms(1000);
	oledClear();
	oledDrawStringCenter(16, "Press right button for");
	oledDrawStringCenter(32, "new wallet");
	showRightButtonLabel("NEW");
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
