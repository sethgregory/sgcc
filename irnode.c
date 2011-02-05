#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "node.h"
#include "symbol.h"
#include "irnode.h"
#include "mips.h"
#include "token.h"

/* Global IR list  */
ir_node *irlist = NULL;
void *returnval = 0;
int registernum = 0;
char temp_string[255];
int labelnum = 0;

/*
 * This function takes a string and generates a new mips label with an appended
 * serial number, to avoid name collisions.
 */
char *new_label(char *label) {
	char buff[100];
	int length = sprintf(buff, "%s_%04d", label, labelnum);
	label = (char*)malloc(length+1);
	strcpy(label, buff);
	labelnum++;
	return label;
}

/*
 * This function takes a node and an opcode and returns a pointer to a new ir node struct
 */
ir_node *new_irnode(node *n, operation opcode) {
	ir_node *irn = malloc(sizeof(ir_node));
	irn->opnode = n;
	irn->opcode = opcode;
	irn->operand1_type = OT_NULL;
	irn->operand1_data = NULL;
	irn->operand2_type = OT_NULL;
	irn->operand2_data = NULL;
	irn->operand3_type = OT_NULL;
	irn->operand3_data = NULL;
	irn->comment = "";

	return irn;
}

/* This function takes an ir node, an operand number, operand type, and the actual operand data
 * and sets the value of the corresponding operand for that ir node.
 */
void set_operand(ir_node *irn, int operand_num, operandtype optype, void *opdata) {
	switch(operand_num) {
	case 1:
		irn->operand1_type = optype;
		irn->operand1_data = opdata;
		break;
	case 2:
		irn->operand2_type = optype;
		irn->operand2_data = opdata;
		break;
	case 3:
		irn->operand3_type = optype;
		irn->operand3_data = opdata;
		break;
	}
}

/*
 * This function takes an ir node and automatically sets its comment based on the operation
 * and its operand values.
 */
void set_comment(ir_node *irn) {
	char *buff = (char*)malloc(200);
	int length;

	switch(irn->opcode){
	case OP_ADD:
	case OP_SUB:
	case OP_MULT:
	case OP_DIV:
	case OP_MOD:
	case OP_SLT:
	case OP_SLTE:
	case OP_SGT:
	case OP_SGTE:
	case OP_SEQ:
	case OP_BITAND:
	case OP_BITOR:
		length=sprintf(buff, "Setting t%04d equal to t%04d %s t%04d", (int)irn->operand1_data, (int)irn->operand2_data, irn->opnode->string_data, (int)irn->operand3_data);
		break;
	case OP_LIW:
		length=sprintf(buff, "Loading indirect word from address in t%04d into t%04d", (int)irn->operand2_data, (int)irn->operand1_data);
		break;
	case OP_LA:
		length=sprintf(buff, "Loading the address of %s into t%04d", (char *)((symbol *)irn->operand2_data)->name, (int)irn->operand1_data);
		break;
	case OP_LI:
		length=sprintf(buff, "Loading the immediate %d into t%04d", (int)irn->operand2_data, (int)irn->operand1_data);
		break;
	case OP_SIW:
		length=sprintf(buff, "Storing indirect word from address in t%04d into t%04d", (int)irn->operand1_data, (int)irn->operand2_data);
		break;
	case OP_PARAM:
		length=sprintf(buff, "Setting parameter $t%d",  (int)irn->operand1_data);
		break;
	case OP_FUNCTIONCALL:
		length=sprintf(buff, "Calling function %s$%p",  ((symbol *)irn->operand1_data)->name, (symbol *)irn->operand1_data);
		break;
	case OP_FUNCTIONSTART:
		length=sprintf(buff, "Start of function %s$%p", ((symbol *)irn->operand1_data)->name, (symbol *)irn->operand1_data);
		break;
	case OP_LABEL:
		length=sprintf(buff, "Label for '%s'", (char*)irn->operand1_data);
		break;
	case OP_BLEZ:
		length=sprintf(buff, "Branching to label '%s' if t%04d is <= 0", (char*)irn->operand2_data, (int)irn->operand1_data);
		break;
	case OP_BGZ:
		length=sprintf(buff, "Branching to label '%s' if t%04d is > 0", (char*)irn->operand2_data, (int)irn->operand1_data);
		break;
	case OP_J:
		length=sprintf(buff, "Jump to label '%s' unconditionally", (char*)irn->operand1_data);
		break;
	case OP_JR:
		length=sprintf(buff, "Jump to address in register t%04d unconditionally", (int)irn->operand1_data);
		break;
	default:
		length=sprintf(buff, "-");
		break;
	}

	irn->comment = (char*)malloc(length+1);
	strcpy(irn->comment, buff);
}

