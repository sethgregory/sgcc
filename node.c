#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "yacc.tab.h"
#include "node.h"
#include "symbol.h"
#include "irnode.h"
#include "parser.h"

extern int yylineno;
extern int is_verbose;
static int nodenum=0;
static int blocklevel=0;
static char typearray[TYPEARRAY_SIZE]={0};
static symboltable* currenttable;
char validtable=1;

static nodelist *end_of_block_list;
static node *rightmost;


void insert_cast_node(node *n) {

	return;

	node *new = malloc_binary_op_node("explcast", malloc_type_node(INT, NULL, NULL), n);
	new->left->parent = new;
	new->parent = n->parent;

	if(n->parent->left == n) { n->parent->left = new; }
	else if(n->parent->right == n) { n->parent->left = new; }


}

int type_greaterequalto_int(char type[]) {
	/* the array for type:  [void] [signed] [unsigned] [char] [short] [long] [int] [is_pointer] */
	if(type[BIT_UNSIGNED] && (type[BIT_INT] || type[BIT_LONG]) && !type[BIT_POINTER]) {
		return 1;
	}
	else return 0;
}

void typecheck_tree(node *n) {
	if(n == NULL) {
		return;
	}

	/* Recurse left */
	typecheck_tree(n->left);

	/* Recurse right */
	typecheck_tree(n->right);

	switch(n->type) {

	/* Usual unary conversions */
	/* applied to operands of unary !, -. +, ~, * and separately to each operand of binary << >>  */

	case NODE_IDENTIFIER:
		//printf("Identifier is %s - greaterorequal to int? %d\n", n->string_data, type_greaterequalto_int(n->symbol->type));
		break;

	case NODE_UNARY_OP:
		break;

	/* Usual binary conversions */
	case NODE_BINARY_OP:
		if(n->left->type == NODE_IDENTIFIER) {
			insert_cast_node(n->left);
		}

		if(n->right->type == NODE_IDENTIFIER) {
			insert_cast_node(n->right);
		}

		if(strlen(n->left->string_data) > 0  && strlen(n->right->string_data) > 0){
			//printf("'%s' opnode with operands %s (%s) and %s (%s)\n", n->string_data, n->left->string_data, n->left->symbol->type, n->right->string_data, n->left->symbol->type);
		}
		break;


	}




}

int is_below_subtype(node *n, int subtype) {
	while(n->parent != NULL) {
		if(n->parent->subtype == subtype) {
			return 1;
		} else {
			n = n->parent;
		}
	}
	return 0;
}
		
	
int compare_nodes(node *n1, node *n2) {
	//printf("Comparing nodes %d and %d: ", n1->nodenum, n2->nodenum);
	if(n1->type == n2->type 
		&& n1->subtype == n2->subtype 
		&& n1->const_type == n2->const_type 
		&& n1->const_value == n2->const_value
		&& !strcmp(n1->string_data, n2->string_data)){
			//printf("same!\n");
			return 1;
		}
		else {
			//printf("different!\n");
			//printf("| %d vs %d | %d vs %d | %d vs %d | %d vs %d | %s vs %s |\n", n1->type, n2->type, n1->subtype, n2->subtype, n1->const_type, n2->const_type, n1->const_value, n2->const_value, n1->string_data, n2->string_data);
			return 0;
		}
}

int compare_subtrees(node *n1, node *n2) {
	if(n1 == NULL && n2 == NULL) return 1;
	else if((n1 == NULL) ^ (n2 == NULL)) return 0;
		
	int returnval=1;
	
	returnval &= compare_subtrees(n1->left, n2->left);
	returnval &= compare_nodes(n1, n2);
	returnval &= compare_subtrees(n1->right, n2->right);	

	return returnval;
}

