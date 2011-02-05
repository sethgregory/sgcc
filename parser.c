#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "node.h"

FILE *yyin;
int yyparse();
extern int yynerrs;
node *root_node;
FILE *error_output;
FILE *out;
int is_verbose;

int main(int argc, char **argv) {
  FILE *input;

  is_verbose=0;
  int result;
  /* figure out whether we're using stdin/stdout or file in/file out */
  if (argc < 2 || !strcmp("-", argv[1])) {
    input = stdin;
    out = fopen( "mips.s", "w" );
  } else {
		/* We have an inputfile */
		input = fopen(argv[1], "r");

		/* Set outputfile to have same name with .s extension */
		char *outfilename = (char*)malloc(100);
		sprintf(outfilename, "%s.s", strtok(argv[1], "."));
		out = fopen(outfilename, "w");
		free(outfilename);

		/* If we have more than 2 params, check to see if 3rd is -v */
		if(argc > 2) {
			if(!strcmp(argv[2], "-v")){
				is_verbose=1;
			}
		}
  }
  
  /* tell lex where to get input */
  yyin = input;
  error_output = stderr;
  
  result = yyparse();
  
  if (yynerrs > 0) {
    result = 1;
  }
  switch (result) {
    case 0:
     break;
}
  return result;

}