/*
 * This is the "top-level" recursive function to build the IR nodes.
 */
void build_ir(node *n) {
	if(n == NULL) {
			return;
	}

	/* Recurse left */
	build_ir(n->left);

	switch(n->subtype) {
	ir_node *irn = NULL;
	char *end;
	char *test;

	case FUNCTION_SPEC:
		irn = new_irnode(n, OP_LABEL);
		char buff[50];
		int length = sprintf(buff, "func_%s", n->right->left->symbol->name);
		char *funclabel=malloc(length);
		strcpy(funclabel, buff);
		set_operand(irn, 1, OT_LABEL, (void*)funclabel);
		set_comment(irn);
		append_ir_node(irn);

		irn = new_irnode(n, OP_FUNCTIONSTART);
		set_operand(irn, 1, OT_POINTER, (void*)n->right->left->symbol);
		set_comment(irn);
		append_ir_node(irn);
		break;

	case IF_STMT:
		if(n->visited) break;
		n->visited=1;

		irn = new_irnode(n, OP_BLEZ);
		end = new_label("endif");
		set_operand(irn, 1, OT_REGISTER, (void*)generate_ir_node(n->left, R_VALUE));
		set_operand(irn, 2, OT_LABEL, (void*)end);
		set_comment(irn);
		append_ir_node(irn);

		build_ir(n->right);

		/* If this "if" has an "else" */
		if(n->parent->subtype == IF_ELSE_STMT) {
			/* use the (unused) string_data member to hold a label for the else node */
			n->parent->right->string_data = new_label("endelse");
			irn = new_irnode(n, OP_J);
			set_operand(irn, 1, OT_LABEL, (void*)n->parent->right->string_data);
			set_comment(irn);
			append_ir_node(irn);
		}

		irn = new_irnode(n, OP_LABEL);
		set_operand(irn, 1, OT_LABEL, (void*)end);
		set_comment(irn);
		append_ir_node(irn);
		break;

	case ELSE_STMT:
		if(n->visited) break;
		n->visited=1;

		build_ir(n->left);
		irn = new_irnode(n, OP_LABEL);
		set_operand(irn, 1, OT_LABEL, (void*)n->string_data);
		set_comment(irn);
		append_ir_node(irn);
		break;

	case WHILE_STMT:
		irn = new_irnode(n, OP_LABEL);
		test = new_label("while_test");
		end = new_label("while_end");
		set_operand(irn, 1, OT_LABEL, (void*)test);
		set_comment(irn);
		append_ir_node(irn);

		irn = new_irnode(n, OP_BLEZ);
		set_operand(irn, 1, OT_REGISTER, (void*)generate_ir_node(n->left, R_VALUE));
		set_operand(irn, 2, OT_LABEL, (void*)end);
		set_comment(irn);
		append_ir_node(irn);

		build_ir(n->right);

		irn = new_irnode(n, OP_J);
		set_operand(irn, 1, OT_LABEL, (void*)test);
		set_comment(irn);
		append_ir_node(irn);

		irn = new_irnode(n, OP_LABEL);
		set_operand(irn, 1, OT_LABEL, (void*)end);
		set_comment(irn);
		append_ir_node(irn);

		break;

	case FOR_STMT:

		/* Generate a node for the first forparam if it exists */
		if(n->left->left->type != NODE_NULL_STMT) {
			generate_ir_node(n->left->left, R_VALUE);
		}

		irn = new_irnode(n, OP_LABEL);
		test = new_label("for_test");
		end = new_label("for_end");
		set_operand(irn, 1, OT_LABEL, (void*)test);
		set_comment(irn);
		append_ir_node(irn);

		/* Test on the second forparam */
		if(n->left->right->left->type != NODE_NULL_STMT) {
			irn = new_irnode(n, OP_BLEZ);
			set_operand(irn, 1, OT_REGISTER, (void*)generate_ir_node(n->left->right->left, R_VALUE));
		}
		else {
			/* If the second forparam is missing, set to always true */
			irn = new_irnode(n, OP_BGZ);
			set_operand(irn, 1, OT_REGISTER, (void*)REGISTERZERO);
		}

		set_operand(irn, 2, OT_LABEL, (void*)end);
		set_comment(irn);
		append_ir_node(irn);

		build_ir(n->right);

		/* After the body of the block, generate a node for the third forparam if it exists */
		if(n->left->right->right->left->type != NODE_NULL_STMT) {
			generate_ir_node(n->left->right->right->left, R_VALUE);
		}

		irn = new_irnode(n, OP_J);
		set_operand(irn, 1, OT_LABEL, (void*)test);
		set_comment(irn);
		append_ir_node(irn);

		irn = new_irnode(n, OP_LABEL);
		set_operand(irn, 1, OT_LABEL, (void*)end);
		set_comment(irn);
		append_ir_node(irn);
		break;
	}

	if(n->type == NODE_EXPR_STMT || n->subtype == EXPR_ITEM) {
		generate_ir_node(n->left, R_VALUE);
	}



	/* Recurse right */
	build_ir(n->right);

	if(n->type == NODE_EXPR_STMT) {
		ir_node *irn = new_irnode(n, OP_EOS);
		irn->comment = "End of statement.";
		append_ir_node(irn);
	}
}

