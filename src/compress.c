/*
 * compress.c
 *
 *  Created on: Mar 29, 2021
 *      Author: nanashi
 */
#include <stdlib.h>
#include <stdint.h>

void compress(uint8_t* p_source, uint8_t* p_compressed, int size)
{
	uint8_t rb_dict[4095];											// ring buffer dictionary from decoded data
	int pos_dict = 4078;											// dictionary position; starts with 4078 zeros pre-loaded
	int rb_mask = 0xfff;											// ring buffer mask
	int i;

	for(i=0;i<4078;i++)
		rb_dict[i] = 0;
}
