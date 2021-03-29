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

/**
 * @brief 	Expands a chunk from the compressed stream.
 * @param 	p_compressed			Pointer to compressed stream.
 * @param	p_expanded				Expanded data is written to this pointer.
 * @return	Nothing.
 */
static void expand_chunk(uint8_t* p_compressed, uint8_t* p_expanded, int size_expanded)
{
	/* Declare variables. */
	uint8_t rb_dict[4095];											// ring buffer dictionary from decoded data
	int pos_compressed = 0;											// position in compressed stream; first 4 bytes are the expanded size
	int pos_expanded = 0;											// position in the expanded stream
	int pos_dict = 4078;											// dictionary position; starts with 4078 zeros pre-loaded
	int rb_mask = 0xfff;											// ring buffer mask
	uint8_t data_byte, temp_byte1, temp_byte2, dict_byte;
	uint16_t ctrl_word;
	int i, cpy_len, cpy_pos;

	/* Initialize variables and fill the first 4078 entries
	 * of the dictionary with zeros. */
	ctrl_word = 0;													// the control word determines the number of bytes to copy
	for(i=0;i<4078;i++)
		rb_dict[i] = 0;

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
			rb_dict[pos_dict] = data_byte;
			pos_dict = (pos_dict+1) & rb_mask;
		} else {

			/* Read length-distance pair. */
			temp_byte1 = p_compressed[pos_compressed++];
			temp_byte2 = p_compressed[pos_compressed++];
			cpy_len = (int) (temp_byte2 & 0xf) + 2;
			cpy_pos = ((int) (temp_byte2& 0xf0)) << 4;
			cpy_pos |= (int) temp_byte1;

			/* Copy data from dictionary. */
			while (cpy_len-- > -1){
				cpy_pos &= rb_mask;
				dict_byte = rb_dict[cpy_pos++];
				p_expanded[pos_expanded++] = dict_byte;
				rb_dict[pos_dict] = dict_byte;
				pos_dict = (pos_dict+1) & rb_mask;
			}
		}
	}
}

/**
 * @brief	Expands chunks from *.CGD file
 * @param	f_infile		Input File.
 * @param	f_outfile		Output File.
 * @param	mode			Mode selection string. "start:end" or "all".
 * @return 	Nothing
 */
void expand_chunks(FILE* f_infile, FILE* f_outfile, const char* mode)
{
	int size_expanded;
	size_t chunk_start, chunk_end, i, start, end;
	uint32_t temp;
	uint8_t* p_compressed;
	uint8_t* p_expanded;

	/* Decode mode argument. */
	char* _mode = strdup(mode);
	char* str_start = strtok(_mode, ":");
	char* str_end = strtok(NULL, ":");

	if (strcmp(str_start, "all")){

		if (strlen(str_start) == 0)
			start = 0;
		else
			start = (size_t) atoi(str_start);

		if (strlen(str_start) == 0)
			end = 10;
		else
			end = (size_t) atoi(str_end);
	} else {
		start = 0;
		end = 10;
	}

	/* Seek first selected chunk. */
	fseek(f_infile, 0x870+start*8, SEEK_SET);
	fread(&temp, sizeof(uint32_t), 1, f_infile);
	chunk_start = (size_t) temp + 0x86c;

	for (i=start+1;i<end+1;i++){

		/* Seek next chunk to determine compressed size. */
		fseek(f_infile, 0x870+i*8, SEEK_SET);
		fread(&temp, sizeof(uint32_t), 1, f_infile);
		chunk_end = (size_t) temp + 0x86c;

		/* Read expanded size. */
		fseek(f_infile, chunk_start, SEEK_SET);
		fread(&temp, sizeof(uint32_t), 1, f_infile);
		size_expanded = (int) temp;

		/* Allocate memory. */
		p_compressed = malloc(chunk_end-chunk_start);
		fread(p_compressed, sizeof(uint8_t), chunk_end-chunk_start, f_infile);
		p_expanded = calloc(size_expanded, sizeof(uint8_t));

		/* Expand chunk and write to file. */
		expand_chunk(p_compressed, p_expanded, size_expanded);
		fwrite(p_expanded, sizeof(uint8_t), size_expanded, f_outfile);

		chunk_start = chunk_end;
		free(p_compressed);
		free(p_expanded);
	}
}