/*
 * This is another recursive function that handles generating ir nodes for
 * basic operations
 */
int generate_ir_node(node *n, valuetype v) {
	if(n == NULL || n->visited) {
		return -1;
	}

	/* Set this node as visited so we don't try to emit IR for it again */
	n->visited=1;

	int returnval;
	ir_node *irn = NULL;

	switch(n->type) {
	case NODE_STMT:
		if(!strcmp(n->string_data, "return")) {

			int returnregister = generate_ir_node(n->parent->right, L_VALUE);

			/* Only need to set v0 if return type is not null */
			if(returnregister >= 0) {
				/* For immediate/constant return values */
				if((int)find_ir_node(1, returnregister)->operand2_type == OT_IMMEDIATE) {
					irn = new_irnode(n, OP_MOVE);
					set_operand(irn, 1, OT_REGISTER, (void*)REGISTERV0);
					set_operand(irn, 2, OT_REGISTER, (void*)returnregister);
					set_comment(irn);
					append_ir_node(irn);

				} else {
					/* For pointer/variable return values */
					irn = new_irnode(n, OP_LIW);
					set_operand(irn, 1, OT_REGISTER, (void*)REGISTERV0);
					set_operand(irn, 2, OT_REGISTER, (void*)returnregister);
					set_comment(irn);
					append_ir_node(irn);
				}
			}

			/* The jump register operation */
			irn = new_irnode(n, OP_JR);
			set_operand(irn, 1, OT_REGISTER, (void*)REGISTERRA);
			set_comment(irn);
			append_ir_node(irn);
		}
		else if(n->subtype == STRING_CONST) {
			/* TODO: Add handling code here for string constants */
			returnval = -1;
		}
		break;

	case NODE_LIST:
		switch(n->subtype) {
		case FUNCTION_CALL:
			irn = new_irnode(n->left, OP_FUNCTIONCALL);
			set_operand(irn, 1, OT_POINTER, (void*)n->left->symbol);
			set_comment(irn);
			append_ir_node(irn);
			returnval = REGISTERV0;
			break;
		}

		break;

	case NODE_NUMBER:
		irn = new_irnode(n, OP_LI);
		set_operand(irn, 1, OT_REGISTER, (void*)registernum);
		set_operand(irn, 2, OT_IMMEDIATE, (void*)n->const_value);
		set_comment(irn);
		append_ir_node(irn);
		registernum++;

		returnval = registernum-1;
		break;

	case NODE_IDENTIFIER:

		irn = new_irnode(n, OP_LA);

		set_operand(irn, 1, OT_REGISTER, (void*)registernum);
		set_operand(irn, 2, OT_POINTER, (void*)n->symbol);
		set_comment(irn);
		append_ir_node(irn);
		registernum++;

		if(v == R_VALUE) {
			irn = new_irnode(n, OP_LIW);
			set_operand(irn, 1, OT_REGISTER, (void*)registernum);
			set_operand(irn, 2, OT_REGISTER, (void*)registernum-1);
			set_comment(irn);
			append_ir_node(irn);
			registernum++;
		}

		returnval = registernum-1;

		break;


	case NODE_UNARY_OP:
		/* Increment */
		if(!strcmp(n->string_data, "++")) {
			int savedreg;
			irn = new_irnode(n, OP_ADDI);
			if(n->left != NULL) {
				savedreg = generate_ir_node(n->left, R_VALUE);
			} else {
				savedreg = generate_ir_node(n->right, R_VALUE);
			}
			set_operand(irn, 1, OT_REGISTER, (void*)savedreg);
			set_operand(irn, 2, OT_REGISTER, (void*)savedreg);
			/* We need to do operands 2 and 3 above first, so registernum is correct for this */
			set_operand(irn, 3, OT_IMMEDIATE, (void*)1);
			set_comment(irn);
			append_ir_node(irn);
			returnval = registernum-1;
		}

		/* Decrement */
		else if(!strcmp(n->string_data, "--")) {
			int savedreg;
			irn = new_irnode(n, OP_ADDI);
			if(n->left != NULL) {
				savedreg = generate_ir_node(n->left, R_VALUE);
			} else {
				savedreg = generate_ir_node(n->right, R_VALUE);
			}
			set_operand(irn, 1, OT_REGISTER, (void*)savedreg);
			set_operand(irn, 2, OT_REGISTER, (void*)savedreg);
			/* We need to do operands 2 and 3 above first, so registernum is correct for this */
			set_operand(irn, 3, OT_IMMEDIATE, (void*)-1);
			set_comment(irn);
			append_ir_node(irn);
			returnval = registernum-1;
		}
		break;


	case NODE_BINARY_OP:

		irn = malloc(sizeof(ir_node));
		irn->opnode = n;

		/* Simple assignment */
		if(!strcmp(n->string_data, "=")) {
			irn = new_irnode(n, OP_SIW);
			set_operand(irn, 2, OT_REGISTER, (void*)generate_ir_node(n->left, L_VALUE));
			set_operand(irn, 1, OT_REGISTER, (void*)generate_ir_node(n->right, R_VALUE));
			set_comment(irn);
			append_ir_node(irn);
			returnval = 0;
		}

		/* Logical And */
		else if(!strcmp(n->string_data,"&&")) {

			char* faillabel = new_label("and_fail");
			char* passlabel = new_label("and_pass");
			char* donelabel = new_label("and_done");

			/* Test left operand's value, branch to fail if false */
			irn = new_irnode(n, OP_BLEZ);
			set_operand(irn, 1, OT_REGISTER, (void*)generate_ir_node(n->left, R_VALUE));
			set_operand(irn, 2, OT_LABEL, (void*)faillabel);
			set_comment(irn);
			append_ir_node(irn);

			/* Test right operand's value, branch to pass if true */
			irn = new_irnode(n, OP_BGZ);
			set_operand(irn, 1, OT_REGISTER, (void*)generate_ir_node(n->right, R_VALUE));
			set_operand(irn, 2, OT_LABEL, (void*)passlabel);
			set_comment(irn);
			append_ir_node(irn);

			/* Fail label */
			irn = new_irnode(n, OP_LABEL);
			set_operand(irn, 1, OT_LABEL, (void*)faillabel);
			set_comment(irn);
			append_ir_node(irn);

			/* Load false into new register if failed */
			irn = new_irnode(n, OP_LI);
			set_operand(irn, 1, OT_REGISTER, (void*)registernum);
			set_operand(irn, 2, OT_IMMEDIATE, (void*)0);
			set_comment(irn);
			append_ir_node(irn);

			/* Jump unconditionally to done */
			irn = new_irnode(n, OP_J);
			set_operand(irn, 1, OT_LABEL, (void*)donelabel);
			set_comment(irn);
			append_ir_node(irn);

			/* Pass label */
			irn = new_irnode(n, OP_LABEL);
			set_operand(irn, 1, OT_LABEL, (void*)passlabel);
			set_comment(irn);
			append_ir_node(irn);

			/* Load true into new register if passed */
			irn = new_irnode(n, OP_LI);
			set_operand(irn, 1, OT_REGISTER, (void*)registernum);
			set_operand(irn, 2, OT_IMMEDIATE, (void*)1);
			set_comment(irn);
			append_ir_node(irn);

			/* Done label */
			irn = new_irnode(n, OP_LABEL);
			set_operand(irn, 1, OT_LABEL, (void*)donelabel);
			set_comment(irn);
			append_ir_node(irn);

			registernum++;
			returnval = registernum-1;
		}

		/* Logical Or */
		else if(!strcmp(n->string_data,"||")) {

			char* faillabel = new_label("or_fail");
			char* passlabel = new_label("or_pass");
			char* donelabel = new_label("or_done");

			/* Test left operand's value, branch to pass if true */
			irn = new_irnode(n, OP_BGZ);
			set_operand(irn, 1, OT_REGISTER, (void*)generate_ir_node(n->left, R_VALUE));
			set_operand(irn, 2, OT_LABEL, (void*)passlabel);
			set_comment(irn);
			append_ir_node(irn);

			/* Test right operand's value, branch to fail if false */
			irn = new_irnode(n, OP_BLEZ);
			set_operand(irn, 1, OT_REGISTER, (void*)generate_ir_node(n->right, R_VALUE));
			set_operand(irn, 2, OT_LABEL, (void*)faillabel);
			set_comment(irn);
			append_ir_node(irn);

			/* Pass label */
			irn = new_irnode(n, OP_LABEL);
			set_operand(irn, 1, OT_LABEL, (void*)passlabel);
			set_comment(irn);
			append_ir_node(irn);

			/* Load true into new register if passed */
			irn = new_irnode(n, OP_LI);
			set_operand(irn, 1, OT_REGISTER, (void*)registernum);
			set_operand(irn, 2, OT_IMMEDIATE, (void*)1);
			set_comment(irn);
			append_ir_node(irn);

			/* Jump unconditionally to done */
			irn = new_irnode(n, OP_J);
			set_operand(irn, 1, OT_LABEL, (void*)donelabel);
			set_comment(irn);
			append_ir_node(irn);

			/* Fail label */
			irn = new_irnode(n, OP_LABEL);
			set_operand(irn, 1, OT_LABEL, (void*)faillabel);
			set_comment(irn);
			append_ir_node(irn);

			/* Load false into new register if failed */
			irn = new_irnode(n, OP_LI);
			set_operand(irn, 1, OT_REGISTER, (void*)registernum);
			set_operand(irn, 2, OT_IMMEDIATE, (void*)0);
			set_comment(irn);
			append_ir_node(irn);

			/* Done label */
			irn = new_irnode(n, OP_LABEL);
			set_operand(irn, 1, OT_LABEL, (void*)donelabel);
			set_comment(irn);
			append_ir_node(irn);

			registernum++;
			returnval = registernum-1;
		}


		/* Standard binary operations */
		else {

			/* Addition */
			if(!strcmp(n->string_data, "+")) {
				irn = new_irnode(n, OP_ADD);
			}
			/* Subtraction */
			else if(!strcmp(n->string_data, "-")) {
				irn = new_irnode(n, OP_SUB);
			}
			/* Multiplication */
			else if(!strcmp(n->string_data, "*")) {
				irn = new_irnode(n, OP_MULT);
			}
			/* Division */
			else if(!strcmp(n->string_data, "/")) {
				irn = new_irnode(n, OP_DIV);
			}
			/* Remainder */
			else if(!strcmp(n->string_data, "%")) {
				irn = new_irnode(n, OP_MOD);
			}
			/* Equal to */
			else if(!strcmp(n->string_data, "==")) {
				irn = new_irnode(n, OP_SEQ);
			}
			/* Less Than */
			else if(!strcmp(n->string_data, "<")) {
				irn = new_irnode(n, OP_SLT);
			}
			/* Less Than or Equal */
			else if(!strcmp(n->string_data, "<=")) {
				irn = new_irnode(n, OP_SLTE);
			}
			/* Greater Than */
			else if(!strcmp(n->string_data, ">")) {
				irn = new_irnode(n, OP_SGT);
			}
			/* Greater Than or Equal */
			else if(!strcmp(n->string_data, ">=")) {
				irn = new_irnode(n, OP_SGTE);
			}
			/* Bitwise And */
			else if(!strcmp(n->string_data, "&")) {
				irn = new_irnode(n, OP_BITAND);
			}
			/* Bitwise Or */
			else if(!strcmp(n->string_data, "|")) {
				irn = new_irnode(n, OP_BITOR);
			}

			set_operand(irn, 2, OT_REGISTER, (void*)generate_ir_node(n->left, R_VALUE));
			set_operand(irn, 3, OT_REGISTER, (void*)generate_ir_node(n->right, R_VALUE));
			/* We need to do operands 2 and 3 above first, so registernum is correct for this */
			set_operand(irn, 1, OT_REGISTER, (void*)registernum);
			set_comment(irn);
			append_ir_node(irn);
			registernum++;
			returnval = registernum-1;
		}

		break;

	default:
		returnval = -999;
	}

	/* Set parameters ! */
	if(is_below_subtype(n, FUNCTION_CALL) && n->parent->subtype == EXPR_ITEM) {
		irn = new_irnode(n, OP_PARAM);
		set_operand(irn, 1, OT_REGISTER, (void*)registernum-1);
		set_comment(irn);
		append_ir_node(irn);
	}

	return returnval;

}

