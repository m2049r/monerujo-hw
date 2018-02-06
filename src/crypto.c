/*
 * This file is part of monerujo-hw
 *
 * Copyright (C) 2018 m2049r <m2049r@monerujo.io> et al.
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

#include "crypto.h"
#include "crypto/ed25519-donna.h"

static const char alphabet[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
static const size_t alphabet_size = sizeof(alphabet) - 1;
static const size_t encoded_block_sizes[] = {0, 2, 3, 5, 6, 7, 9, 10, 11};

static uint64_t uint_8be_to_64(const uint8_t* data, size_t size)
{
	uint64_t res = 0;
	switch (9 - size)
	{
		case 1:            res |= *data++;
		case 2: res <<= 8; res |= *data++;
		case 3: res <<= 8; res |= *data++;
		case 4: res <<= 8; res |= *data++;
		case 5: res <<= 8; res |= *data++;
		case 6: res <<= 8; res |= *data++;
		case 7: res <<= 8; res |= *data++;
		case 8: res <<= 8; res |= *data;
			break;
		default:
			break;
	}
	return res;
}

static void encode_block(char* res, const uint8_t* block, size_t size)
{
	uint64_t num = uint_8be_to_64(block, size);
	size_t i = encoded_block_sizes[size] - 1;
	memset(res, alphabet[0], i);
	res[i+1] = 0;
	while (0 < num)
	{
		uint64_t remainder = num % alphabet_size;
		num /= alphabet_size;
		res[i] = alphabet[remainder];
		i--;
	}
}

#define FULL_BLOCK_SIZE 8
#define FULL_ENCODED_BLOCK_SIZE encoded_block_sizes[FULL_BLOCK_SIZE]

void encode58(char* res, const uint8_t* data, size_t size)
{
  if (!size) {
	  res[0] = 0;
	  return;
  }

  size_t full_block_count = size / FULL_BLOCK_SIZE;
  size_t last_block_size = size % FULL_BLOCK_SIZE;
  size_t res_size = full_block_count * FULL_ENCODED_BLOCK_SIZE + encoded_block_sizes[last_block_size];

  for (size_t i = 0; i < full_block_count; i++)
  {
    encode_block(res + i * FULL_ENCODED_BLOCK_SIZE, data + i * FULL_BLOCK_SIZE, FULL_BLOCK_SIZE);
  }

  if (last_block_size)
  {
	encode_block(res + full_block_count * FULL_ENCODED_BLOCK_SIZE, data + full_block_count * FULL_BLOCK_SIZE, last_block_size);
  }
  res[res_size] = 0;
}

void reduce32(uint8_t *x) {
	bignum256modm r, q1, r1;

	expand_raw256_modm(r1, x);
	/* q1 = x >> 248 = 264 bits = 9 30 bit elements */
	q1[0] = x[31];// & 0x3fffffff;
	q1[1] = 0;
	q1[2] = 0;
	q1[3] = 0;
	q1[4] = 0;
	q1[5] = 0;
	q1[6] = 0;
	q1[7] = 0;
	q1[8] = 0;
	barrett_reduce256_modm(r, q1, r1);
	contract256_modm(x, r);
}

void publickey(uint8_t *pub, const uint8_t *sec)
{
	bignum256modm a;
	ge25519 ALIGN(16) A;

	/* A = aB */
	expand_raw256_modm(a, sec);
	ge25519_scalarmult_base_niels(&A, ge25519_niels_base_multiples, a);
	ge25519_pack(pub, &A);
}
