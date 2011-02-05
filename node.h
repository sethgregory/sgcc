#ifndef _NODE_H
#define _NODE_H

#include <stdio.h>

/* Node types */
#define NODE_LIST			1
#define NODE_DECL			2
#define NODE_TYPE			3
#define	NODE_IDENTIFIER		4
#define	NODE_NUMBER			5
#define	NODE_UNARY_OP		6
#define	NODE_BINARY_OP		7
#define	NODE_STMT			8
#define	NODE_STMT_LIST		9
#define	NODE_NULL_STMT		10
#define NODE_EXPR_STMT		11

/* Subtypes */
#define GENERIC				1
#define FUNCTION_DEF		2
#define	FUNCTION_DECL		3
#define	FUNCTION_SPEC		4
#define	DECL_LIST			5
#define	STMT_LIST			6
#define FILESCOPE			7
#define	TOPLEVEL			8
#define PARAM_DECL			9
#define STRING_CONST		10
#define	CHAR_CONST			11
#define RESERVED_WORD		12
#define IF_STMT				13
#define IF_ELSE_STMT		14
#define ELSE_STMT			15
#define FOR_STMT			16
#define WHILE_STMT			17
#define DOWHILE_STMT		18
#define ARRAY_DECL			19
#define SUBSCRIPT_EXPR		20
#define DECL				21
#define FUNCTION_CALL		22
#define PARAM_LIST			23
#define PTR_DECL			24
#define ABSTRACT_DECL		25
#define COND_STMT			26
#define LIST_BLOCK			27
#define IDENTIFIER_LIST		28
#define FOR_PARAM			29
#define EXPR_LIST			30
#define EXPR_ITEM			31

/* Structs */

typedef struct node {
	int type;
	int subtype;
	int nodenum;
	struct node *parent, *left, *right;
	int linenumber;
	int const_type;
	int const_value;
	char *string_data;
	struct symbol *symbol;
	char visited;
} node;

typedef struct nodelist {
	struct node* current;
	struct nodelist* next;
} nodelist;


/* Function prototypes */

char *lookup_type(char*, int);
char *lookup_subtype(char*, int);
void insert_cast_node(node*);
int type_greaterequalto_int(char[]);
void typecheck_tree(node*);
int is_below_subtype(node*, int);
int compare_nodes(node*, node*);
int compare_subtrees(node*, node*);
void build_symbol_tables(node*);
void build_symbol_tables_recursive(node*);
void print_symbol_tables(void);
node *create_node(int);
void start_new_block(node*);
void end_new_block(void);
int is_end_of_block(node*);
node *find_rightmost_subnode(node*);
void print_block_tabs(void);
void print_leftparen(node*);
void print_rightparen(node*);
void print_tree(node*);
void print_tree_code(node*);
node *malloc_list_node(node*, node*, int);
node *malloc_identifier_node(char*, node*, node*);
node *malloc_type_node(unsigned long, node*, node*);
node *malloc_unary_op_node(char*, node*, node*);
node *malloc_binary_op_node(char*, node*, node*);
node *malloc_number_node(unsigned long);
node *malloc_stmt_node(char*, int);
node *malloc_expr_node(node*, node*);
node *malloc_null_node(void);
char *lookup_subtype(char*, int);
char *lookup_type(char*, int);

void process_tree(node*);


#endif
