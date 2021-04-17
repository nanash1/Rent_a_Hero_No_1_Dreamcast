/*
 * compress.c
 *
 *  Created on: Mar 29, 2021
 *      Author: nanashi
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <wchar.h>
#include <windows.h>
#include "buffer.h"


typedef struct {
	int len;
	int pos;
} match_t;

struct file_size {
	int compressed;
	int chunks;
};

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
	while (1){
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

		if (match_dict.tail == dict.tail)
			break;

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
	if (cntr == 8)
		p_compressed--;
	else
		*p_ctrl_word = (uint8_t) ((ctrl_word >> cntr) & 0xff);
	padding = (p_compressed - p_comp_base) % 4;
	padding = (padding > 0) ? 4 - padding : 0;
	while (padding--)
		*p_compressed++ = 0;

	return p_compressed - p_comp_base;
}

struct file_size compress_file(FILE* f_infile, uint8_t* p_compressed,  uint32_t* p_table, size_t chunk_size)
{
	struct file_size rtn;
	rtn.compressed = 0;
	int num_chunks, i;
	int comp_chunk_size, last_cchunk_size;
	size_t f_size;
	uint8_t* p_source;

	fseek(f_infile, 0, SEEK_END);
	f_size = ftell(f_infile);
	fseek(f_infile, 0, SEEK_SET);

	num_chunks = (int) (f_size/chunk_size);
	last_cchunk_size = f_size % chunk_size;
	if (last_cchunk_size)
		num_chunks++;
	else
		last_cchunk_size = chunk_size;

	for (i=num_chunks;i>0;i--){

		if (i == 1)
			chunk_size = last_cchunk_size;

		*p_table++ = (uint32_t) i;

		/* Seek next chunk to determine compressed size. */
		p_source = malloc(chunk_size);
		fread(p_source, sizeof(uint8_t), chunk_size, f_infile);

		comp_chunk_size = compress_chunk(p_source, p_compressed, (unsigned int) chunk_size);
		p_compressed += comp_chunk_size;
		rtn.compressed += comp_chunk_size;
		*p_table++ = (uint32_t) comp_chunk_size;

		free(p_source);
	}

	rtn.chunks = num_chunks;
	return rtn;
}

int compress_folder(FILE* fcompressed, const char* dir)
{
	int num_files = 0;
	int num_chunks = 0;
	int comp_size = 0;
	struct file_size fcomp_size;
	size_t header_size;
	FILE* fheader;
	FILE* fimage;
	HANDLE hFind;
	WIN32_FIND_DATA FindFileData;
	char sPath[2060];
	char sfile[2060];
	sprintf(sPath, "%s*.PVR", dir);
	uint8_t* p_header;
	uint8_t* p_compressed;
	uint32_t* p_table_sizes;
	uint32_t* p_table;

	/* Count number of files in source folder. */
    if((hFind = FindFirstFile(sPath, &FindFileData)) == INVALID_HANDLE_VALUE)
    {
        printf("Path not found: [%s]\n", dir);
        return EXIT_FAILURE;
    }
    num_files = 1;
	while(FindNextFile(hFind, &FindFileData)){
		num_files++;
	}
	hFind = FindFirstFile(sPath, &FindFileData);

	/* Reader "header.bin". */
	if ((fheader=fopen("header.bin","rb"))==NULL) {
		printf("header.bin is missing\n");
		FindClose(hFind);
		return EXIT_FAILURE;
	}
	fseek(fheader,0,SEEK_END);
	header_size = ftell(fheader);
	p_header = malloc(header_size);
	fseek(fheader,0,SEEK_SET);
	fread(p_header, sizeof(uint8_t), header_size, fheader);
	fclose(fheader);
	fwrite(p_header, sizeof(uint8_t), header_size, fcompressed);
	free(p_header);

	/* Allocate memory for pointer table and compressed data. */
	p_table_sizes = malloc((num_files+1)*8*10);
	p_compressed = malloc(20971520);

	do {
		sprintf(sfile, "%s%s", dir, FindFileData.cFileName);
		printf("Compressing %s\n", FindFileData.cFileName);
		fimage = fopen(sfile,"rb");
		fcomp_size = compress_file(fimage, p_compressed+comp_size, p_table_sizes+(num_chunks*2), 16400);
		comp_size += fcomp_size.compressed;
		num_chunks += fcomp_size.chunks;
		fclose(fimage);

	} while (FindNextFile(hFind, &FindFileData));

	/* Write pointer table. */
	p_table = malloc(num_chunks*8);
	p_table[0] = p_table_sizes[0];
	p_table[1] = num_chunks*8;
	for (int i = 1; i < num_chunks;i++){
		p_table[2*i] = p_table_sizes[2*i];
		p_table[2*i+1] = p_table[2*i-1] + p_table_sizes[2*i-1];
	}

	/* Write data to file. */
	fwrite(p_table, sizeof(uint8_t), num_chunks*8, fcompressed);
	fwrite(p_compressed, sizeof(uint8_t), comp_size, fcompressed);

	free(p_table);
	free(p_table_sizes);
	free(p_compressed);

	FindClose(hFind);
    return num_files;
}
