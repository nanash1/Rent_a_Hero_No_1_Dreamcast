/*
 * compress.c
 *
 *  Created on: Mar 29, 2021
 *      Author: nanashi
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "buffer.h"


typedef struct {
	int len;
	int pos;
} match_t;

static match_t find_match(ringb_t dict, winb_t lookahead)
{
	int lookahead_empty = 0;
	int dict_empty = 0;
	int full_match = 1;
	ringb_t _dict;
	match_t match;
	match.len = 0;

	/* Compare dict and lookahead data and find the longest match. */
	_dict = dict;
	while (!dict_empty && !lookahead_empty){
		if (ringb_pop(&dict, &dict_empty) != winb_pop(&lookahead, &lookahead_empty)){
			full_match = 0;
			break;
		}
		match.len++;
	}

	/* Look for repeats. */
	if (full_match){
		dict = _dict;
		while (!lookahead_empty){

			if (ringb_pop(&dict, &dict_empty) != winb_pop(&lookahead, &lookahead_empty))
				break;
			match.len++;

			if (dict_empty)
				dict = _dict;
		}
	}

	return match;
}

static match_t find_best_match(ringb_t dict, winb_t lookahead)
{
	match_t best_match, match;
	best_match.len = 0;
	ringb_t match_dict = dict;
	match_dict.tail = (dict.head - 1) & 0xfff;

	/* Go through the entire dictionary and test
	 * each position for the best match */
	while (match_dict.tail != match_dict.head){
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

static int compress_chunk(uint8_t* p_source, uint8_t* p_compressed, unsigned int size_src)
{
	uint8_t* p_comp_base = p_compressed;
	int is_empty = 0;
	int padding, cntr;
	uint16_t ctrl_word;													// control word to indicate when to copy data from dict
	uint8_t* p_ctrl_word;												// pointer to next control word in compressed stream
	match_t match;														// match between dict and lookahead buffer
	uint8_t temp_byte;

	/* Initialize ring buffers */
	ringb_t dict = ringb_init();
	winb_t lookahead = winb_init(p_source, size_src);

	/* Write size to compressed stream. */
	*((uint32_t*) p_compressed) = (uint32_t) size_src;
	p_compressed += 4;

	ctrl_word = 0;
	p_ctrl_word = p_compressed;
	p_compressed++;
	cntr = 8;
	while (!is_empty){

		match = find_best_match(dict, lookahead);						// search best match between dict and lookahead

		if (match.len > 2){

			/* Generate length-distance pair. */
			*p_compressed++ = (uint8_t) (match.pos & 0xff);
			*p_compressed = (uint8_t) ((match.pos & 0xf00) >> 4);
			*p_compressed++ |= (uint8_t) ((match.len - 3) & 0xf);

			/* Copy matched data to dict. */
			while (match.len--){
				ringb_insert(&dict, winb_advance(&lookahead, &is_empty));
			}

		} else {
			/* Indicate unmatched byte in control word and
			 * write to dict. */
			ctrl_word |= 0x100;
			temp_byte = winb_advance(&lookahead, &is_empty);
			*p_compressed++ = temp_byte;
			ringb_insert(&dict, temp_byte);
		}
		ctrl_word >>= 1;

		/* Control word is written every 8 entries. */
		if (!(--cntr)){
			cntr = 8;
			*p_ctrl_word = (uint8_t) (ctrl_word & 0xff);
			p_ctrl_word = p_compressed;
			p_compressed++;
		}
	}
	*p_ctrl_word = (uint8_t) ((ctrl_word >> cntr) & 0xff);
	padding = (p_compressed - p_comp_base) % 4;
	padding = (padding > 0) ? 4 - padding : 0;
	while (padding--)
		*p_compressed++ = 0;

	return p_compressed - p_comp_base;
}

void compress_file(FILE* f_infile, FILE* f_outfile, size_t chunk_size)
{
	int num_chunks, i;
	int comp_chunk_size;
	size_t f_size;
	uint8_t* p_source;
	uint8_t* p_compressed;

	/* Seek first selected chunk. */
	fseek(f_infile, 0, SEEK_END);
	f_size = ftell(f_infile);
	fseek(f_infile, 0, SEEK_SET);

	num_chunks = (int) (f_size/chunk_size);

	for (i=0;i<num_chunks;i++){

		/* Seek next chunk to determine compressed size. */
		p_source = malloc(chunk_size);
		fread(p_source, sizeof(uint8_t), chunk_size, f_infile);
		p_compressed = malloc(chunk_size*2);

		comp_chunk_size = compress_chunk(p_source, p_compressed, (unsigned int) chunk_size);
		fwrite(p_compressed, sizeof(uint8_t), comp_chunk_size, f_outfile);

		free(p_compressed);
		free(p_source);
	}
}
