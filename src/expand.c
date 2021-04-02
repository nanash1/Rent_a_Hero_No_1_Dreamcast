/*
 * expand.c
 *
 *  Created on: Mar 29, 2021
 *      Author: nanashi
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "buffer.h"

#define CHUNK_BASE 0x86c							/* Base pointer of chunk table */

typedef struct {
	uint32_t start;									/* Chunk start address. */
	uint32_t length;								/* Chunk size in bytes. */
} chunk_t;

typedef struct {
	FILE* file;										/* File pointer. */
	size_t pos;										/* Current position in chunks. */
	size_t fsize;									/* File size. */
	size_t chnk_num;								/* Number of chunks. */
} chunk_table_t;

/**
 * @brief	Initialize chunk table structure.
 * @param	file		Pointer to *.CGD file
 * @return	Chunk table structure.
 */
static chunk_table_t chnktbl_init(FILE* file)
{
	uint32_t first_offset;

	chunk_table_t rtn;
	rtn.file = file;
	rtn.pos = 0;

	fseek(file, 0, SEEK_END);
	rtn.fsize = ftell(file);
	fseek(file, 0, SEEK_SET);

	fseek(file, CHUNK_BASE+4, SEEK_SET);
	fread(&first_offset, sizeof(uint32_t), 1, file);
	rtn.chnk_num = first_offset / 8;

	return rtn;
}

/**
 * @brief	Get next chunk from chunk table.
 * @param	chunk_table		Pointer to chunk table structure.
 * @param	eof				When end of current file is reached a 1 is written to this variable.
 * @return	Chunk information structure.
 */
static chunk_t chnktbl_next_chunk(chunk_table_t* chunk_table, int* eof)
{
	chunk_t rtn;

	uint32_t chunk_idx;
	uint32_t chunk_pos;
	fseek(chunk_table->file, CHUNK_BASE+(chunk_table->pos*8), SEEK_SET);
	fread(&chunk_idx, sizeof(uint32_t), 1, chunk_table->file);
	fread(&chunk_pos, sizeof(uint32_t), 1, chunk_table->file);
	chunk_table->pos++;
	rtn.start = chunk_pos+CHUNK_BASE;

	*eof = chunk_idx == 1;

	if (chunk_table->pos > chunk_table->chnk_num){
		rtn.length = chunk_table->fsize - rtn.start;
	} else {
		fread(&chunk_idx, sizeof(uint32_t), 1, chunk_table->file);
		fread(&chunk_pos, sizeof(uint32_t), 1, chunk_table->file);
		rtn.length = chunk_pos + CHUNK_BASE - rtn.start;
	}

	return rtn;
}

/**
 * @brief	Seeks file index in chunk table.
 * @param	chunk_table		Pointer to chunk table structure.
 * @param	file_idx		Index of the file.
 * @return	Nothing.
 */
static void chnktbl_file_seek(chunk_table_t* chunk_table, int file_idx)
{
	uint32_t chunk_idx;
	uint32_t chunk_pos;
	chunk_table->pos = 0;
	fseek(chunk_table->file, CHUNK_BASE, SEEK_SET);
	fread(&chunk_idx, sizeof(uint32_t), 1, chunk_table->file);
	fread(&chunk_pos, sizeof(uint32_t), 1, chunk_table->file);

	while (file_idx){
		chunk_table->pos++;
		if (chunk_table->pos > chunk_table->chnk_num)
			break;
		if (chunk_idx == 1)
			file_idx--;
		fread(&chunk_idx, sizeof(uint32_t), 1, chunk_table->file);
		fread(&chunk_pos, sizeof(uint32_t), 1, chunk_table->file);
	}
}

/**
 * @brief	Counts the total number of files in the chunk table.
 * @param	chunk_table		Pointer to chunk table structure.
 * @return	Number of files.
 */
static int chnktbl_cnt_file(chunk_table_t* chunk_table)
{
	int rtn = 0;
	int pos = 0;
	uint32_t chunk_idx;
	uint32_t chunk_pos;
	fseek(chunk_table->file, CHUNK_BASE, SEEK_SET);
	fread(&chunk_idx, sizeof(uint32_t), 1, chunk_table->file);
	fread(&chunk_pos, sizeof(uint32_t), 1, chunk_table->file);
	while (pos < chunk_table->chnk_num){
		pos++;
		if (chunk_idx == 1){
			rtn++;
		}
		fread(&chunk_idx, sizeof(uint32_t), 1, chunk_table->file);
		fread(&chunk_pos, sizeof(uint32_t), 1, chunk_table->file);
	}
	return rtn;
}

