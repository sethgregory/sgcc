#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "node.h"
#include "symbol.h"
#include "irnode.h"

static int tablelevel=0;


/*
 * Creates a new symboltable.
 */
symboltable *create_symboltable(symboltable *parent, node *n) {

	/* Pass in either a NULL or the FUNCTION_DECL node */
	if(n != NULL) {
		symboltable *filescope = parent;
		while(filescope->parent != NULL) {
		 	filescope = filescope->parent;
		}
		
		tablelist *toplevel = filescope->children;
		
		/* Iterate through the top level's list of tables to see if one exists for the function already */
		
		while(toplevel != NULL) {
		//printf("Checking whether %s = %s\n", n->left->string_data, toplevel->table->name);
			/* If you find one... */
			if(!strcmp(n->left->string_data, toplevel->table->name)) {
				/* Compare the subtrees of the two decl nodes to see if they're the same */
				if(compare_subtrees(n, toplevel->table->tablefor->funcinfo->declnode)) {
					//printf("They're the same!\n");
					/* they're the same, check if it's defined/prototyped */
					if(toplevel->table->tablefor->funcinfo->is_defined || toplevel->table->tablefor->funcinfo->is_prototyped) {
						//printf("Create symboltable returned %p\n", toplevel->table);
						return toplevel->table;
					}
					else {
						//printf("Create symboltable returned NULL\n");
						return NULL;
					}
				}
				else {
					/* The subtrees are not the same - different function signatures, error. */
					printf("ERROR: Function \"%s\" redeclared with different parameters.\n", toplevel->table->name);
					return toplevel->table;
				}
			}
			toplevel = toplevel->next;
		}
	} // end if(n != NULL);


	/* Create the new symbol table */
	symboltable *newtable;
	newtable = (symboltable*)malloc(sizeof(symboltable));
	newtable->parent = parent;
	newtable->children = NULL;
	newtable->symbol = NULL;
	newtable->tablefor = NULL;

	/* If this is for a function */	
	if( n != NULL) {
		newtable->name = n->left->string_data;		
		newtable->tablefor = n->left->symbol;
	}
	
	//printf("Created new symbol table at 0x%08p.\t",newtable);
	
	

	if(newtable->parent != NULL) {
		/* set parent's child links */
		symbol *sptr = parent->symbol;
		
		if(newtable->parent->children == NULL) {
			/* create a new linked list of child tables and add this one */
			newtable->parent->children = (tablelist*)malloc(sizeof(tablelist));
			newtable->parent->children->table = newtable;
			newtable->parent->children->next = NULL;
		} else {
			/* just append this one */
			tablelist *ptr = newtable->parent->children;
			while(ptr->next != NULL) {
				ptr = ptr->next;
			}
			ptr->next = (tablelist*)malloc(sizeof(tablelist));
			ptr->next->table = newtable;
			ptr->next->next = NULL;
		}
	}
		else {
			//printf("This is the top level table.\n");
			newtable->name = "file scope";
		}
	
		//printf("Create symboltable returned newtable @ %p\n", newtable);
		return newtable;
	
}

/*
 * Adds an array dimension list to a given symbol
 */
void add_array_dimension(symbol *symbol, int size) {

	//if(symbol == NULL) return;
	
	/* Create & populate the new arraylist object */	
	arraylist *newindex = (arraylist*)malloc(sizeof(arraylist));
	newindex->next = NULL;
	newindex->size = size;
		
	/* Find where to attach it */
	if(symbol->arrayidx != NULL) {
		arraylist *aptr = symbol->arrayidx;	
		while(aptr->next != NULL) {
			aptr = aptr->next;
		}
		aptr->next = newindex;
	} else symbol->arrayidx = newindex;
		
	
}

/*
 * Appends a parameter list item to a function symbol given its symboltable
 */
void append_symbol_param(symboltable *table, char typearray[]) {
	
	if(table->tablefor->funcinfo->params_inserted) return;
		
	/* Initialize symbol pointer to point to the function that created our table */
	symbol *ptr = table->tablefor;
	
	/* Create a new paramlist and copy in the typearray */	
	paramlist *newparam = (paramlist*)malloc(sizeof(paramlist));
	newparam->next = NULL;
	int i;
	for(i=0;i<TYPEARRAY_SIZE;i++) {
		newparam->paramtype[i] = typearray[i];
	}
	
	/* Initialize size to at least one parameter */
	ptr->size = 1;
	
	/* ptr points to the function that created our table */
	if(ptr->params == NULL) {
		//printf("Added first param ");
		ptr->params = newparam;
	} else {
		paramlist *pptr = ptr->params;
		while(pptr->next != NULL) {
			ptr->size++;
			pptr = pptr->next;
		}
		/* At the end of the list, tack the last one on */
		ptr->size++;
		pptr->next = newparam;
		//printf("Added additional param ");
	}
	
	//printf(" (params size = %d)\n", ptr->size);
}


