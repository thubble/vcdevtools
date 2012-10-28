#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "parse.h"
#include "emit.h"

extern FILE* yyin;

int main(int argc, char** argv)
{
	// vcasm {-o outfile} asmfile
	char* outfile = "bootcode.bin";
	char* infile = NULL;
	int c;
	opterr = 0;
	while ((c = getopt (argc, argv, "o:")) != -1)
	{
		switch(c)
		{
		case 'o':
			outfile = optarg;
			break;
		default:
			abort();
		}
	}
	
	if (optind < argc)
		infile = argv[optind];
	if (infile == NULL)
	{
		printf("USAGE: vcasm {-o outfile} asmfile\n");
		return 1;
	}
	
	FILE* fIn = fopen(infile, "r");
	yyin = fIn;
	
	// parse infile
	cur_pc = 0;
	yyparse();
	fclose(fIn);
	
	// emit assembled bytecode
	unsigned char* out_mem = malloc(cur_pc);
	emit_data(out_mem);
	
	// write outfile
	FILE* fOut = fopen(outfile, "wb");
	fwrite(out_mem, sizeof(unsigned char), cur_pc, fOut);
	fclose(fOut);
	free(out_mem);
	return 0;
}