void build_symbol_tables_recursive(node *n) {
	if(n == NULL) {
		return;
	}
	
	/* Build parent links in the node tree */
	if(n->left != NULL) {
		n->left->parent = n;
	}
	if(n->right != NULL) {
		n->right->parent = n;
	}
	
	/* When we first hit a new block, create a new table. */
	if(n->subtype	== LIST_BLOCK ) {
			if(n->parent->subtype != FUNCTION_DEF) {
				symboltable* parenttable = currenttable;
				currenttable = create_symboltable(parenttable, NULL);
		}
	}
	
	/* Set the is_function when we see a function-decl and clear it when we return to it */
	if(n->subtype == FUNCTION_DECL) {
		typearray[IS_FUNCTION]=1;
	}
	
	/* Set the pointer bit when we see a pointer-decl */
	if(n->subtype == PTR_DECL) {
		typearray[BIT_POINTER]=1;
	}
	
	/* Left-recurse */
	build_symbol_tables_recursive(n->left);
	
	/* the array for type:  [void] [signed] [unsigned] [char] [short] [int] [long] */
	if(n->type == NODE_TYPE) {
		//printf("type: %d\n", n->subtype);
		
		switch(n->subtype) {
			case VOID:
				typearray[BIT_VOID]=1;
				break;
			case SIGNED:
				typearray[BIT_SIGNED]=1;
				break;
			case UNSIGNED:
				typearray[BIT_UNSIGNED]=1;
				break;
			case CHAR:
				typearray[BIT_CHAR]=1;
				break;
			case SHORT:
				typearray[BIT_SHORT]=1;
				break;
			case LONG:
				typearray[BIT_LONG]=1;
				break;
			case INT:
				typearray[BIT_INT]=1;
				break;
		}
	}
	
	/* Increment pointer depth as needed when seeing * operators */
	if(n->type == NODE_UNARY_OP && !strcmp(n->string_data, "*") && typearray[BIT_POINTER]) {
		typearray[POINTER_DEPTH]++;
	}
	
		
			
	/* Handle identifier nodes */
	if(n->type == NODE_IDENTIFIER){

		/* Only create new symbols for identifiers in declarations */
		if(is_below_subtype(n, DECL) || is_below_subtype(n, FUNCTION_SPEC)) {
			
			/* If this is formal parameter, also add its type to the function symbol */
			/* Have to walk up the chain looking for a param_decl because of possible pointers */
			if(is_below_subtype(n, PARAM_DECL)) {
				append_symbol_param(currenttable, typearray);
			}					

			/* Insert the identifier into its symbol table */
			n->symbol = (symbol*)insert_symbol(currenttable, n->string_data, typearray, n);
			if(n->symbol == NULL) {
				fprintf(stderr, "ERROR: Cannot redeclare symbol \"%s\"\n", n->string_data);
				n->symbol = (symbol*)find_symbol(currenttable, n->string_data);	
			}
			
			/* If this is an array decl, find all of the dimensions */
			if(is_below_subtype(n, ARRAY_DECL)) {
				node *nptr = n;
				if(nptr->parent != NULL) {
					while(nptr->parent->subtype == ARRAY_DECL) {
						add_array_dimension(n->symbol, nptr->parent->right->const_value);
						nptr = nptr->parent;
					}
				}
			}

			
		}
		
		/* Otherwise, associate with a symbol within scope if it exists */
		else {
			n->symbol = (symbol*)find_symbol(currenttable, n->string_data);	
			if(n->symbol == NULL) {
				fprintf(stderr, "ERROR: Reference to undeclared variable \"%s\"\n", n->string_data);
			}
		}
	}
	
	
	/* If the current node is a function decl */
	if(n->subtype == FUNCTION_DECL) {
		/* clear the bit field after the function decl for its parameters */
		typearray[BIT_VOID]=0; typearray[BIT_SIGNED]=0; typearray[BIT_UNSIGNED]=0; typearray[BIT_CHAR]=0; typearray[BIT_SHORT]=0; typearray[BIT_LONG]=0; typearray[BIT_INT]=0;  typearray[BIT_POINTER]=0;
		typearray[IS_FUNCTION]=0;
	}
	
	/* Create a new symbol table for the parameters and the remainder of the function contents */
	if(n->subtype == FUNCTION_DECL) {
		symboltable* parenttable = create_symboltable(currenttable, n);
		//printf("Got back. At node %d\n",n->nodenum);
		
		if(parenttable == NULL) {
			fprintf(stderr, "ERROR: Function is already defined\n");
		} else {
			/* Created a legitimate table */
			currenttable = parenttable;
			validtable = 1;
		}


	}
	
	
	build_symbol_tables_recursive(n->right);
	
	/* Walk back up the currenttable if you have merely prototyped a function */
	if(n->subtype == FUNCTION_DECL && currenttable->tablefor->funcinfo != NULL) {
		
		/* Set a flag to show the parameters have been added */
		currenttable->tablefor->funcinfo->params_inserted = 1;
		
		/* If this is a prototype and not a definition, revert the symbol table */
		if(!is_below_subtype(n,FUNCTION_SPEC)) {
			currenttable = currenttable->parent;
		}
	}
	
	/* when leaving a node */
	if(n->subtype == DECL || n->subtype == FUNCTION_SPEC || n->subtype == PARAM_DECL) {
		typearray[0]=0; typearray[1]=0; typearray[2]=0; typearray[3]=0; typearray[4]=0; typearray[5]=0; typearray[6]=0; 
	}
	
	/* Unset the pointer bit when we leave a pointer-decl */
	if(n->subtype == PTR_DECL) {
		typearray[BIT_POINTER]=0;
		typearray[POINTER_DEPTH]=0;
	}

	/* When leaving a block node, go to the parent symbol table */
	if(n->subtype	== LIST_BLOCK) {
		currenttable = currenttable->parent;
	}
	
}

