%verbose 
%error-verbose

%{
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "node.h"
#include "symbol.h"

#define YYSTYPE unsigned long

extern int yylineno;
int yylex (void);

#define YYERROR_VERBOSE
void yyerror (char *s)
{
  fprintf (stderr, "Error at line %d is: %s\n", yylineno, s);
}

%}


/* constant types */
%token IDENTIFIER NUMBER STRING CHARACTER

/* reserved words */
%token RETURN CHAR INT LONG SHORT SIGNED UNSIGNED VOID
%token DO WHILE FOR BREAK IF ELSE GOTO CONTINUE
	
/* operators */ 
%token	LEFT_PAREN RIGHT_PAREN LEFT_BRACKET RIGHT_BRACKET LEFT_BRACE RIGHT_BRACE
%token	INCREMENT DECREMENT PLUS MINUS ASTERISK SLASH REMAINDER
%token	RELOP_LT RELOP_LE RELOP_GT RELOP_GE RELOP_EQ RELOP_NE
%token	BITWISEOR BITWISEXOR BITWISEAND BITWISENEG LEFTSHIFT RIGHTSHIFT
%token	LOGICALOR LOGICALAND LOGICALNEG
%token	ASSIGN_SIMPLE ASSIGN_ADD ASSIGN_SUBTRACT ASSIGN_MULTIPLY ASSIGN_DIVIDE ASSIGN_REMAINDER
%token	ASSIGN_LEFTSHIFT ASSIGN_RIGHTSHIFT ASSIGN_BITWISEAND ASSIGN_BITWISEXOR ASSIGN_BITWISEOR
%token	CONDQUEST CONDCOLON SEMICOLON COMMA

%left PLUS MINUS
%left ASTERISK SLASH

%nonassoc IF
%nonassoc ELSE

%start file_scope

%%


abstract_declarator
		: pointer
		| pointer direct_abstract_declarator
		| direct_abstract_declarator
		;
							
additive_expr
		: multiplicative_expr
		| additive_expr add_op multiplicative_expr						{ $$ = (YYSTYPE) malloc_binary_op_node((char *)$2, (node *)$1, (node *)$3); }
		;

add_op 
		: PLUS											{ $$ = (YYSTYPE) "+"; }
		| MINUS											{ $$ = (YYSTYPE) "-"; }
		;

address_expr 
		: BITWISEAND cast_expr 																{ $$ = (YYSTYPE) malloc_unary_op_node("&", NULL, (node *)$2); }
		;
							
array_declarator
		: direct_declarator LEFT_BRACKET constant_expr RIGHT_BRACKET		{ $$ = (YYSTYPE) malloc_list_node((node *)$1, (node *)$3, ARRAY_DECL); }
		| direct_declarator LEFT_BRACKET RIGHT_BRACKET									{ $$ = (YYSTYPE) malloc_list_node((node *)$1, NULL, ARRAY_DECL); }
		;
									
assignment_expr
		: conditional_expr
		| unary_expr assignment_op assignment_expr						{ $$ = (YYSTYPE) malloc_binary_op_node((char *)$2, (node *)$1, (node *)$3); }
		;

assignment_op
		: ASSIGN_SIMPLE							{ $$ = (YYSTYPE) "="; }
		| ASSIGN_ADD								{ $$ = (YYSTYPE) "+="; }
		| ASSIGN_SUBTRACT						{ $$ = (YYSTYPE) "-="; }
		| ASSIGN_MULTIPLY						{ $$ = (YYSTYPE) "*="; }
		| ASSIGN_DIVIDE							{ $$ = (YYSTYPE) "/="; }
		| ASSIGN_REMAINDER					{ $$ = (YYSTYPE) "%="; }
		| ASSIGN_LEFTSHIFT					{ $$ = (YYSTYPE) "<<="; }
		| ASSIGN_RIGHTSHIFT					{ $$ = (YYSTYPE) ">>="; }
		| ASSIGN_BITWISEAND					{ $$ = (YYSTYPE) "&="; }
		| ASSIGN_BITWISEXOR					{ $$ = (YYSTYPE) "^="; }
		| ASSIGN_BITWISEOR					{ $$ = (YYSTYPE) "|="; }
		;

