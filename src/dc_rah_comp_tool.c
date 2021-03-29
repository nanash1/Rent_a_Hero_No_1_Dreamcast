/*
 ============================================================================
 Name        : dc_rah2_comp_tool.c
 Author      : nanashi
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
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
	printf("Usage: dc_rah_comp_tool.exe expand infile outfile chunks\n");
}

int main (int argc, char *argv[])
{
	FILE *f_in, *f_out;
	int mode = -1;

	if (argc < 3){
		_print_usage();
		return EXIT_FAILURE;
	} else {
		if (strcmp(argv[1], "compress") == 0){
			mode = 0;
		} else if (strcmp(argv[1], "expand") == 0){
			mode = 1;
		} else {
			_print_usage();
			return EXIT_FAILURE;
		}
	}

	if ((f_in=fopen(argv[2],"rb"))==NULL) {
		printf("Error opening input %s\n",argv[2]);
		return EXIT_FAILURE;
	}
	if ((f_out=fopen(argv[3],"wb"))==NULL) {
		printf("Error opening input %s\n",argv[3]);
		return EXIT_FAILURE;
	}

	if (mode == 0){
		compress(f_in, f_out, 10);
	} else if (mode == 1){
		expand_chunks(f_in, f_out, argv[4]);
	}

	fclose(f_in);
	fclose(f_out);
	return EXIT_SUCCESS;
}
