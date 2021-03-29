/*
 * compress.c
 *
 *  Created on: Mar 29, 2021
 *      Author: nanashi
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct {
	int len;
	int pos;
} match_t;

typedef struct {
	uint8_t* p_data;
	int head;
	int tail;
} ringb_t;

static match_t find_match(ringb_t dict, ringb_t lookahead)
{
	int match_start, match_end;
	match_t match;
	match.len = 0;

	/* Compare dict and lookahead data and find the longest match */
	match_start = lookahead.tail;
	while (1){
		if ((lookahead.tail == lookahead.head) || (dict.tail == dict.head))		// don't look outside of the buffer area
			return match;

		if (dict.p_data[dict.tail] != lookahead.p_data[lookahead.tail])
			break;
		match.len++;

		/* Wrap the buffers around. */
		dict.tail = (dict.tail+1) & 0xfff;
		lookahead.tail = (lookahead.tail+1) & 0xfff;
	}

//	if (match.len > 1){
//		match_end = lookahead.tail;
//		lookahead.tail = match_start;
//		while (dict.p_data[dict.tail] == lookahead.p_data[lookahead.tail]){
//			match.len++;
//			dict.tail = (dict.tail+1) & 0xfff;
//			lookahead.tail = (lookahead.tail+1) & 0xfff;
//
//			if (lookahead.tail == match_end)
//				lookahead.tail = match_start;
//		}
//	}

	return match;
}

static match_t find_best_match(ringb_t dict, ringb_t lookahead)
{
	match_t best_match, match;
	best_match.len = 0;
	ringb_t match_dict = dict;
	match_dict.tail = (dict.head - 1) & 0xfff;

	/* Go through the entire dictionary and test
	 * each position for the best match */
	while (match_dict.tail != dict.tail){
		match = find_match(match_dict, lookahead);

		if (match.len > best_match.len) {
			best_match.len = match.len;
			best_match.pos = match_dict.tail;

			/* Maximum size is 17 because nibble size + 3. */
			if (best_match.len > 17){
				best_match.len = 18;
				break;
			}
		}

		match_dict.tail = (match_dict.tail - 1) & 0xfff;				// propagate backwards through the buffer
	}

	return best_match;
}

static void compress_chunk(uint8_t* p_source, uint8_t* p_compressed, unsigned int size)
{
	int i, cntr;
	uint16_t ctrl_word;													// control word to indicate when to copy data from dict
	uint8_t* p_ctrl_word;												// pointer to next control word in compressed stream
	match_t match;														// match between dict and lookahead buffer

	/* Initialize ring buffers */
	uint8_t rb_dict_data[4095];											// ring buffer dictionary data from decoded data
	for(i=0;i<4078;i++)													// pre-load buffer with 4078 zeros
		rb_dict_data[i] = 0;
	ringb_t dict;														// dictionary ring buffer
	dict.p_data = rb_dict_data;
	dict.head = 4078;
	dict.tail = 0;
	ringb_t lookahead;													// lookahead ring buffer
	lookahead.p_data = p_source;
	lookahead.head = 4095;
	lookahead.tail = 0;
	int rb_mask = 0xfff;												// ring buffer mask

	/* Write size to compressed stream. */
	*((uint32_t*) p_compressed) = (uint32_t) size;
	p_compressed += 4;

	ctrl_word = 0;
	p_ctrl_word = p_compressed;
	p_compressed++;
	cntr = 0;
	for (i=0;i<16;i++){
		match = find_best_match(dict, lookahead);						// search best match between dict and lookahead

		if (match.len > 2){

			/* Generate length-distance pair. */
			*p_compressed++ = (uint8_t) (match.pos & 0xff);
			*p_compressed = (uint8_t) ((match.pos & 0xf00) >> 4);
			*p_compressed++ |= (uint8_t) ((match.len - 3) & 0xf);

			/* Copy matched data to dict. */
			while (match.len--){
				dict.p_data[dict.head] = *lookahead.p_data++;
				dict.head = (dict.head + 1) & rb_mask;
				size--;
			}

		} else {
			/* Indicate unmatched byte in control word and
			 * write to dict. */
			ctrl_word |= 0x100;
			*p_compressed++ = *lookahead.p_data;
			dict.p_data[dict.head] = *lookahead.p_data;
			dict.head = (dict.head + 1) & rb_mask;
			if (dict.head == dict.tail)
				dict.tail = (dict.tail + 1) & rb_mask;
			lookahead.p_data++;
			size--;
		}
		ctrl_word >>= 1;

		/* Control word is written every 8 entries. */
		cntr++;
		if (cntr == 8){
			cntr = 0;
			*p_ctrl_word = (uint8_t) (ctrl_word & 0xff);
			p_ctrl_word = p_compressed;
			p_compressed++;
		}

		/* Adjust buffer size to data size. */
		if (size < lookahead.head)
			lookahead.head = (int) size;

	}
}

void compress_file(FILE* f_infile, FILE* f_outfile, size_t chunk_size)
{
	int num_chunks, i;
	size_t f_size;
	uint8_t* p_source;
	uint8_t* p_compressed;

	/* Seek first selected chunk. */
	fseek(f_infile, 0, SEEK_END);
	f_size = ftell(f_infile);
	fseek(f_infile, 0, SEEK_SET);

	num_chunks = 1; //(int) (f_size/chunk_size);

	for (i=0;i<num_chunks;i++){

		/* Seek next chunk to determine compressed size. */
		p_source = malloc(chunk_size);
		fread(p_source, sizeof(uint8_t), chunk_size, f_infile);
		p_compressed = malloc(chunk_size*2);

		compress_chunk(p_source, p_compressed, (unsigned int) chunk_size);
		fwrite(p_compressed, sizeof(uint8_t), 32, f_outfile);

		free(p_compressed);
		free(p_source);
	}
}