void build_symbol_tables(node *n) {
	// Initialize the top level symbol table
	currenttable = create_symboltable(NULL, NULL);
	build_symbol_tables_recursive(n);
	
}

void print_symbol_tables() {
	symboltable* tabletoprint = currenttable;
	while(tabletoprint->parent != NULL) {
		tabletoprint = tabletoprint->parent;
	}
	print_symbol_table(tabletoprint);
}

/* allocate and initialize a generic node */
node *create_node(int node_type) {
  node *n;
  
  n = malloc(sizeof(node));
  assert(NULL != n);

  n->type = node_type;
  n->subtype = 0;
  n->const_type = 0;
  n->const_value = 0;
  n->nodenum = nodenum;
  n->linenumber = yylineno;
  n->string_data = (char*)malloc(100);
  n->visited = 0;
  nodenum++;
  return n;
}

void start_new_block(node *n) {
	
	blocklevel += 1;
	
	/* If the list is empty, start it. */
	if(end_of_block_list == NULL) {
		end_of_block_list = (nodelist*)malloc(sizeof(nodelist));
		end_of_block_list->next = NULL;
		end_of_block_list->current = (node*)find_rightmost_subnode(n);
		return;
	}
	
	/* Init a new nodelist pointer to find the end of the list for appending */
	nodelist *nlptr;
	nlptr=end_of_block_list;
	while(nlptr->next != NULL) {
		nlptr = nlptr->next;
	}
	
	/* Malloc a new nodelist node and append there */
	nlptr->next = (nodelist*)malloc(sizeof(nodelist));
	nlptr->next->next = NULL;
	nlptr->next->current = (node*)find_rightmost_subnode(n);
	
	return;
}

void end_new_block() {
	blocklevel -= 1;
	
	/* If there's only one item in the list, free/null it. */
	if(end_of_block_list->next == NULL){
		free(end_of_block_list);
		end_of_block_list = NULL;
		return;
	}
	
	/* Init a new nodelist pointer to find the end of the list to remove */
	nodelist *nlptr;
	nlptr=end_of_block_list;
	while(nlptr->next->next != NULL) {
		nlptr = nlptr->next;
	}
	
	/* Remove the nodelist node */
	free(nlptr->next);
	nlptr->next = NULL;
}

