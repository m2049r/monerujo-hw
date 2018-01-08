/*
 * This file is part of the TREZOR project, https://trezor.io/
 *
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
#include "bitmaps.h"
#include "display.h"
#include "setup.h"
#include "usb.h"
#include "util.h"


int main(void)
{
	setup();
	oled_setup();
	usb_setup();

	oledClear();
	//oledDrawBitmap(42, 11, &bmp_logo64);

	const char* hello = "Hello Monero world!\r\n";
//	const char* hello = "0123456789ABCDEf0123456789ABCDEg0123456789ABCDEh0123456789ABCDEiX";
				
	while(1) {
		
		usb_poll();
		oledDrawBitmap(42, 11, &bmp_logo64);
		//oledBox(0,0,1000,1000,false);
		oledRefresh();
		
		if (!gpio_get(GPIOC, GPIO5)){
//			oledDrawBitmap(10, 11, &bmp_logo64);
//			oledSwipeRight();
//			oledDrawBitmap(70, 11, &bmp_logo64);
			oledSwipeLeft();
		}
		
		if (!gpio_get(GPIOC, GPIO2)){
			usb_write(hello);
			oledSwipeRight();
//			oledInvert(0,0,128,64);
//			delay(1000);
//			oledRefresh();
		}
//		oledClear();
	
	}

	return 0;
}
