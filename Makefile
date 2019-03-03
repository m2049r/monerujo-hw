##
## This file is part of the libopencm3 project.
##
## Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
##
## This library is free software: you can redistribute it and/or modify
## it under the terms of the GNU Lesser General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This library is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU Lesser General Public License for more details.
##
## You should have received a copy of the GNU Lesser General Public License
## along with this library.  If not, see <http://www.gnu.org/licenses/>.
##

BINARY = monerujo


###############################################################################
# Source files (modify here after adding new .c files into src folder)

OBJS		+= src/bitmaps.o
OBJS		+= src/button.o
OBJS		+= src/crypto.o
OBJS		+= src/display.o
OBJS		+= src/fonts.o
OBJS		+= src/logo.o
OBJS		+= src/main.o
OBJS		+= src/setup.o
OBJS		+= src/timer.o
OBJS		+= src/usb.o
OBJS		+= src/util.o
OBJS		+= src/crypto/curve25519-donna-32bit.o
OBJS		+= src/crypto/curve25519-donna-helpers.o
OBJS		+= src/crypto/curve25519-donna-scalarmult-base.o
OBJS		+= src/crypto/ed25519.o
OBJS		+= src/crypto/ed25519-donna-32bit-tables.o
OBJS		+= src/crypto/ed25519-donna-basepoint-table.o
OBJS		+= src/crypto/ed25519-donna-impl-base.o
OBJS		+= src/crypto/ed25519-keccak.o
OBJS		+= src/crypto/ed25519-sha3.o
OBJS		+= src/crypto/modm-donna-32bit.o
OBJS		+= src/crypto/sha2.o
OBJS		+= src/crypto/sha3.o
OBJS		+= src/mnemonics/mnemonics.o
OBJS		+= src/qrcode/qrcode.o
OBJS		+= src/qrcode/qrcodegen.o
#OBJS		+= src/newCfileHere.

OPENCM3_DIR=libopencm3
LDSCRIPT = monerujo.ld

include libopencm3.target.mk

noblock nonblock:
	@printf "  Making an I/O nonblocking application (-nbl)\n"
	@[ -f $(BINARY).bin ] && @mv $(BINARY).bin $(BINARY)-old.bin
	@[ -f $(BINARY).elf ] && @mv $(BINARY).bin $(BINARY)-old.elf
	@[ -f $(BINARY).hex ] && @mv $(BINARY).bin $(BINARY)-old.hex
	@[ -e $(CPPFLAGS) ] && CPPFLAGS="-DNO_BLOCKING_IO" make all || CPPFLAGS="$(CPPFLAGS) -DNO_BLOCKING_IO" make all
	@mv $(BINARY).bin $(BINARY)-nbl.bin
	@mv $(BINARY).elf $(BINARY)-nbl.elf
	@mv $(BINARY).hex $(BINARY)-nbl.hex
