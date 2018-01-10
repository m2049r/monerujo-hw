/*
 * This file is part of monerujo-hw
 *
 * Copyright (C) 2018 m2049r <m2049r@monerujo.io>
 * Copyright (C) 2014 Pavol Rusnak <stick@satoshilabs.com>
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
#include <libopencm3/stm32/spi.h>

#include <string.h>

#include "display.h"
#include "util.h"

#define OLED_SETCONTRAST			0x81
#define OLED_DISPLAYRESUME			0xA4
#define OLED_DISPLAYPAUSE			0xA5
#define OLED_NORMALDISPLAY			0xA6
#define OLED_INVERTDISPLAY			0xA7
#define OLED_DISPLAYOFF				0xAE
#define OLED_DISPLAYON				0xAF
#define OLED_SETDISPLAYOFFSET		0xD3
#define OLED_SETCOMPINS				0xDA
#define OLED_SETVCOMDESELECT		0xDB
#define OLED_SETDFOSC				0xD5
#define OLED_SETPRECHARGE			0xD9
#define OLED_SETMULTIPLEX			0xA8
#define OLED_SETSTARTLINE			0x40
#define OLED_MEMORYMODE				0x20
#define OLED_CHARGEPUMP				0x8D
#define OLED_SETCOMREMAP            0xC8
#define OLED_SETSEGREMAP            0xA1

#define OLED_SPI_BASE	SPI1
#define OLED_DC_PORT	GPIOB
#define OLED_DC_PIN		GPIO0	// PB0 | Data/Command
#define OLED_CS_PORT	GPIOA
#define OLED_CS_PIN		GPIO4	// PA4 | SPI Select
#define OLED_RST_PORT	GPIOB
#define OLED_RST_PIN	GPIO1	// PB1 | Reset display

/* The display size is OLED_WIDTH x OLED_HEIGHT (128x64).
 * The contents of this display are buffered in _oledbuffer.  This is
 * an array of OLED_WIDTH * OLED_HEIGHT/8 bytes.  At byte y*OLED_WIDTH + x
 * it stores the column of pixels from (x,8y) to (x,8y+7); the LSB stores
 * the top most pixel.  The pixel (0,0) is the top left corner of the
 * display.
 */

static uint8_t _oledBuffer[OLED_BUFSIZE];

/*
 * macros to convert coordinate to bit position
 */
#define OLED_OFFSET(x, y) (OLED_BUFSIZE - 1 - (x) - ((y)/8)*OLED_WIDTH)
#define OLED_MASK(x, y)   (1 << (7 - (y) % 8))

/*
 * Send a block of data to the display via the SPI bus.
 */
inline void SPISend(uint8_t *data, int len, bool isData) {
	if (isData)
		gpio_set(OLED_DC_PORT, OLED_DC_PIN);	// set to DATA
	else
		gpio_clear(OLED_DC_PORT, OLED_DC_PIN);	// set to COMMAND

	for (int i = 0; i < len; i++) {
		spi_send(OLED_SPI_BASE, data[i]);
	}
	// now, wait for SPI communication to finish
	while ((SPI_SR(OLED_SPI_BASE) & SPI_SR_BSY))
		__asm__("nop");
}

/*
 * Draws a white pixel at x, y
 */
void oledDrawPixel(int x, int y) {
	if ((x < 0) || (y < 0) || (x >= OLED_WIDTH) || (y >= OLED_HEIGHT)) {
		return;
	}
	_oledBuffer[OLED_OFFSET(x, y)] |= OLED_MASK(x, y);
}

/*
 * Clears pixel at x, y
 */
void oledClearPixel(int x, int y) {
	if ((x < 0) || (y < 0) || (x >= OLED_WIDTH) || (y >= OLED_HEIGHT)) {
		return;
	}
	_oledBuffer[OLED_OFFSET(x, y)] &= ~OLED_MASK(x, y);
}

/*
 * Inverts pixel at x, y
 */
void oledInvertPixel(int x, int y) {
	if ((x < 0) || (y < 0) || (x >= OLED_WIDTH) || (y >= OLED_HEIGHT)) {
		return;
	}
	_oledBuffer[OLED_OFFSET(x, y)] ^= OLED_MASK(x, y);
}

/*
 * Initialize the display.
 */