bitwise_and_expr
		: equality_expr
		| bitwise_and_expr BITWISEAND equality_expr						{ $$ = (YYSTYPE) malloc_binary_op_node("&", (node *)$1, (node *)$3); }
		;

bitwise_negation_expr
		: BITWISENEG cast_expr																{ $$ = (YYSTYPE) malloc_unary_op_node("~", NULL, (node *)$2); }
		;

bitwise_or_expr
		: bitwise_xor_expr
		| bitwise_or_expr BITWISEOR bitwise_xor_expr					{ $$ = (YYSTYPE) malloc_binary_op_node("|", (node *)$1, (node *)$3); }
		;

bitwise_xor_expr
		: bitwise_and_expr
		| bitwise_xor_expr BITWISEXOR bitwise_and_expr				{ $$ = (YYSTYPE) malloc_binary_op_node("^", (node *)$1, (node *)$3); }
		;

break_statement
		: BREAK SEMICOLON																			{ $$ = (YYSTYPE) malloc_expr_node(malloc_stmt_node("break", RESERVED_WORD), NULL); }
		;

cast_expr
		: unary_expr
		| LEFT_PAREN type_name RIGHT_PAREN cast_expr					{ $$ = (YYSTYPE) malloc_binary_op_node("cast", (node *)$2, (node *)$4); }
		;

character_type_specifier
		: CHAR											{ $$ = (YYSTYPE) malloc_type_node(CHAR, NULL, NULL); }
		| SIGNED CHAR								{ $$ = (YYSTYPE) malloc_type_node(CHAR, malloc_type_node(SIGNED, NULL, NULL), NULL); }
		| UNSIGNED CHAR							{ $$ = (YYSTYPE) malloc_type_node(CHAR, malloc_type_node(UNSIGNED, NULL, NULL), NULL); }
		;

comma_expr
		: assignment_expr
		| comma_expr COMMA assignment_expr			{ $$ = (YYSTYPE) malloc_list_node((node *)$1, (node *)$3, GENERIC); }
		;

compound_statement
		: LEFT_BRACE declaration_or_statement_list RIGHT_BRACE					{ $$ = (YYSTYPE) malloc_list_node((node *) $2, NULL, LIST_BLOCK); }
		| LEFT_BRACE RIGHT_BRACE																				{ $$ = (YYSTYPE) NULL; }
		;

conditional_expr
		: logical_or_expr
		| logical_or_expr CONDQUEST expr CONDCOLON conditional_expr			{ $$ = (YYSTYPE) malloc_list_node(
																																							malloc_list_node((node *) $1, (node *)$3, IF_STMT), 
																																							malloc_list_node((node *) $5, NULL, ELSE_STMT), 
																																							COND_STMT); }
		;

conditional_statement
		: if_statement
		| if_else_statement
		;

constant
		: NUMBER										{ $$ = (YYSTYPE) malloc_number_node($1); }
		| CHARACTER									{ $$ = (YYSTYPE) malloc_number_node($1); }
		| STRING										{ $$ = (YYSTYPE) malloc_stmt_node((char *)$1, STRING_CONST); }
		;
					
constant_expr
		: conditional_expr
		;

continue_statement
		: CONTINUE SEMICOLON				{ $$ = (YYSTYPE) malloc_expr_node(malloc_stmt_node("continue", RESERVED_WORD), NULL); }
		;

decl
		: declaration_specifiers declarator_list SEMICOLON							{ $$ = (YYSTYPE) malloc_list_node((node *)$1, (node *)$2, DECL); }
		;

declaration_or_statement
		: decl
		| statement
		;

