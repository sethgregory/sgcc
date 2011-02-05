/* This file contains defines and a function to be used as a clever hack shown to us by Daniel Willenson
 * to retrieve the names of enumerated types as string values
 */

#include <stdio.h>
#include <string.h>
#include "irnode.h"

char *opcode_names[100];

/*
 * get_token_name - get the name of a token returned by yylex()
 *
 * Parameters:
 *  name - char * - token name will be copied here
 *  token - int - the token returned by yylex()
 *
 * Return: none
 * Side effects: none
 *
 */
void get_opcode_name(char *name, operation opcode) {
  strcpy(name, opcode_names[opcode]);
}

/* map enumerated types back to strings */
#define ADD_NAME(ARRAY, TOKEN) ARRAY[TOKEN] = #TOKEN

/*
 * init_names - set up the mappings from enumerated values to strings for tokens, reserved words,
 *              operators, number types, and number errors. must be called before any get_*_name is called.
 *
 * Parameters: none
 *
 * Return: none
 *
 * Side effects: the tables mapping enumerated values to strings are initialized, so that get_*_name methods above
 *                can be called.
 */
void init_names() {

  ADD_NAME(opcode_names, OP_LABEL);
  ADD_NAME(opcode_names, OP_FUNCTIONSTART);
  ADD_NAME(opcode_names, OP_FUNCTIONCALL);
  ADD_NAME(opcode_names, OP_PARAM);

  ADD_NAME(opcode_names, OP_ADD);
  ADD_NAME(opcode_names, OP_SUB);
  ADD_NAME(opcode_names, OP_MULT);
  ADD_NAME(opcode_names, OP_DIV);
  ADD_NAME(opcode_names, OP_MOD);

  ADD_NAME(opcode_names, OP_ADDI);

  ADD_NAME(opcode_names, OP_LA);
  ADD_NAME(opcode_names, OP_LIW);
  ADD_NAME(opcode_names, OP_LI);
  ADD_NAME(opcode_names, OP_SIW);
  ADD_NAME(opcode_names, OP_MOVE);

  ADD_NAME(opcode_names, OP_SLT);
  ADD_NAME(opcode_names, OP_SLTE);
  ADD_NAME(opcode_names, OP_SGT);
  ADD_NAME(opcode_names, OP_SGTE);
  ADD_NAME(opcode_names, OP_SEQ);

  ADD_NAME(opcode_names, OP_BITAND);
  ADD_NAME(opcode_names, OP_BITOR);

  ADD_NAME(opcode_names, OP_BLEZ);
  ADD_NAME(opcode_names, OP_BGZ);
  ADD_NAME(opcode_names, OP_J);
  ADD_NAME(opcode_names, OP_JR);

  ADD_NAME(opcode_names, OP_DO);
  ADD_NAME(opcode_names, OP_WHILE);
  ADD_NAME(opcode_names, OP_FOR);
  ADD_NAME(opcode_names, OP_IF);
  ADD_NAME(opcode_names, OP_ELSE);

  ADD_NAME(opcode_names, OP_EOS);



}