/*
 * This function takes an ir node and appends it to the end of the ir list
 */
void append_ir_node(ir_node *irn) {
	if(irlist == NULL) {
		irn->prev = NULL;
		irn->next = NULL;
		irlist = irn;
		return;
	}

	/* Else, this is going on the end of a list */
	ir_node *irptr = irlist;
	while(irptr->next != NULL) {
		irptr = irptr->next;
	}

	irptr->next = irn;
	irn->prev = irptr;
	irn->next = NULL;
}

/*
 * This function takes an operation, operand number, and temporary register number and returns the
 * last node that matches those parameters.
 */
ir_node *find_ir_node(int operandnum, int tempregister) {

	/* Else, iterate through the list */
	ir_node *irptr = irlist;
	ir_node *found_irnode = NULL;

	while(irptr != NULL) {
		switch(operandnum) {
			case 1:
				if((int)irptr->operand1_data == tempregister) {
					found_irnode = irptr;
				}
				break;
			case 2:
				if((int)irptr->operand2_data == tempregister) {
					found_irnode = irptr;
				}
				break;
			case 3:
				if((int)irptr->operand3_data == tempregister) {
					found_irnode = irptr;
				}
				break;
		}
		irptr = irptr->next;
	}

	return found_irnode;
}

/*
 * This function takes an operandtype and its data and prints the operand
 * how it should be seen based on those factors.
 */