declaration_or_statement_list
		: declaration_or_statement																			{ $$ = (YYSTYPE) malloc_list_node((node *)$1, NULL, DECL_LIST); }
		| declaration_or_statement_list declaration_or_statement				{ $$ = (YYSTYPE) $1;
																																			node *nptr = (node *)$1;
																																			while(nptr->right != NULL) nptr=nptr->right;
																																			((node *)nptr)->right = (node *) malloc_list_node((node *)$2, NULL, DECL_LIST); } 
		;

declaration_specifiers
		:  type_specifier
		;
												
declarator
		: pointer_declarator
		| direct_declarator
		;

declarator_list
		: declarator
		| declarator_list COMMA declarator															{ $$ = (YYSTYPE) malloc_list_node((node *)$1, (node *)$3, IDENTIFIER_LIST); } 
		;

direct_abstract_declarator
		: LEFT_PAREN abstract_declarator RIGHT_PAREN														{ $$ = (YYSTYPE) $2; }
		| direct_abstract_declarator LEFT_BRACKET constant_expr RIGHT_BRACKET		{ $$ = (YYSTYPE) malloc_list_node((node *)$1, (node *)$3, GENERIC); }
		| LEFT_BRACKET constant_expr RIGHT_BRACKET															{ $$ = (YYSTYPE) $2; }
		| direct_abstract_declarator LEFT_PAREN parameter_list RIGHT_PAREN			{ $$ = (YYSTYPE) malloc_list_node((node *)$1, (node *)$3, GENERIC); }
		| direct_abstract_declarator LEFT_PAREN RIGHT_PAREN											{ $$ = (YYSTYPE) $1; }
		| LEFT_PAREN parameter_list RIGHT_PAREN																	{ $$ = (YYSTYPE) $2; }
		| LEFT_PAREN RIGHT_PAREN
		;
														
direct_declarator
		: simple_declarator
		| LEFT_PAREN declarator RIGHT_PAREN															{ $$ = (YYSTYPE) $2; }
		| function_declarator
		| array_declarator
		;

do_statement
		: DO statement WHILE LEFT_PAREN expr RIGHT_PAREN SEMICOLON			{ $$ = (YYSTYPE) malloc_list_node((node *)$2, (node *)$5, DOWHILE_STMT);
																																			node *nptr = (node *) $$;
																																			if(nptr->left == NULL) {
																																				nptr->left = (node *)malloc_null_node();
																																			}
																																		}
		;

equality_expr
		: relational_expr
		| equality_expr equality_op relational_expr						{ $$ = (YYSTYPE) malloc_binary_op_node((char *)$2, (node *)$1, (node *)$3); }
		;

equality_op
		: RELOP_EQ									{ $$ = (YYSTYPE) "=="; }
		| RELOP_NE									{ $$ = (YYSTYPE) "!="; }
		;

expr
		: comma_expr
		;

expression_list
		: assignment_expr															{ $$ = (YYSTYPE) malloc_list_node((node *)$1, NULL, EXPR_ITEM); }
		| expression_list COMMA assignment_expr				{ $$ = (YYSTYPE) malloc_list_node((node *)$1, NULL, EXPR_LIST); 
																												node *nptr = (node *)$$;
																												while(nptr->right != NULL) nptr=nptr->right;
																												nptr->right = (node*)malloc_list_node((node *) $3, NULL, EXPR_ITEM);
																											
		
																									}
		;

expression_statement
		: expr SEMICOLON						{ $$ = (YYSTYPE) malloc_expr_node((node *) $1, NULL); }
		;
		

file_scope
		:	translation_unit									{ process_tree((node*) $1); }
		;

