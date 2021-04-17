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

static void inline _print_usage(const char* app)
{
	printf("\n");
	printf("Usage: %s compress folder outfile\n", app);
	printf("Usage: %s expand chunks infile [folder]\n", app);
}

int main (int argc, char *argv[])
{
	FILE* f_in;
	FILE* f_out;
	int mode = -1;
	int rtn = EXIT_SUCCESS;
	size_t slen;
	char sfolder[2048] = "";
	char sfile[2048] = "";
	char* ctrl;

	if (argc < 3){
		_print_usage(argv[0]);
		return EXIT_FAILURE;
	} else {
		if (strcmp(argv[1], "compress") == 0){
			mode = 0;
			strcpy(sfolder, argv[2]);
			strcpy(sfile, argv[3]);
		} else if (strcmp(argv[1], "expand") == 0){
			mode = 1;
			ctrl = argv[2];
			strcpy(sfile, argv[3]);
			if (argc > 4)
				strcpy(sfolder, argv[4]);
		} else {
			_print_usage(argv[0]);
			return EXIT_FAILURE;
		}
	}

	slen = strlen(sfolder);
	if (slen > 0){
		if (!(!strcmp(sfolder+slen-1, "/") || !strcmp(sfolder+slen-2, "\\"))){
			strcat(sfolder, "/");
		}
	}

	if ((mode == 1) && (f_in=fopen(sfile,"rb"))==NULL) {
		printf("Error opening input %s\n",sfile);
		return EXIT_FAILURE;
	}

	if ((mode == 0) && (f_out=fopen(sfile,"wb"))==NULL) {
		printf("Error opening output %s\n",sfile);
		return EXIT_FAILURE;
	}

	if (mode == 0){
		compress_folder(f_out, sfolder);
		fclose(f_out);
	} else if (mode == 1){
		rtn = expand_files(f_in, ctrl, sfolder);
		fclose(f_in);
	}

	return rtn;
}