void print_operand(operandtype optype, void *opdata) {
	switch(optype) {
	case OT_REGISTER:
		printf("t%04d", (int)opdata);
		break;
	case OT_LABEL:
		printf("%s\t\t", (char*)opdata);
		return;
		break;
	case OT_IMMEDIATE:
		printf("#%d", (int)opdata);
		break;
	case OT_POINTER:
		printf("%s$%p\t\t", (char *)((symbol *)opdata)->name, (symbol *)opdata);
		return;
		break;
	default:
		printf("-");
		break;
	}
	printf("\t\t\t");
}

/*
 * This function iterates through the ir list and prints each of the nodes contained therein
 */
void print_ir_list() {
	printf("OPCODE\t\t\tOPERAND1\t\tOPERAND2\t\tOPERAND3\t\tCOMMENT\n");
	printf("----------------\t------------------\t------------------\t------------------\t------------------\n");
	ir_node *irptr = irlist;
	init_names();
	while(irptr != NULL) {
		get_opcode_name(temp_string, irptr->opcode);
		printf("%s", temp_string+3);

		/* Keep stuff aligned for longer opcodes */
		if(strlen(temp_string) > 10) {
			printf("\t\t");
		} else printf("\t\t\t");

		print_operand(irptr->operand1_type, irptr->operand1_data);
		print_operand(irptr->operand2_type, irptr->operand2_data);
		print_operand(irptr->operand3_type, irptr->operand3_data);
		printf("; line %02d: %s", irptr->opnode->linenumber, irptr->comment);

		printf("\n");


		irptr = irptr->next;
	}
}

/*
 * This function calls a variety of other functions that handle generating and printing
 * the final MIPS assembly output
 */
void print_mips(symboltable *currenttable) {
	map_registers(irlist);
	generate_mips(irlist, currenttable);
}

