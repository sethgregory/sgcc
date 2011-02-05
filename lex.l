%{

#include <stdio.h>
#include <stdlib.h>
#include "yacc.tab.h"

char *create_identifier(char *text, int length);
char *create_string(char *text, int length);
unsigned long create_number(char *text, int length);


%}

%option yylineno
%option noyywrap
%option nounput



newline         \n
ws              [ \t\v\f]

digit           [[:digit:]]
letter          [[:alpha:]]
nondigit        {letter}|_

comment					\/\*([^\*]|[\r\n]|(\*+([^\*\/]|[\r\n])))*\*+\/

charescape			\\[ntbrfv\\\'\"\?a]
octalescape			\\([0-7]{1,2}|[0-3][0-7]{2})
escapedchar			\'{charescape}\'|\'{octalescape}\'

id              {nondigit}({nondigit}|{digit})*

number          {digit}+
singlechar			\'[a-zA-Z\$\@\`]\'
string  				\"([^\"\\\n]|\\.)*\"

%%

{newline}   /* do nothing; lex is counting lines */
{ws}        /* do nothing */
{comment}		/* do nothing */

  /* reserved words begin */
return      return RETURN; 
char        return CHAR; 
int         return INT;
long        return LONG; 
short       return SHORT; 
signed      return SIGNED; 
unsigned    return UNSIGNED;
void				return VOID;
do					return DO;
while				return WHILE;
for					return FOR;
break				return BREAK;
if					return IF;
else				return ELSE;
goto				return GOTO;
continue		return CONTINUE;
  /* reserved words end */

  /* operators begin */
"("				return LEFT_PAREN;
")"				return RIGHT_PAREN;
"["				return LEFT_BRACKET;
"]"				return RIGHT_BRACKET;
"{"				return LEFT_BRACE;
"}"				return RIGHT_BRACE;

"++"			return INCREMENT;
"--"			return DECREMENT;
"+"				return PLUS;
"-"				return MINUS;
"*"				return ASTERISK;
"/"				return SLASH;
"%"				return REMAINDER;

"<"				return RELOP_LT;
"<="			return RELOP_LE;
">"				return RELOP_GT;
">="			return RELOP_GE;
"=="			return RELOP_EQ;
"!="			return RELOP_NE;

"|"				return BITWISEOR;
"^"				return BITWISEXOR;
"&"				return BITWISEAND;
"~"				return BITWISENEG;
"<<"			return LEFTSHIFT;
">>"			return RIGHTSHIFT;

"||"			return LOGICALOR;
"&&"			return LOGICALAND;
"!"				return LOGICALNEG;

"="				return ASSIGN_SIMPLE;
"+="			return ASSIGN_ADD;
"-="			return ASSIGN_SUBTRACT;
"*="			return ASSIGN_MULTIPLY;
"/="			return ASSIGN_DIVIDE;
"%="			return ASSIGN_REMAINDER;
"<<="			return ASSIGN_LEFTSHIFT;
">>="			return ASSIGN_RIGHTSHIFT;
"&="			return ASSIGN_BITWISEAND;
"^="			return ASSIGN_BITWISEXOR;
"|="			return ASSIGN_BITWISEOR;

"?"				return CONDQUEST;
":"				return CONDCOLON;
";"				return SEMICOLON;
","				return COMMA;

  /* operators end */

  /* constants begin */
  {number}    yylval = (YYSTYPE)create_number(yytext, yyleng); return NUMBER;
  /* constants end */

  /* identifiers */
  

{id}        	yylval = (YYSTYPE)create_identifier(yytext, yyleng); 	return IDENTIFIER;
{string}			yylval = (YYSTYPE)create_string(yytext, yyleng); 			return STRING;
{singlechar}	yylval = (YYSTYPE)create_number(yytext, yyleng);			return CHARACTER;
{escapedchar}	yylval = (YYSTYPE)create_number(yytext, yyleng);			return CHARACTER;

.           return -1;

%%


 /*
  * create_identifier - allocate memory and store the name of an identifier in it, for 
  *                     return to the calling program
  *
  * Parameters:
  *   text - string - contains the name of the identifier as matched in the source program
  *   length - integer - the length of text (not including terminating NUL)
  *
  * Returns:
  *   a NUL-terminated string in newly allocated memory, containing the text copied from the
  *   source program. returns NULL if memory could not be allocated.
  *
  * Side-effects:
  *   memory may be allocated on the heap.
  *
  */
char *create_identifier(char *text, int length) {
  char *id;

  id = malloc(length + 1);
  if (NULL != id) {
    strcpy(id, text);
  }
  return id;
}

 /*
  * create_number - read the value of a number from a string
  *
  * Parameters:
  *   text - string - contains the constant as matched in the source program
  *   length - integer - the length of text (not including terminating NUL)
  *
  * Returns:
  *   an unsigned long containing the value from text
  *
  */
unsigned long create_number(char *text, int length) {
  unsigned long n;
  
  if ('\'' != text[0]) {
    /* integer constant */
    errno = 0;

    n = strtoul(text, NULL, 10);
  
    if (ERANGE == errno || n > 4294967295ul) {
      /* integer constant was too large for unsigned long. value will be MAX_ULONG, as defined by strtoul */

      /* do something about overflow here! */
    	/* num_type = TYPE_OVERFLOW; */

    } else if (n > 2147483647) {
      /* the value has type unsigned int */
      /* num_type = TYPE_UNSIGNED_LONG; */

    } else if (n > 32767) {
      /* the value has type signed int */
      /* num_type = TYPE_SIGNED_LONG; */

    } else {
      /* the value has type signed short -- but everything is going to be a signed int/long here */
      /*num_type = TYPE_SIGNED_LONG; */
    }
  } else {
  		/* the value has type signed char */
  		/* num_type = TYPE_SIGNED_CHAR; */
  		
	  	if ('\\' != text[1]) {
 		  	/* character constant */
 		  	return (unsigned long)text[1];
			} else {
				
				/* some sort of escape char */
				
				if(text[2] >= 48 && text[2] <= 57) {
					/* octal value character escape */
					char *substr;
  				substr=strdup(text+2);
					n = strtoul(substr, NULL, 8);
					free(substr);
					
				} else {
					/* character escape */
					switch(text[2]) {
						case 'n':
							n = 10;
							break;
						case 't':
							n = 9;
							break;
						case 'b':
							n = 8;
							break;
						case 'r':
							n = 13;
							break;
						case 'f':
							n = 12;
							break;
						case 'v':
							n = 11;
							break;
						case '\\':
							n = 92;
							break;
						case '\'':
							n = 39;
							break;
						case '"':
							n = 34;
							break;
						case '?':
							n = 63;
							break;
						case 'a':
							n = 7;
							break;
					}
				}
			}
		}		
  
  return n;
}

 /*
  * create_string - allocate memory and store a string in it, for 
  *                     return to the calling program
  *
  * Parameters:
  *   text - string - contains the string as matched in the source program
  *   length - integer - the length of text (not including terminating NUL)
  *
  * Returns:
  *   a NUL-terminated string in newly allocated memory, containing the text copied from the
  *   source program. returns NULL if memory could not be allocated.
  *
  * Side-effects:
  *   memory may be allocated on the heap.
  *
  */
char *create_string(char *text, int length) {
  char *str;
  str = malloc(length + 1);

  if (NULL != str) {
	  char *sp1 = text;
	  char *sp2 = str;
	  
	  while(*sp1 != '\0'){
	  	
	  	if(*sp1 == '\"') {
	  		// do nothing, don't copy over quotes
	  	}
	  	else if(*sp1 == '\\') {
	  		
	  		/* this is terrible, but it works... */
	  		char temp_octal[4];
	  		temp_octal[3] = '\0';
	  		
	  		/* Check to see if the first character is numeric */
	  		if(*(sp1+1) >= 48 && *(sp1+1) <= 57){
	  			temp_octal[0] = *(sp1+1);
	  			sp1++;
	  		
	  				/* Check to see if the second character is numeric, else nullterm string */
			  		if(*(sp1+1) >= 48 && *(sp1+1) <= 57){
			  			temp_octal[1] = *(sp1+1);
			  			sp1++;
			  		} else {
			  			temp_octal[1] = '\0';
			  		}
			  		
	  				/* Check to see if the third character is numeric & size allows a third, else nullterm string */
			  		if(*(sp1-1) >= 48 && *(sp1-1) <= 51 && *(sp1+1) >= 48 && *(sp1+1) <= 57){
			  			temp_octal[2] = *(sp1+1);
			  			sp1++;
			  		} else {
			  			temp_octal[2] = '\0';
			  		}
			  		
			  	/* Write the char value into the new string */
			  	*sp2 = strtoul(temp_octal, NULL, 8);
			  	sp2++;
			  		
	  		}
	  		
	  		else {
	  		/* This is a single character escape sequence */
	  		
	  		/* After escape backslash, switch on the next character */
	  		switch(*(sp1+1)) {
						case 'n':
							*sp2 = 10;
							break;
						case 't':
							*sp2 = 9;
							break;
						case 'b':
							*sp2 = 8;
							break;
						case 'r':
							*sp2 = 13;
							break;
						case 'f':
							*sp2 = 12;
							break;
						case 'v':
							*sp2 = 11;
							break;
						case '\\':
							*sp2 = 92;
							break;
						case '\'':
							*sp2 = 39;
							break;
						case '"':
							*sp2 = 34;
							break;
						case '?':
							*sp2 = 63;
							break;
						case 'a':
							*sp2 = 7;
							break;
						default:
							/* just a slash */
							*sp2 = *sp1;
							sp1--;
							break;
							
					}
					sp1++;
					sp2++;
	  	}
	  }
	  	else {
	  		/* Just copy directly from string 1 into string 2 */
	  		*sp2 = *sp1;
	  		sp2++;
	  	}
	  	
			sp1++;
			
		}
		
		/* Be sure to null-terminate! */
		*sp2 = '\0';

  }

  return str;
}
