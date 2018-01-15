/*
 * wordlist.c
 *
 *  Created on: Jan 12, 2018
 *      Author: ia
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>  //itoa :/

#include <libopencm3/stm32/crc.h>
#include "wordlist.h"
#include "crypto.h"


const char * bytes_to_words(uint8_t *src, uint8_t len)//, int len)//), const char *words)//, const char *language_name)
  {
	uint16_t word_list_length = 1626;

	//Seed will be constructed in this array, but is [11] long enough? do we have longer word in wordlist?
    static char mnemonic_seed[25][11];
    //3*24 +1 = 73 this is just to prepare a string for checksum
    static char mnemonic_seed_for_checksum[73];

    for (uint8_t i=0; i<24; i++)
    	//preparing all arrays to behave as an empty place
    	mnemonic_seed[i][0] = '\0';

    //to be sure that it will be at the end interpreted as a string
    mnemonic_seed_for_checksum[72] = '\0';

    for (int8_t i=0; i < len/4; i++)
    {
		  uint32_t w1, w2, w3; //three words

		  uint32_t val;        // each iteration are 4 chars from hexadecimal string here

		  memcpy(&val, src + (i * 4), 4);

		  w1 = val % word_list_length;
		  w2 = ((val / word_list_length) + w1) % word_list_length;
		  w3 = (((val / word_list_length) / word_list_length) + w2) % word_list_length;

		  //prepare a seed which is going out as a seed
		  strcat(mnemonic_seed[3*i], wordlist_electrum[w1]);
		  strcat(mnemonic_seed[3*i+1], wordlist_electrum[w2]);
		  strcat(mnemonic_seed[3*i+2], wordlist_electrum[w3]);

		  //debug
		  usb_write(mnemonic_seed[3*i]);
		  usb_write(" ");
		  usb_write(mnemonic_seed[3*i+1]);
		  usb_write(" ");
		  usb_write(mnemonic_seed[3*i+2]);
		  usb_write(" ");

    }

    //construct string for checksum here
    for (int i=0; i<24; i++)
    {
    	mnemonic_seed_for_checksum[3*i]   = mnemonic_seed[i][0];
    	mnemonic_seed_for_checksum[3*i+1] = mnemonic_seed[i][1];
    	mnemonic_seed_for_checksum[3*i+2] = mnemonic_seed[i][2];
    }
    //debug
    usb_write("\r\n\r\n checksum seed: \r\n");
    usb_write(mnemonic_seed_for_checksum);

    // Calculate CRC check here

    // reset hw block
    crc_reset();

    uint32_t num;

    for (int i = 0; i < 18; i++)
    {
    	uint32_t digest = 0;
    	digest = ( (uint32_t) mnemonic_seed_for_checksum[4*i]   << 24 ) | digest;
    	digest = ( (uint32_t) mnemonic_seed_for_checksum[4*i+1] << 16 ) | digest;
    	digest = ( (uint32_t) mnemonic_seed_for_checksum[4*i+2] << 8  ) | digest;
    	digest = ( (uint32_t) mnemonic_seed_for_checksum[4*i+3]       ) | digest;
    	num = crc_calculate(digest);
    }

    //debug
    //crc_reset();
    //num = crc_calculate(345);

    static char snum[21];
    sprintf(snum, "%lu", num);

    usb_write("\r\n\r\n  checksum number: \r\n");

    //need to get 1790087523 !!
    //according to https://monero.stackexchange.com/questions/874/what-is-the-checksum-at-the-end-of-a-mnemonic-seed
    usb_write(snum);

    return 0;
}