int is_end_of_block(node *n) {
	/* If the list is empty, return */
	if(end_of_block_list == NULL) {
		return 0;
	}
	
	/* Init a new nodelist pointer to find the end of the list to check */
	nodelist *nlptr;
	nlptr=end_of_block_list;
	while(nlptr->next != NULL) {
		nlptr = nlptr->next;
	}
	
	/* Is this node the end of the current block? */
	if(nlptr->current == n) {
		return 1;
	} else {
		return 0;
	}
}

node *find_rightmost_subnode(node *n) {
	node *original = n;
	
	if(n == NULL) {
		return NULL;
	}
	
	while (n->right != NULL && n->left != NULL) {
	  if (n->right != NULL) {
	    n = n->right;
	  } else if (n->left != NULL) {
      n = n->left;
	  } 
  } 

	//printf("\nrightmost subnode of node %d is %d\n", original->nodenum, n->nodenum);
	return n;
	
}

void print_block_tabs() {
		int i;
		for(i=0; i<blocklevel; i++){
			printf("\t");
		}
}

void print_leftparen(node *n) {
	switch(n->type) {
		case NODE_BINARY_OP:
		case NODE_UNARY_OP:
			if(!strcmp(n->string_data, ":")) {
				/* This is a label, don't print surrounding parens */
				print_block_tabs();
			}
			else printf("(");
			break;
		default:
			break;
	}
}
	
void print_rightparen(node *n) {
	switch(n->type) {
		case NODE_BINARY_OP:
		case NODE_UNARY_OP:
			if(!strcmp(n->string_data, ":")) {
				/* This is a label, don't print surrounding parens */
			}
			else printf(")");
			break;
		default:
			break;
	}
}