for_expr
		: LEFT_PAREN SEMICOLON SEMICOLON RIGHT_PAREN														{ $$ = (YYSTYPE) malloc_list_node(
																																								malloc_null_node(),
																																								malloc_list_node(malloc_null_node(), malloc_list_node(malloc_null_node(), NULL, FOR_PARAM), FOR_PARAM),
																																								FOR_PARAM); }
		| LEFT_PAREN SEMICOLON SEMICOLON expr RIGHT_PAREN												{ $$ = (YYSTYPE) malloc_list_node(
																																								malloc_null_node(),
																																								malloc_list_node(malloc_null_node(), malloc_list_node((node *)$4, NULL, FOR_PARAM), FOR_PARAM),
																																								FOR_PARAM); }
		| LEFT_PAREN SEMICOLON expr SEMICOLON RIGHT_PAREN												{ $$ = (YYSTYPE) malloc_list_node(
																																								malloc_null_node(),
																																								malloc_list_node((node *)$3, malloc_list_node(malloc_null_node(), NULL, FOR_PARAM), FOR_PARAM),
																																								FOR_PARAM); }
		| LEFT_PAREN SEMICOLON expr SEMICOLON expr RIGHT_PAREN									{ $$ = (YYSTYPE) malloc_list_node(
																																								malloc_null_node(),
																																								malloc_list_node((node *)$3, malloc_list_node((node *)$5, NULL, FOR_PARAM), FOR_PARAM),
																																								FOR_PARAM); }
		| LEFT_PAREN initial_clause SEMICOLON SEMICOLON RIGHT_PAREN							{ $$ = (YYSTYPE) malloc_list_node(
																																								(node *)$2,
																																								malloc_list_node(malloc_null_node(), malloc_list_node(malloc_null_node(), NULL, FOR_PARAM), FOR_PARAM),
																																								FOR_PARAM); }
		| LEFT_PAREN initial_clause SEMICOLON SEMICOLON expr RIGHT_PAREN				{ $$ = (YYSTYPE) malloc_list_node(
																																								(node *)$2,
																																								malloc_list_node(malloc_null_node(), malloc_list_node((node *)$5, NULL, FOR_PARAM), FOR_PARAM),
																																								FOR_PARAM); }
		| LEFT_PAREN initial_clause SEMICOLON expr SEMICOLON RIGHT_PAREN				{ $$ = (YYSTYPE) malloc_list_node(
																																								(node *)$2,
																																								malloc_list_node((node *)$4, malloc_list_node(malloc_null_node(), NULL, FOR_PARAM), FOR_PARAM),
																																								FOR_PARAM); }
		| LEFT_PAREN initial_clause SEMICOLON expr SEMICOLON expr RIGHT_PAREN		{ $$ = (YYSTYPE) malloc_list_node(
																																								(node *)$2,
																																								malloc_list_node((node *)$4, malloc_list_node((node *)$6, NULL, FOR_PARAM), FOR_PARAM),
																																								FOR_PARAM); }
		;

for_statement
		: FOR for_expr statement																								{ $$ = (YYSTYPE) malloc_list_node((node *)$2, (node *)$3, FOR_STMT); }
		;

function_call
		: postfix_expr LEFT_PAREN expression_list RIGHT_PAREN				{ $$ = (YYSTYPE) malloc_list_node((node *)$1, (node *)$3, FUNCTION_CALL); }
		| postfix_expr LEFT_PAREN RIGHT_PAREN												{ $$ = (YYSTYPE) malloc_list_node((node *)$1, NULL, FUNCTION_CALL); }
		;

function_declarator
		: direct_declarator LEFT_PAREN RIGHT_PAREN									{ $$ = (YYSTYPE) malloc_list_node((node *)$1, malloc_list_node(malloc_type_node(VOID, NULL, NULL), NULL, PARAM_DECL), FUNCTION_DECL); }
		| direct_declarator LEFT_PAREN parameter_list RIGHT_PAREN		{ $$ = (YYSTYPE) malloc_list_node((node *)$1, (node *)$3, FUNCTION_DECL); }
		;

function_definition
		: function_def_specifier compound_statement									{ $$ = (YYSTYPE) malloc_list_node((node *)$1, (node *)$2, FUNCTION_DEF); }
		;

function_def_specifier
		: declaration_specifiers declarator													{ $$ = (YYSTYPE) malloc_list_node((node *)$1, (node *)$2, FUNCTION_SPEC); }
		| declarator																					
		;

