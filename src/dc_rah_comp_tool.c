/*
 ============================================================================
 Name        : dc_rah2_comp_tool.c
 Author      : nanashi
 Version     :
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "dc_rah_comp_tool.h"

static void inline _print_usage(void)
{
	printf("\n");
	printf("Usage: dc_rah_comp_tool.exe compress infile outfile\n");
	printf("Usage: dc_rah_comp_tool.exe expand infile chunks\n");
}

int main (int argc, char *argv[])
{
	FILE* f_in;
	FILE* f_out;
	int mode = -1;
	int rtn = EXIT_SUCCESS;
	char* str_f_in;
	char* str_f_out;
	char* ctrl;

	if (argc < 3){
		_print_usage();
		return EXIT_FAILURE;
	} else {
		if (strcmp(argv[1], "compress") == 0){
			mode = 0;
			str_f_in = argv[2];
			str_f_out = argv[3];
		} else if (strcmp(argv[1], "expand") == 0){
			mode = 1;
			str_f_in = argv[3];
			ctrl = argv[2];
		} else {
			_print_usage();
			return EXIT_FAILURE;
		}
	}

	if ((f_in=fopen(str_f_in,"rb"))==NULL) {
		printf("Error opening input %s\n",str_f_in);
		return EXIT_FAILURE;
	}

	if ((mode == 0) && ((f_out=fopen(str_f_out,"wb"))==NULL)) {
		printf("Error opening output %s\n",str_f_out);
		fclose(f_in);
		return EXIT_FAILURE;
	}

	if (mode == 0){
		compress_file(f_in, f_out, 16400);
	} else if (mode == 1){
		rtn = expand_files(f_in, ctrl);
	}

	fclose(f_in);
	if (mode == 0)
		fclose(f_out);
	return rtn;
}