void oled_setup() {

	static uint8_t initCommands[] = {
	OLED_MEMORYMODE, 0x00, // Horizontal Addressing Mode
			OLED_CHARGEPUMP, 0x14, // enable charge pump
			OLED_DISPLAYON };

	/*
	 * Function	     | CS# | D/C# | D0 = SCLK   | D1 = SDIN
	 * Write Command |  L  |  L   | rising edge | data D7->D0
	 * Write Data    |  L  |  H   | rising edge | data D7->D0
	 */

	// TODO check timings here - they need to based off HSE_VALUE and not be random numbers!
	/*
	 *
	 * Power ON sequence :
	 * 1. Power ON VDD
	 * 2. After VDD become stable, set RES# pin LOW (logic low) for at least 3us (t1)
	 *    and then HIGH (logic high).
	 * 3. After set RES# pin LOW (logic low), wait for at least 3us (t2). Then Power ON VCC.
	 * 4. After VCC become stable, send command AFh for display ON. SEG/COM will be ON after 100ms (tAF).
	 *
	 *
	 * Power OFF sequence:
	 * 1. Send command AEh for display OFF.
	 * 2. Power OFF VCC.
	 * 3. Power OFF VDD after tOFF (Typical tOFF=100ms)
	 */

	// Reset the OLED
	gpio_set(OLED_RST_PORT, OLED_RST_PIN);
	delay(5); // 5us
	gpio_clear(OLED_RST_PORT, OLED_RST_PIN);
	delay(5);
	gpio_set(OLED_RST_PORT, OLED_RST_PIN);

	/*
	 * The chip is initialized with the following status:
	 * 1. Display is OFF
	 * 2. 128 x 64 Display Mode
	 * 3. Normal segment and display data column address and row address mapping
	 *    (SEG0 mapped to address 00h and COM0 mapped to address 00h)
	 * 4. Shift register data clear in serial interface
	 * 5. Display start line is set at display RAM address 0
	 * 6. Column address counter is set at 0
	 * 7. Normal scan direction of the COM outputs
	 * 8. Contrast control register is set at 7Fh
	 * 9. Normal display mode (Equivalent to A4h command)
	 */

	// SPI select - keep it selected as the display is the only device on the bus
	gpio_clear(OLED_CS_PORT, OLED_CS_PIN);

	// initialise non-default behaviour
	SPISend(initCommands, sizeof(initCommands), false);

	oledClear();
	oledRefresh();
}

/*
 * Clears the display buffer (sets all pixels to black)
 */
void oledClear() {
	memset(_oledBuffer, 0, sizeof(_oledBuffer));
}

/*
 * Refresh the display. This copies the buffer to the display to show the
 * contents.  This must be called after every operation to the buffer to
 * make the change visible.  All other operations only change the buffer
 * not the content of the display.
 */
void oledRefresh() {
	// since we always send a fullsize buffer, we dont need to set start/end for Col/Page.
	SPISend(_oledBuffer, sizeof(_oledBuffer), true);
}

void oledDrawChar(int x, int y, char c, int zoom) {
	if (x >= OLED_WIDTH || y >= OLED_HEIGHT || y <= -FONT_HEIGHT) {
		return;
	}

	int char_width = fontCharWidth(c);
	const uint8_t *char_data = fontCharData(c);

	if (x <= -char_width) {
		return;
	}

	for (int xo = 0; xo < char_width; xo++) {
		for (int yo = 0; yo < FONT_HEIGHT; yo++) {
			if (char_data[xo] & (1 << (FONT_HEIGHT - 1 - yo))) {
				if (zoom <= 1) {
					oledDrawPixel(x + xo, y + yo);
				} else {
					oledBox(x + xo * zoom, y + yo * zoom, x + (xo + 1) * zoom - 1, y + (yo + 1) * zoom - 1,
					true);
				}
			}
		}
	}
}

int oledStringWidth(const char *text) {
	if (!text)
		return 0;
	int l = 0;
	for (; *text; text++) {
		l += fontCharWidth(*text) + 1;
	}
	return l;
}

void oledDrawStringZoom(int x, int y, const char* text, int zoom) {
	if (!text)
		return;
	int l = 0;
	for (; *text; text++) {
		oledDrawChar(x + l, y, *text, zoom);
		l += zoom * (fontCharWidth(*text) + 1);
	}
}