goto_statement
		: GOTO named_label SEMICOLON																{ $$ = (YYSTYPE) malloc_expr_node(malloc_stmt_node("goto", RESERVED_WORD), (node *)$2); }
		;

if_else_statement
		: IF LEFT_PAREN expr RIGHT_PAREN statement ELSE statement %prec ELSE	{ $$ = (YYSTYPE) malloc_list_node(
																																							malloc_list_node((node *) $3, (node *)$5, IF_STMT), 
																																							malloc_list_node((node *) $7, NULL, ELSE_STMT), 
																																							IF_ELSE_STMT); }
		;

if_statement
		: IF LEFT_PAREN expr RIGHT_PAREN statement %prec IF					{ $$ = (YYSTYPE) malloc_list_node((node *)$3, (node *)$5, IF_STMT); }
		;

indirection_expr
		: ASTERISK cast_expr																				{ $$ = (YYSTYPE) malloc_unary_op_node("*", NULL, (node *)$2); }
		;

initial_clause
		: expr
		| decl
		;
					
integer_type_specifier
		: signed_type_specifier				
		| unsigned_type_specifier				
		| character_type_specifier				
		;
                        
iterative_statement
		: while_statement
		| do_statement
		| for_statement
		;

label
		: named_label
		;

labeled_statement
		: label CONDCOLON statement																{ $$ = (YYSTYPE) malloc_binary_op_node(":", (node *)$1, (node *)$3); }
		;

logical_and_expr
		: bitwise_or_expr
		| logical_and_expr LOGICALAND bitwise_or_expr							{ $$ = (YYSTYPE) malloc_binary_op_node("&&", (node *)$1, (node *)$3); }
		;

logical_negation_expr
		: LOGICALNEG cast_expr																		{ $$ = (YYSTYPE) malloc_unary_op_node("!", NULL, (node *)$2); }
		;

logical_or_expr
		: logical_and_expr
		| logical_or_expr LOGICALOR logical_and_expr							{ $$ = (YYSTYPE) malloc_binary_op_node("||", (node *)$1, (node *)$3); }
		;

multiplicative_expr
		: cast_expr
		| multiplicative_expr mult_op cast_expr										{ $$ = (YYSTYPE) malloc_binary_op_node((char *)$2, (node *)$1, (node *)$3); }
		;

mult_op
		: ASTERISK									{ $$ = (YYSTYPE) "*"; }
		| SLASH											{ $$ = (YYSTYPE) "/"; }
		| REMAINDER     						{ $$ = (YYSTYPE) "%"; }
		;

named_label
		: IDENTIFIER																			{ $$ = (YYSTYPE) malloc_identifier_node((char *)$1, NULL, NULL); }
		;

null_statement
		: SEMICOLON																				{ $$ = (YYSTYPE) malloc_null_node(); }
		;

parameter_decl
		: declaration_specifiers declarator								{ $$ = (YYSTYPE) malloc_list_node((node *)$1, (node *)$2, PARAM_DECL); } 
		| declaration_specifiers abstract_declarator			{ $$ = (YYSTYPE) malloc_list_node((node *)$1, (node *)$2, PARAM_DECL); }
		| declaration_specifiers													{ $$ = (YYSTYPE) malloc_list_node((node *)$1, NULL, PARAM_DECL); }
		;

parameter_list
		: parameter_decl																
		| parameter_list COMMA parameter_decl							{ $$ = (YYSTYPE) malloc_list_node((node *)$1, (node *)$3, PARAM_LIST); } 
		;
					
parenthesized_expr
		: LEFT_PAREN expr RIGHT_PAREN											{ $$ = (YYSTYPE) $2; }
		;

pointer
		: ASTERISK																				{ $$ = (YYSTYPE) malloc_unary_op_node("*", NULL, NULL); }
		| ASTERISK pointer																{ $$ = (YYSTYPE) malloc_unary_op_node("*", NULL, (node *)$2); }
		;
				