void print_tree(node *n) {
	if(n == NULL) {
		return;
	}


	/* start block if the next child node is a block */
	if(n->parent != NULL) {
		if(n->parent->subtype == LIST_BLOCK) {
			printf(" ");
			if(n->subtype == DECL_LIST) {
				/* an explicit/standalone block */
				print_block_tabs();
			}
			printf("{\n");
			start_new_block(n->parent);
		}
	}
	
	/* Prior to recursing further, perform any "at the top of a structure" actions needed */
	print_leftparen(n);
	switch(n->type) {
		
		case NODE_LIST:
			switch(n->subtype) {

				case DECL:
					print_block_tabs();
					rightmost = find_rightmost_subnode(n->right);
					break;
				
				case FUNCTION_DECL:
				case FUNCTION_CALL:
					rightmost = find_rightmost_subnode(n->right);
					break;
				
				case IF_STMT:
					if(n->parent->parent->subtype != ELSE_STMT) {print_block_tabs(); }
					if(n->parent->subtype == COND_STMT) {
						/* account for conditional stmts, do nothing */
					}
					else printf("if(");
					break;
				
				case ELSE_STMT:
					/* account for conditional stmts */
					if(n->parent->subtype == COND_STMT) {
						printf(" : ");
					}
					else {
						print_block_tabs();
						printf("else ");
					}
					break;
			
				case FOR_STMT:
					print_block_tabs();
					printf("for(");
					break;
				
				case WHILE_STMT:
					print_block_tabs();
					printf("while(");
					break;
					
				case DOWHILE_STMT:
					print_block_tabs();
					printf("do ");
					break;

			}
					
			break;
		
		case NODE_EXPR_STMT:
			rightmost = find_rightmost_subnode(n->right);
			print_block_tabs();
			break;
			
		case NODE_BINARY_OP:
			if(!strcmp(n->string_data, "cast")) {
				printf("(");
			}
				break;
		
		default:
			break;
	}
	
	/* Recurse left */
	print_tree(n->left);


	/* The current node */	
	
	char *nodetype = (char *)malloc(20*sizeof(char));
	char *subtype = (char *)malloc(20*sizeof(char));
	

	lookup_type(nodetype, n->type);
	lookup_subtype(subtype, n->subtype);
	
	switch(n->type) {
		
		case NODE_LIST:
			switch(n->subtype) {
								
				case FUNCTION_DECL:
				case FUNCTION_CALL:
					printf("(");
					break;
									
				case DOWHILE_STMT:
					print_block_tabs();
					printf("while (");
					break;
					
				case IF_STMT:
				case FOR_STMT:
					if(n->parent->subtype == COND_STMT) {
						printf(" ? ");
					}
					else printf(")");
					break;

				case WHILE_STMT:
					printf(")");
					if(n->right == NULL) {
						printf("{}\n");
					}
					break;	
				
				case FOR_PARAM:
					if(n->right != NULL) {
						printf("; ");
					}
					break;
					
				case ARRAY_DECL:
				case SUBSCRIPT_EXPR:
					printf("[");
					break;
					
			}
					
			break;
			
		case NODE_STMT:
			switch(n->subtype) {
				case RESERVED_WORD:	
					printf("%s", n->string_data);
					/* print a space after "goto" or "return" */
					if(!strcmp(n->string_data, "goto") || !strcmp(n->string_data, "return")) { printf(" "); }
					break;
				case STRING_CONST:
					printf("\"%s\"", n->string_data);
					break;
			}
			break;
			
		case NODE_TYPE:
			printf("%s", subtype);
			if(n->parent->type == NODE_BINARY_OP && !strcmp(n->parent->string_data, "cast")) {
				/* no space after the identifier */
			} else printf(" ");
			break;
		
		case NODE_IDENTIFIER:
			printf("%s$%p", n->string_data, n->symbol);
			if(n->parent->subtype == IDENTIFIER_LIST && n != rightmost) { printf(", "); }
			else if(!strcmp(n->parent->string_data, "goto")) { printf(";"); }
			break;
			
		case NODE_BINARY_OP: 
			if(!strcmp(n->string_data, "cast")) {
				/* it's a cast. */
				printf(")");
			}
			
			else printf(" %s ", n->string_data);
			break; 
			
		case NODE_UNARY_OP:
			if(!strcmp(n->string_data, "goto")) {
				/* it's a goto. */
				printf("%s ", n->string_data);
			}
			else printf("%s", n->string_data);
			break;
			
		case NODE_NUMBER:
			printf("%d", n->const_value);
			break;
			
		case NODE_NULL_STMT:
			/* If the { } are empty in a do-while, print a semicolon. */
			if(n->parent->subtype == DOWHILE_STMT) {
				printf(";");
			}
			break;
		

		
		default:
			break;
	}
	
	free(nodetype);
	free(subtype);
	
	/* end block if the current node is the end of a previous block */
	if(is_end_of_block(n)) {
		end_new_block();
		print_block_tabs();
		printf("}\n");
	}
	
	/* Recurse right */
	print_tree(n->right);
	
	
	/* After printing the node and its children */
	
	print_rightparen(n);
	
	switch(n->type) {
		
		case NODE_LIST:
			switch(n->subtype) {
				case DECL:
					printf(";\n");
					break;
				case FUNCTION_DEF:
					break;
				case FUNCTION_DECL:
				case FUNCTION_CALL:
					printf(")");
					break;
				case DOWHILE_STMT:
					printf(");\n");
					break;
				case PARAM_DECL:	
					if(find_rightmost_subnode(n) != rightmost) {
						printf(", ");
					}
					break;
				case PTR_DECL:
				/* Don't print the comma if it's a parameter, let that logic handle it */
					if(n->parent->subtype != PARAM_DECL) {
						if(find_rightmost_subnode(n) != rightmost && find_rightmost_subnode(n)->type == NODE_IDENTIFIER) {
							printf(", ");
						}
					}
					break;
				case EXPR_ITEM:
					if(n != rightmost) {
						printf(", ");
					}
						break;	
				case ARRAY_DECL:
				case SUBSCRIPT_EXPR:
					printf("]");
					break;	
			}
					
			break;
		
		case NODE_EXPR_STMT:
			printf(";\n");
			break;
		
		default:
			break;
	}
}