/*
 * Return the address of a table identified by the given name or NULL
 */
symboltable *find_symboltable(symboltable *table, char *name) {

	symboltable *filescope = table;
	while(filescope->parent != NULL) {
		filescope = filescope->parent;
	}
	
	tablelist *toplevel = filescope->children;
	
	while(toplevel != NULL) {
		if(toplevel->table->tablefor != NULL) {
			if(!strcmp(toplevel->table->tablefor->name, name)){ 
				return toplevel->table;
			}
		}
		toplevel = toplevel->next;
	}
	
	return NULL;
	
}

/*
 * Finds a symbol in a given symbol table or any of its parent tables (in scope)
 */
symbol *find_symbol(symboltable *table, char *name) {
	//printf("DEBUG: looking for symbol %s in table %p\n", name, table);
	symbol *ptr = table->symbol;
	while(ptr != NULL) {
		//printf("|%s| == |%s| ?\n", name, ptr->name);
		if(!strcmp(name, ptr->name)) {
			//printf("DEBUG: found the sucker!\n");
			return ptr;
		}
		ptr = ptr->next;
	}
	
	/* If we didn't find anything, check the parent */
	if(table->parent != NULL) {
		//printf("DEBUG: didn't find, checking parent.\n");
		ptr = find_symbol(table->parent, name);
		return ptr;
	}
	
	//printf("DEBUG: outta luck!\n");
	return NULL;

}

/*
 * Inserts a new symbol into the symbol table
 */
symbol *insert_symbol(symboltable *table, char *name, char typearray[], node *n) {
	
	/* First of all, check to see if a symbol of the same name already exists in this scope */
	symbol *ptr = table->symbol;
	while(ptr != NULL) {
		if(!strcmp(name, ptr->name)) {
			if(ptr->type[IS_FUNCTION]){
				/* we should check to see if this one is the same as what we were going to insert */
				if(compare_subtrees(n->parent, ptr->node->parent)){
					
					/* Multiple times seeing this function signature - is this a def or a proto? */
					if(is_below_subtype(n, FUNCTION_SPEC)) {
						
						/* If it's already been defined once, you can't do it again! */
						if(ptr->funcinfo->is_defined) {
							//printf("It's a re-definition.  Fail.\n");
							return NULL;
						} else {
							/* Seeing a def after a prototype is ok */
							//printf("It's a definition after a prototype.\n");
							ptr->funcinfo->is_defined = 1;
							return ptr;
						}
					} else {
						/* Seeing a prototype after a function has been prototyped OR defined is always bad */
						//printf("It's a prototype, but already been defined/prototyped\n");
							return NULL;
					}
				}
			} // end "if this is a function var" block
			
			/* If this is a parameter decl, check to see if it may have already been decl'd */
			if(is_below_subtype(n, PARAM_DECL)) {
				if(table->tablefor->funcinfo->params_inserted) {
					return ptr;
				}
			}
		
			/* If none of the above cases apply, but we've seen an identifier twice in the same scope, error. */
			return NULL;
		}
		ptr = ptr->next;
	}
	
	/* malloc a new symbol and copy its name */
	symbol *newsymbol = (symbol*)malloc(sizeof(symbol));
	newsymbol->name = (char*)malloc(strlen(name)+1);
	newsymbol->arrayidx = NULL;
	newsymbol->node = n;
	strcpy(newsymbol->name, name);
	
	/* set the symbol's attributes */
	
	int i;
	for(i=0;i<TYPEARRAY_SIZE;i++) {
		newsymbol->type[i] = typearray[i];
		//printf("passed in value of typearray[%d] is %d; set value in symbol is %d\n", i, typearray[i], newsymbol->type[i]);;
	}
	
	newsymbol->size = 0;
	newsymbol->params = NULL;
	newsymbol->funcinfo = NULL;
	newsymbol->next = NULL;
	
	/* If this is a function symbol, determine whether it's a definition or prototype */
	if(typearray[IS_FUNCTION]) {
		newsymbol->funcinfo = (functioninfo*)malloc(sizeof(functioninfo));
		newsymbol->funcinfo->declnode = n->parent;
		
		if(is_below_subtype(n, FUNCTION_SPEC)) {
			//printf("It's a definition\n");
			newsymbol->funcinfo->is_defined = 1;
		} else {
			//printf("It's a prototype\n");
			newsymbol->funcinfo->is_prototyped = 1;
		}
	}
	

	
	
	/* add the symbol to the table */
	if(table->symbol == NULL) {
		/* the first item */
		table->symbol = newsymbol;
		//printf("Attached first symbol - %s - @ %p to table %p.\n", name, table->symbol, table);
	} 
	else {
		/* re-init the symbol pointer */
		ptr = table->symbol;
		while(ptr->next != NULL) {
			ptr = ptr->next;
		}
		ptr->next = newsymbol;
		//printf("Attached new symbol - %s - @ %p to table %p.\n", name, ptr->next, table);
		
	}
	
	return newsymbol;
	
}