pointer_declarator
		: pointer direct_declarator												{ $$ = (YYSTYPE) malloc_list_node((node *)$1, NULL, PTR_DECL); 
																												node *nptr = (node *)$1;
																												while(nptr->right != NULL) nptr=nptr->right;
																												nptr->right = (node *) $2;
																											} 
		;

postdecrement_expr
		: postfix_expr DECREMENT													{ $$ = (YYSTYPE) malloc_unary_op_node("--", (node *)$1, NULL); }
		;

postfix_expr
		: primary_expr
		| subscript_expr
		| function_call
		| postincrement_expr
		| postdecrement_expr
		;

postincrement_expr
		: postfix_expr INCREMENT													{ $$ = (YYSTYPE) malloc_unary_op_node("++", (node *)$1, NULL); }
		;

predecrement_expr
		: DECREMENT unary_expr														{ $$ = (YYSTYPE) malloc_unary_op_node("--", NULL, (node *)$2); }
		;
									
preincrement_expr
		: INCREMENT unary_expr														{ $$ = (YYSTYPE) malloc_unary_op_node("++", NULL, (node *)$2); }
		;

primary_expr
		: IDENTIFIER																			{ $$ = (YYSTYPE) malloc_identifier_node((char *)$1, NULL, NULL); }
		| constant
		| parenthesized_expr									
		;

relational_expr
		: shift_expr
		| relational_expr relational_op shift_expr				{ $$ = (YYSTYPE) malloc_binary_op_node((char *)$2, (node *)$1, (node *)$3); }
		;

relational_op
		: RELOP_LT									{ $$ = (YYSTYPE) "<"; }
		| RELOP_LE									{ $$ = (YYSTYPE) "<="; }
		| RELOP_GT									{ $$ = (YYSTYPE) ">"; }
		| RELOP_GE									{ $$ = (YYSTYPE) ">="; }
		;
		
return_statement
		: RETURN expr SEMICOLON														{ $$ = (YYSTYPE) malloc_expr_node(malloc_stmt_node("return", RESERVED_WORD), (node *)$2); }
		| RETURN SEMICOLON																{ $$ = (YYSTYPE) malloc_expr_node(malloc_stmt_node("return", RESERVED_WORD), NULL); }
		;

shift_expr
		: additive_expr
		| shift_expr shift_op additive_expr								{ $$ = (YYSTYPE) malloc_binary_op_node((char *)$2, (node *)$1, (node *)$3); }
		;

shift_op
		: LEFTSHIFT									{ $$ = (YYSTYPE) "<<"; }
		| RIGHTSHIFT								{ $$ = (YYSTYPE) ">>"; }
		;

signed_type_specifier
		: SHORT											{ $$ = (YYSTYPE) malloc_type_node(SHORT, NULL, NULL); }
		| SHORT INT									{ $$ = (YYSTYPE) malloc_type_node(INT, malloc_type_node(SHORT, NULL, NULL), NULL); }
		| SIGNED SHORT							{ $$ = (YYSTYPE) malloc_type_node(SHORT, malloc_type_node(SIGNED, NULL, NULL), NULL); }
		| SIGNED SHORT INT					{ $$ = (YYSTYPE) malloc_type_node(INT, malloc_type_node(SHORT, malloc_type_node(SIGNED, NULL, NULL), NULL), NULL); }
		| INT												{ $$ = (YYSTYPE) malloc_type_node(INT, NULL, NULL); }
		| SIGNED INT								{ $$ = (YYSTYPE) malloc_type_node(INT, malloc_type_node(SIGNED, NULL, NULL), NULL); }
		| SIGNED										{ $$ = (YYSTYPE) malloc_type_node(SIGNED, NULL, NULL); }
		| LONG											{ $$ = (YYSTYPE) malloc_type_node(LONG, NULL, NULL); }
		| LONG INT									{ $$ = (YYSTYPE) malloc_type_node(INT, malloc_type_node(LONG, NULL, NULL), NULL); }
		| SIGNED LONG								{ $$ = (YYSTYPE) malloc_type_node(LONG, malloc_type_node(SIGNED, NULL, NULL), NULL); }
		| SIGNED LONG INT						{ $$ = (YYSTYPE) malloc_type_node(INT, malloc_type_node(LONG, malloc_type_node(SIGNED, NULL, NULL), NULL), NULL); }
		;

