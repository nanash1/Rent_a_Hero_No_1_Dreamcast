/*
 * dc_rah2_comp_tool.h
 *
 *  Created on: Mar 29, 2021
 *      Author: nanashi
 */

#ifndef DC_RAH_COMP_TOOL_H_
#define DC_RAH_COMP_TOOL_H_

#include <stdint.h>

/**
 * @brief	Expands chunks from *.CGD file
 * @param	f_infile		Input File.
 * @param	mode			Mode selection string. "start:end" or "all".
 * @return 	Status
 */
int expand_files(FILE* f_infile, const char* mode, const char* folder);

void compress_file(FILE* f_infile, FILE* f_outfile, size_t chunk_size);
int compress_folder(FILE* fcompressed, const char* dir);

#endif /* DC_RAH_COMP_TOOL_H_ */