void print_tree_code(node *n) {		
	if(n == NULL) {
		//printf("[NULL]");
		return;
	}	

	printf("[");
	
	char *nodetype = (char *)malloc(20*sizeof(char));
	char *subtype =  (char *)malloc(20*sizeof(char));
	lookup_type(nodetype, n->type);
	lookup_subtype(subtype, n->subtype);

	switch(n->type) {
		case NODE_NUMBER:
			printf("%s(%d)_%03d",nodetype,n->const_value,n->nodenum);
			break;
		case NODE_STMT:
			switch(n->subtype) {
				case STRING_CONST:
					printf("%s(string)_%03d %s",nodetype,n->nodenum,n->string_data);
					break;
				case RESERVED_WORD:
					printf("%s(%s)_%03d",nodetype,n->string_data,n->nodenum);
					break;
				default:
					break;
			}
			break;
		default:
			if(n->subtype == 0) {
				printf("%s(%s)_%03d",nodetype,n->string_data,n->nodenum);
			}
			else {
					printf("%s(%s)_%03d",nodetype,subtype,n->nodenum);
				}
			break;
	}
	
	free(nodetype);
	free(subtype);


	print_tree_code(n->left);
	print_tree_code(n->right);
	
	printf("]");
}

node *malloc_list_node(node *child_left, node *child_right, int list_type) {

	node *n;
	n = create_node(NODE_LIST);
	
	#ifdef DEBUG
  char *subtype = (char *)malloc(20*sizeof(char));
	lookup_subtype(subtype, list_type);
	printf("malloc_list_node: LIST #%3d (@ 0x%08x) - type %d / %s\t",n->nodenum, n, list_type, subtype);
	if(child_left != NULL) printf("L->%3d\t",child_left->nodenum); else printf("L->NULL\t");
	if(child_right != NULL) printf("R->%3d\t\n",child_right->nodenum); else printf("R->NULL\t\n");
	free(subtype);
	#endif
	
	if(n == NULL) {
		printf("***Out of memory***\n");
	} else {
		n->subtype = list_type;
		n->left = child_left;
		n->right = child_right;
	}

  return n;
}

node *malloc_identifier_node(char *identifier, node *child_left, node *child_right) {
	node *n;
	n = create_node(NODE_IDENTIFIER);
	
	#ifdef DEBUG
	printf("malloc_iden_node: ID   #%3d (@ 0x%08x) - value %s\t\t",n->nodenum, n, identifier);
	if(child_left != NULL) printf("L->%3d\t",child_left->nodenum); else printf("L->NULL\t");
	if(child_right != NULL) printf("R->%3d\t\n",child_right->nodenum); else printf("R->NULL\t\n");
	#endif
	
	if(n == NULL) {
		printf("***Out of memory***\n");
	} else {
		n->string_data = identifier;
		n->left = child_left;
		n->right = child_right;
	}

  return n;
}

node *malloc_type_node(unsigned long type, node *child_left, node *child_right) {
	node *n;
	n = create_node(NODE_TYPE);
	
	#ifdef DEBUG
  char *subtype = (char *)malloc(20*sizeof(char));
	lookup_subtype(subtype, type);
	printf("malloc_type_node: TYPE #%3d (@ 0x%08x) - type %d / %s\t",n->nodenum, n, type, subtype);
	if(child_left != NULL) printf("L->%3d\t",child_left->nodenum); else printf("L->NULL\t");
	if(child_right != NULL) printf("R->%3d\t\n",child_right->nodenum); else printf("R->NULL\t\n");
	free(subtype);
	#endif
	
	if(n == NULL) {
		printf("***Out of memory***\n");
	} else {
		n->subtype = type;
		n->left = child_left;
		n->right = child_right;
	}

  return n;
}

node *malloc_unary_op_node(char *operator, node *child_left, node *child_right) {
	node *n;
	n = create_node(NODE_UNARY_OP);
	
	#ifdef DEBUG
	printf("malloc_u_op_node: U_OP #%3d (@ 0x%08x) - operator %s\t\t",n->nodenum, n, operator);
	if(child_left != NULL) printf("L->%3d\t",child_left->nodenum); else printf("L->NULL\t");
	if(child_right != NULL) printf("R->%3d\t\n",child_right->nodenum); else printf("R->NULL\t\n");
	#endif
	
	if(n == NULL) {
		printf("***Out of memory***\n");
	} else {
		n->string_data = operator;
		n->left = child_left;
		n->right = child_right;
	}

  return n;
}