simple_declarator
		: IDENTIFIER								{ $$ = (YYSTYPE) malloc_identifier_node((char *)$1, NULL, NULL); }
		;

statement
		: expression_statement
		| labeled_statement
		| compound_statement
		| conditional_statement
		| iterative_statement
		| break_statement
		| continue_statement
		| return_statement
		| goto_statement
		| null_statement
		;

subscript_expr
		: postfix_expr LEFT_BRACKET expr RIGHT_BRACKET			{ $$ = (YYSTYPE) malloc_list_node((node *)$1, (node *)$3, SUBSCRIPT_EXPR); }
		;
								
top_level_decl 
		: decl															
		| function_definition
		;

translation_unit 
		: top_level_decl										{ $$ = (YYSTYPE) malloc_list_node((node *)$1, NULL, FILESCOPE); }
		| translation_unit top_level_decl		{ $$ = (YYSTYPE) $1;
																					/* follow all the way to the right and add a new list node */
																					node *nptr = (node *)$1;
																					while(nptr->right != NULL) nptr=nptr->right;
																					((node *)nptr)->right = (node *) malloc_list_node((node *)$2, NULL, TOPLEVEL);
																					}
		;
		

type_name
		: declaration_specifiers abstract_declarator	{ $$ = (YYSTYPE) malloc_list_node((node *)$1, (node *)$2, ABSTRACT_DECL); }
		| declaration_specifiers
		;

type_specifier
		: integer_type_specifier
		| void_type_specifier
		;

unary_expr
		: postfix_expr
		| unary_minus_expr
		| unary_plus_expr
		| logical_negation_expr
		| bitwise_negation_expr
		| address_expr
		| indirection_expr
		| preincrement_expr
		| predecrement_expr
		;

unary_minus_expr
		: MINUS cast_expr									{ $$ = (YYSTYPE) malloc_unary_op_node("-", NULL, (node *)$2); }
		;

unary_plus_expr
		: PLUS cast_expr									{ $$ = (YYSTYPE) malloc_unary_op_node("+", NULL, (node *)$2); }							
		;

unsigned_type_specifier
		: UNSIGNED SHORT									{ $$ = (YYSTYPE) malloc_type_node(SHORT, malloc_type_node(UNSIGNED, NULL, NULL), NULL); }
		| UNSIGNED SHORT INT							{ $$ = (YYSTYPE) malloc_type_node(INT, malloc_type_node(SHORT, malloc_type_node(UNSIGNED, NULL, NULL), NULL), NULL); }
		| UNSIGNED												{ $$ = (YYSTYPE) malloc_type_node(UNSIGNED, NULL, NULL); }
		| UNSIGNED INT										{ $$ = (YYSTYPE) malloc_type_node(INT, malloc_type_node(UNSIGNED, NULL, NULL), NULL); }
		| UNSIGNED LONG										{ $$ = (YYSTYPE) malloc_type_node(LONG, malloc_type_node(UNSIGNED, NULL, NULL), NULL); }
		| UNSIGNED LONG INT								{ $$ = (YYSTYPE) malloc_type_node(INT, malloc_type_node(LONG, malloc_type_node(UNSIGNED, NULL, NULL), NULL), NULL); }
		;

void_type_specifier
		: VOID														{ $$ = (YYSTYPE) malloc_type_node(VOID, NULL, NULL); }
		;

while_statement
		: WHILE LEFT_PAREN expr RIGHT_PAREN statement			{ $$ = (YYSTYPE) malloc_list_node((node *)$3, (node *)$5, WHILE_STMT); }
		;

%%