/*
 * Prints the appropriate number of tabs given the tablelevel global
 */
void print_tabs() {
	int i;
	for(i=0;i<tablelevel;i++) {
		printf("\t");
	}
}

/*
 * Iterates through the type bitmap and prints a variable's type
 */
void print_type(char type[]) {
	
	if(type[IS_FUNCTION]) printf("function(");
	
	if(type[7]) {
		int i;
		for(i=0;i<type[POINTER_DEPTH];i++) {
			printf("pointer(");
		}
	}
	
	char *str = (char*)malloc(50);

	strcpy(str, "");
	if(type[0]) strcat(str, "void ");
	if(type[1]) strcat(str, "signed ");
	if(type[2]) strcat(str, "unsigned ");
	if(type[3]) strcat(str, "char ");
	if(type[4]) strcat(str, "short ");
	if(type[5]) strcat(str, "long ");
	if(type[6]) strcat(str, "int ");

	/* Knock off trailing space */
	char *pos = str + strlen(str) - 1;
	*pos = '\0';

	printf("%s", str);
	
	if(type[7]) {
		int i;
		for(i=0;i<type[POINTER_DEPTH];i++) {
			printf(")");
		}
	}

}

/*
 * Prints out the top level symbol table and all of its nested children and symbols
 */
void print_symbol_table(symboltable *table) {
	tablelist *tablelist_ptr;
	paramlist *paramlist_ptr;
	symbol *symbol_ptr;
	
	/* Print the symbol table info */
	print_tabs();

	printf("Symbol table for ");
	/* Print appropriate name for symbol table */		
	if(table->tablefor != NULL) printf("function '%s' [size: %d] (symbol @ %p)", table->name, table->sizeofsymbols, table->tablefor);
	else if(table->parent == NULL) printf("file scope");
	else printf("block");

	/* Print info for table's location and parent */
	printf(" @ %p ", table);
	if(table->parent == NULL) {
		printf("(no parent)\n");
	} else {
			printf("(parent is ");
			if(table->parent != NULL) printf("function '%s'", table->parent->name);
			else if(table->parent->parent == NULL) printf("file scope");
			else printf("block");
			printf(" @ %p)\n", table->parent);
	}
	
	/* Print all the symbols in the current table */
	symbol_ptr = table->symbol;
	while(symbol_ptr != NULL) {
		print_tabs();
		printf(" - variable: [offset: %d] %s$%p, ", symbol_ptr->offset, symbol_ptr->name, symbol_ptr);
		
		paramlist_ptr = symbol_ptr->params;
		
		int i=0;
		
		/* Print out array dimensions */
		if(symbol_ptr->arrayidx != NULL) {
		
			arraylist *arrayptr = symbol_ptr->arrayidx;
			while(arrayptr != NULL) {
				printf("array(%d, ", arrayptr->size);
				arrayptr = arrayptr->next;
				i++;
			}
		}
		
		print_type(symbol_ptr->type);
			
		/* Print out function info & parameters */
		if(symbol_ptr->type[IS_FUNCTION]) {
			printf(", %d, [", symbol_ptr->size);
			if(paramlist_ptr == NULL) printf("void");
			while(paramlist_ptr != NULL) {
				print_type(paramlist_ptr->paramtype);
				if(paramlist_ptr->next != NULL) printf(", ");
				paramlist_ptr = paramlist_ptr->next;
			}
			
			printf("])");
		}
		
		if(symbol_ptr->arrayidx != NULL) {
			while(i>0){
				printf(")");
				i--;
			}
		}
		
		printf("\n");
		symbol_ptr = symbol_ptr->next;
	}
	printf("\n");


	/* Recursively call the print function for each child table */	
	tablelist_ptr = table->children;
	while(tablelist_ptr != NULL) {
		tablelevel++;
		print_symbol_table(tablelist_ptr->table);
		tablelist_ptr = tablelist_ptr->next;
		tablelevel--;
	}
		
	
	
	
}