node *malloc_binary_op_node(char *operator, node *child_left, node *child_right) {
	node *n;
	n = create_node(NODE_BINARY_OP);
	
	#ifdef DEBUG
	printf("malloc_b_op_node: B_OP #%3d (@ 0x%08x) - operator %s\t\t",n->nodenum, n, operator);
	if(child_left != NULL) printf("L->%3d\t",child_left->nodenum); else printf("L->NULL\t");
	if(child_right != NULL) printf("R->%3d\t\n",child_right->nodenum); else printf("R->NULL\t\n");
	#endif

	
	if(n == NULL) {
		printf("***Out of memory***\n");
	} else {
		n->string_data = operator;
		n->left = child_left;
		n->right = child_right;
	}

  return n;
}

node *malloc_number_node(unsigned long val) {
  node *n;

  n = create_node(NODE_NUMBER);

	#ifdef DEBUG
  printf("malloc_nmbr_node: NUM  #%3d (@ 0x%08x) - value %d\n",n->nodenum, n, val);
	#endif

  if(n == NULL) {
    printf("***Out of memory***\n");
  } else {
    n->const_value = (int)val;
  }

  return n;
}

node *malloc_stmt_node(char *str, int sub) {
  node *n;

  n = create_node(NODE_STMT);

	#ifdef DEBUG
	char *subtype = (char *)malloc(20*sizeof(char));
	lookup_subtype(subtype, sub);
	printf("malloc_stmt_node: STMT #%3d (@ 0x%08x) - stmt: %s, subtype: %s\n",n->nodenum, n, str, subtype);
	free(subtype);
	#endif

  if(n == NULL) {
    printf("***Out of memory***\n");
  } else {
  	n->subtype = sub;
    n->string_data = str;
  }

  return n;
}

node *malloc_expr_node(node *child_left, node *child_right) {
  node *n;

  n = create_node(NODE_EXPR_STMT);

	#ifdef DEBUG
	printf("malloc_expr_node: EXPR #%3d (@ 0x%08x)\t\t\t",n->nodenum, n);
	if(child_left != NULL) printf("L->%3d\t",child_left->nodenum); else printf("L->NULL\t");
	if(child_right != NULL) printf("R->%3d\t\n",child_right->nodenum); else printf("R->NULL\t\n");
	#endif

  if(n == NULL) {
    printf("***Out of memory***\n");
  } else {
		n->left = child_left;
		n->right = child_right;
  }

  return n;
}

node *malloc_null_node() {
  node *n;

  n = create_node(NODE_NULL_STMT);

	#ifdef DEBUG
	printf("malloc_null_node: NULL #%3d (@ 0x%08x)\n",n->nodenum, n);
	#endif

	if(n == NULL) {
		printf("***Out of memory***\n");
	} else {
		n->left = NULL;
		n->right = NULL;
	}
  return n;
}

