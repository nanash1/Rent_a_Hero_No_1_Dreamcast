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
 * @param	f_outfile		Output File.
 * @param	mode			Mode selection string. "start-end" or "all".
 * @return 	Nothing
 */
void expand_chunks(FILE* f_infile, FILE* f_outfile, const char* mode);
void compress(uint8_t* p_source, uint8_t* p_compressed, int size);

#endif /* DC_RAH_COMP_TOOL_H_ */