/**
 * @brief 	Expands a chunk from the compressed stream.
 * @param 	p_compressed			Pointer to compressed stream.
 * @param	p_expanded				Expanded data is written to this pointer.
 * @return	Nothing.
 */
static void expand_chunk(uint8_t* p_compressed, uint8_t* p_expanded, int size_expanded)
{
	/* Declare variables. */
	ringb_t dict = ringb_init();
	int pos_compressed = 0;											// position in compressed stream; first 4 bytes are the expanded size
	int pos_expanded = 0;											// position in the expanded stream
	uint8_t data_byte, temp_byte1, temp_byte2, dict_byte;
	uint16_t ctrl_word;
	int cpy_len, cpy_pos;
	ctrl_word = 0;													// the control word determines the number of bytes to copy

	while(pos_expanded < size_expanded){
		ctrl_word >>= 1;											// shift out lsb of control word

		/* Load a new control word every 8 bytes. */
		if (!(ctrl_word & 0x100)){
			ctrl_word = (uint16_t) p_compressed[pos_compressed++];
			ctrl_word |= 0xff00;
		}

		/* If lsb bit flag of the control is set copy a byte
		 * from the input to the output stream. If not then
		 * read length-distance pair and copy data from the
		 * dictionary. */
		if (ctrl_word & 0x1){
			data_byte = p_compressed[pos_compressed++];
			p_expanded[pos_expanded++] = data_byte;
			ringb_insert(&dict, data_byte);
		} else {

			/* Read length-distance pair. */
			temp_byte1 = p_compressed[pos_compressed++];
			temp_byte2 = p_compressed[pos_compressed++];
			cpy_len = (int) (temp_byte2 & 0xf) + 2;
			cpy_pos = ((int) (temp_byte2& 0xf0)) << 4;
			cpy_pos |= (int) temp_byte1;

			/* Copy data from dictionary. */
			while (cpy_len-- > -1){
				dict_byte = ringb_get(&dict, cpy_pos++);
				p_expanded[pos_expanded++] = dict_byte;
				ringb_insert(&dict, dict_byte);
			}
		}
	}
}

/**
 * @brief	Expands chunks from *.CGD file
 * @param	f_infile		Input File.
 * @param	mode			Mode selection string. "start:end" or "all".
 * @return 	Status
 */
int expand_files(FILE* f_infile, const char* mode, const char* folder)
{
	FILE* f_outfile;
	char fname[2060];
	int eof = 0;
	uint32_t size_expanded;
	int start, end;
	chunk_t chunk;
	uint8_t* p_compressed;
	uint8_t* p_expanded;

	chunk_table_t chunk_table = chnktbl_init(f_infile);

	/* Decode mode argument. */
	char* _mode = strdup(mode);
	char* str_start = strtok(_mode, ":");
	char* str_end = strtok(NULL, ":");
	if (strcmp(str_start, "all")){

		if (strlen(str_start) == 0)
			start = 0;
		else
			start = atoi(str_start);

		if (strlen(str_start) == 0)
			end = 10;
		else
			end = atoi(str_end);
	} else {
		start = 0;
		end = chnktbl_cnt_file(&chunk_table);
	}

	chnktbl_file_seek(&chunk_table, start);

	for (int image = start; image < end; image++){
		sprintf(fname, "%s%04d.PVR", folder, image);

		if ((f_outfile=fopen(fname,"wb"))==NULL) {
			printf("Error opening output %s. Make sure the target folder exists.\n",fname);
			return EXIT_FAILURE;
		}

		printf("Expanding %s\n", fname);

		while (!eof){
			chunk = chnktbl_next_chunk(&chunk_table, &eof);

			/* Read expanded size. */
			fseek(f_infile, chunk.start, SEEK_SET);
			fread(&size_expanded, sizeof(uint32_t), 1, f_infile);

			/* Allocate memory. */
			p_compressed = malloc(chunk.length);
			fread(p_compressed, sizeof(uint8_t), chunk.length, f_infile);
			p_expanded = calloc((size_t) size_expanded, sizeof(uint8_t));

			/* Expand chunk and write to file. */
			expand_chunk(p_compressed, p_expanded, (int) size_expanded);
			fwrite(p_expanded, sizeof(uint8_t), size_expanded, f_outfile);

			free(p_compressed);
			free(p_expanded);
		}

		eof = 0;
		fclose(f_outfile);
	}
	return EXIT_SUCCESS;
}