char *lookup_subtype(char *aux, int subtype) {
	switch(subtype) {
		case SIGNED:
			strcpy(aux, "signed");
			break;
		case UNSIGNED:
			strcpy(aux, "unsigned");
			break;
		case CHAR:
			strcpy(aux, "char");
			break;
		case SHORT:
			strcpy(aux, "short");
			break;
		case INT:
			strcpy(aux, "int");
			break;
		case LONG:
			strcpy(aux, "long");
			break;
		case VOID:
			strcpy(aux, "void");
			break;
		case GENERIC:
			strcpy(aux, "generic");
			break;
		case FUNCTION_DEF:
			strcpy(aux, "func-def");
			break;
		case FUNCTION_DECL:
			strcpy(aux, "func-decl");
			break;
		case FUNCTION_SPEC:
			strcpy(aux, "func-spec");
			break;
		case FUNCTION_CALL:
			strcpy(aux, "func-call");
			break;
		case DECL:
			strcpy(aux, "decl");
			break;
		case DECL_LIST:
			strcpy(aux, "decl-list");
			break;
		case STMT_LIST:
			strcpy(aux, "stmt-list");
			break;
		case PARAM_DECL:
			strcpy(aux, "param-decl");
			break;
		case FILESCOPE:
			strcpy(aux, "filescope");
			break;
		case TOPLEVEL:
			strcpy(aux, "toplevel");
			break;
		case STRING_CONST:
			strcpy(aux, "string-const");
			break;
		case RESERVED_WORD:
			strcpy(aux, "reservedword");
			break;
		case IF_STMT:
			strcpy(aux, "if-stmt");
			break;
		case IF_ELSE_STMT:
			strcpy(aux, "if-else-stmt");
			break;
		case ELSE_STMT:
			strcpy(aux, "else-stmt");
			break;
		case COND_STMT:
			strcpy(aux, "cond-stmt");
			break;
		case FOR_STMT:
			strcpy(aux, "for-stmt");
			break;
		case FOR_PARAM:
			strcpy(aux, "for-param");
			break;
		case WHILE_STMT:
			strcpy(aux, "while");
			break;
		case DOWHILE_STMT:
			strcpy(aux, "do-while");
			break;
		case ARRAY_DECL:
			strcpy(aux, "array-decl");
			break;
		case SUBSCRIPT_EXPR:
			strcpy(aux, "subscript");
			break;
		case PARAM_LIST:
			strcpy(aux, "param-list");
			break;
		case PTR_DECL:
			strcpy(aux, "pointer-decl");
			break;
		case ABSTRACT_DECL:
			strcpy(aux, "abstract-decl");
			break;
		case LIST_BLOCK:
			strcpy(aux, "block");
			break;
		case IDENTIFIER_LIST:
			strcpy(aux, "ident-list");
			break;
		case EXPR_LIST:
			strcpy(aux, "expr-list");
			break;
		case EXPR_ITEM:
			strcpy(aux, "expr-item");
			break;
		default:
			strcpy(aux, "");
			break;
	}
	
	return aux;
}

char *lookup_type(char *aux, int type) {
	switch(type) {
		case NODE_LIST:
			strcpy(aux, "list");
			break;
		case NODE_DECL:
			strcpy(aux, "decl");
			break;
		case NODE_TYPE:
			strcpy(aux, "type");
			break;
		case NODE_IDENTIFIER:
			strcpy(aux, "identifier");
			break;
		case NODE_NUMBER:
			strcpy(aux, "number");
			break;
		case NODE_EXPR_STMT:
			strcpy(aux, "expr-stmt");
			break;
		case NODE_STMT:
			strcpy(aux, "stmt");
			break;
		case NODE_UNARY_OP:
			strcpy(aux, "unaryop");
			break;
		case NODE_BINARY_OP:
			strcpy(aux, "binaryop");
			break;
		case NODE_NULL_STMT:
			strcpy(aux, "nullnode");
			break;
		default:
			strcpy(aux, "");
			break;
	}
	
	return aux;
}

void process_tree(node *n) {
	build_symbol_tables(n);

	//printf("Typechecking...\n\n");
	//typecheck_tree(n);

	if(is_verbose){
		printf("Generating intermediate representation nodes... ");
	}

	build_ir(n);

	if(is_verbose){
		printf(" Done.\n\n");
		printf("\n\nDEBUG: Tree output code (paste @ http://ironcreek.net/phpsyntaxtree)\n\n");
		print_tree_code(n);
		printf("\n\n");

		printf("\n\n*** Pretty Print ***\n\n");
		print_tree(n);

		printf("\n\n*** Symbol Tables ***\n\n");
		print_symbol_tables();

		printf("\n\n*** Intermediate Representation ***\n\n");
		print_ir_list();

		printf("\n\n*** MIPS Assembly ***\n\n");
	}

	print_mips(currenttable);

	if(is_verbose) {
		printf("\n\n*** Symbol Tables (WITH OFFSETS) ***\n\n");
		print_symbol_tables();
	}
}
