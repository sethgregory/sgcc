#ifndef _SYMBOL_H
#define _SYMBOL_H

#include <stdio.h>

#define TYPEARRAY_SIZE	10

/* the array for type:  [void] [signed] [unsigned] [char] [short] [long] [int] [is_pointer] */
#define	BIT_VOID			0
#define	BIT_SIGNED			1
#define	BIT_UNSIGNED		2
#define BIT_CHAR			3
#define	BIT_SHORT			4
#define	BIT_LONG			5
#define BIT_INT				6
#define BIT_POINTER			7
#define POINTER_DEPTH		8
#define IS_FUNCTION			9

/* The functioninfo struct contains information specifically about functions and their status */
typedef struct functioninfo {
	struct symbol *tablefor;
	struct node *declnode;
	char is_prototyped;
	char is_defined;	
	char params_inserted;
} functioninfo;

/* A symboltable has a single parent, a list of children, a link to the symbol for its function if applicable,
 * a name, a list of symbols, and an int value expressing the total size needed to store all its symbols
 */
typedef struct symboltable {
	struct symboltable *parent;
	struct tablelist *children;
	struct symbol *tablefor;
	char *name;
	struct symbol *symbol;
	int sizeofsymbols;
} symboltable;

/* A tablelist is simply a list of symbol tables */
typedef struct tablelist {
	struct symboltable *table;
	struct tablelist *next;
} tablelist;

/* A symbol has a number of attributes, including a name, a bitmap array representing its type (see above),
 * a size, which for functions can represent how many parameters it takes, an offset (determined during code generation),
 * a link to the tree node which created it, a list representing its array dimensions, a list of parameters, a functioninfo
 * object if it is a function, and a link to the next symbol in the list.
 */
typedef struct symbol {
	char *name;
	char type[TYPEARRAY_SIZE];
	int size;
	int offset;
	struct node *node;
	struct arraylist *arrayidx;
	struct paramlist *params;
	struct functioninfo *funcinfo;
	struct symbol *next;
} symbol;

/* A paramlist is just a singly linked list indicating the type of each parameter */
typedef struct paramlist {
	char paramtype[TYPEARRAY_SIZE];
	struct paramlist *next;
} paramlist;

/* An arraylist is used to represent the dimensions of an array variable, if any */
typedef struct arraylist {
	int size;
	struct arraylist *next;
} arraylist;

/* Function prototypes */
symboltable *create_symboltable(symboltable*, node*);
void add_array_dimension(symbol*, int);
void append_symbol_param(symboltable*, char[]);
symboltable *find_symboltable(symboltable*, char*);
symbol *find_symbol(symboltable*, char*);
symbol *insert_symbol(symboltable*, char*, char[], node*);
void print_tabs(void);
void print_type(char[]);
void print_symbol_table(symboltable*);

#endif