void oledDrawStringCenter(int y, const char* text) {
	int x = ( OLED_WIDTH - oledStringWidth(text)) / 2;
	oledDrawString(x, y, text);
}

void oledDrawStringRight(int x, int y, const char* text) {
	x -= oledStringWidth(text);
	oledDrawString(x, y, text);
}

void oledDrawBitmap(int x, int y, const BITMAP *bmp) {
	for (int i = 0; i < bmp->width; i++) {
		for (int j = 0; j < bmp->height; j++) {
			if (bmp->data[(i / 8) + j * bmp->width / 8] & (1 << (7 - i % 8))) {
				oledDrawPixel(x + i, y + j);
			} else {
				oledClearPixel(x + i, y + j);
			}
		}
	}
}

void oledSplash(const BITMAP *bmp) {
	if ((bmp->width != OLED_WIDTH) || (bmp->height != OLED_HEIGHT)) return;
	memcpy(_oledBuffer, bmp->data, sizeof(_oledBuffer));
}


#define max(X,Y) ((X) > (Y) ? (X) : (Y))
#define min(X,Y) ((X) < (Y) ? (X) : (Y))

/*
 * Inverts box between (x1,y1) and (x2,y2) inclusive.
 */
void oledInvert(int x1, int y1, int x2, int y2) {
	x1 = max(x1, 0);
	y1 = max(y1, 0);
	x2 = min(x2, OLED_WIDTH - 1);
	y2 = min(y2, OLED_HEIGHT - 1);
	for (int x = x1; x <= x2; x++) {
		for (int y = y1; y <= y2; y++) {
			oledInvertPixel(x, y);
		}
	}
}

/*
 * Draw a filled rectangle.
 */
void oledBox(int x1, int y1, int x2, int y2, bool set) {
	x1 = max(x1, 0);
	y1 = max(y1, 0);
	x2 = min(x2, OLED_WIDTH - 1);
	y2 = min(y2, OLED_HEIGHT - 1);
	for (int x = x1; x <= x2; x++) {
		for (int y = y1; y <= y2; y++) {
			set ? oledDrawPixel(x, y) : oledClearPixel(x, y);
		}
	}
}

void oledHLine(int y) {
	if (y < 0 || y >= OLED_HEIGHT) {
		return;
	}
	for (int x = 0; x < OLED_WIDTH; x++) {
		oledDrawPixel(x, y);
	}
}

/*
 * Draw a rectangle frame.
 */
void oledFrame(int x1, int y1, int x2, int y2) {
	for (int x = x1; x <= x2; x++) {
		oledDrawPixel(x, y1);
		oledDrawPixel(x, y2);
	}
	for (int y = y1 + 1; y < y2; y++) {
		oledDrawPixel(x1, y);
		oledDrawPixel(x2, y);
	}
}

/*
 * Animates the display, swiping the current contents out to the left.
 * This clears the display.
 */
void oledSwipeLeft(void) {
	for (int i = 0; i < OLED_WIDTH; i++) {

		for (int j = 0; j < OLED_HEIGHT / 8; j++) {
			for (int k = OLED_WIDTH - 1; k > 0; k--) {
				_oledBuffer[j * OLED_WIDTH + k] = _oledBuffer[j * OLED_WIDTH + k - 1];
			}
			_oledBuffer[j * OLED_WIDTH] = 0x00;
		}

		oledRefresh();
		delay(1000000/OLED_WIDTH);
	}
}

/*
 * At byte j*OLED_WIDTH + i it stores the column of pixels
 * from (i,8j) to (i,8j+7);
 * the LSB stores the top most pixel.
 * The pixel (0,0) is the top left corner of the display.
 */

/*
 * Animates the display, swiping the current contents out to the right.
 * This clears the display.
 */
void oledSwipeRight(void) {
	for (int i = 0; i < OLED_WIDTH; i++) {

		for (int j = 0; j < OLED_HEIGHT / 8; j++) {
			for (int k = 0; k < OLED_WIDTH - 1; k++) {
				_oledBuffer[j * OLED_WIDTH + k] = _oledBuffer[j * OLED_WIDTH + k + 1];
			}
			_oledBuffer[j * OLED_WIDTH + OLED_WIDTH - 1] = 0x00;
		}

		oledRefresh();
		delay(1000000/OLED_WIDTH);
	}
}
