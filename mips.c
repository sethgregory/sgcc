#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "node.h"
#include "symbol.h"
#include "irnode.h"
#include "mips.h"
#include "parser.h"

extern int is_verbose;
extern FILE *out;

/* Create a register map */
registermap *rm = NULL;

/* Used registers bitmap */
int t_registers[2][10];

/* Temporary global to store stack size */
int stacksize;



/*
 * This function handles the stack operations upon function entry.
 */
void emit_function_entry(symboltable *table, char *str) {
	char strbuf[300];

	stacksize = 44;
	stacksize += table->sizeofsymbols;


	sprintf(strbuf, "sub\t$sp, $sp, %d\t\t# function entry: push space onto stack\n\t", stacksize);
	strcpy(str, strbuf);

	if(table->tablefor->params != NULL) {
		/* Create a symbol pointer to iterate with */
		symbol *symbol_ptr = table->symbol;

		/* Copy down parameters from caller's stack */
		int i, paramoffset, localoffset;
		for(i=1;i<=table->tablefor->size; i++) {

			/* Get the parameter's offset in the caller's stack - add callee's stack size plus saved registers
			 * and total parameters size in caller's stack, subtract i*4 to offset # of parameters */
			paramoffset = stacksize + 48 + (4*table->tablefor->size) - (4 * i);

			/* Get the offset for the local variable of this symbol */
			localoffset = symbol_ptr->offset + 44;

			sprintf(strbuf, "lw\t$t0, %d($sp)\t\t# copy down parameter %d from caller's stack\n\t",paramoffset,i);
			strcat(str, strbuf);
			sprintf(strbuf, "sw\t$t0, %d($sp)\t\t# store parameter %d into local variable '%s'\n\t",localoffset,i, symbol_ptr->name);
			strcat(str, strbuf);

			/* Go to the next symbol to prepare for the next parameter*/
			symbol_ptr = symbol_ptr->next;

		}
	}

	sprintf(strbuf, "sw\t$s0, 40($sp)\t\t# save register s0 on stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "sw\t$s1, 36($sp)\t\t# save register s1 on stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "sw\t$s2, 32($sp)\t\t# save register s2 on stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "sw\t$s3, 28($sp)\t\t# save register s3 on stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "sw\t$s4, 24($sp)\t\t# save register s4 on stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "sw\t$s5, 20($sp)\t\t# save register s5 on stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "sw\t$s6, 16($sp)\t\t# save register s6 on stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "sw\t$s7, 12($sp)\t\t# save register s7 on stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "sw\t$s8, 8($sp)\t\t# save register s8 on stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "sw\t$ra, 4($sp)\t\t# save register ra on stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "sw\t$fp, 0($sp)\t\t# save register fp on stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "move\t$fp, $sp\t\t# update fp with current sp");
	strcat(str, strbuf);




}

/*
 * This function handles the stack operations upon function exit.
 */
void emit_function_exit(char *str) {
	char strbuf[300];

	sprintf(strbuf, "lw\t$s0, 40($sp)\t\t# restore register s0 from stack\n\t");
	strcpy(str, strbuf);
	sprintf(strbuf, "lw\t$s1, 36($sp)\t\t# restore register s1 from stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "lw\t$s2, 32($sp)\t\t# restore register s2 from stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "lw\t$s3, 28($sp)\t\t# restore register s3 from stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "lw\t$s4, 24($sp)\t\t# restore register s4 from stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "lw\t$s5, 20($sp)\t\t# restore register s5 from stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "lw\t$s6, 16($sp)\t\t# restore register s6 from stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "lw\t$s7, 12($sp)\t\t# restore register s7 from stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "lw\t$s8, 8($sp)\t\t# restore register s8 from stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "lw\t$ra, 4($sp)\t\t# restore register ra from stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "lw\t$fp, 0($sp)\t\t# update fp with sp from stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "add\t$sp, $sp, %d\t\t# function exit: pop space off of stack", stacksize);
	strcat(str, strbuf);

}

/*
 * This function handles the stack operations upon function call.
 */
void emit_function_call(symbol* symbol, char *str) {

	char strbuf[2000];

	/* Parameters can be assumed to be a full word long */
	int paramsize = 4 * symbol->size;

	/* Caller saves t-registers */
	sprintf(strbuf, "sub\t$sp, $sp, 48\t\t# function call: push space onto stack\n\t");
	strcpy(str, strbuf);
	sprintf(strbuf, "sw\t$t0, 44($sp)\t\t# save register t0 on stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "sw\t$t1, 40($sp)\t\t# save register t1 on stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "sw\t$t2, 36($sp)\t\t# save register t2 on stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "sw\t$t3, 32($sp)\t\t# save register t3 on stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "sw\t$t4, 28($sp)\t\t# save register t4 on stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "sw\t$t5, 24($sp)\t\t# save register t5 on stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "sw\t$t6, 20($sp)\t\t# save register t6 on stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "sw\t$t7, 16($sp)\t\t# save register t7 on stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "sw\t$t8, 12($sp)\t\t# save register t8 on stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "sw\t$t9, 8($sp)\t\t# save register t9 on stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "sw\t$ra, 4($sp)\t\t# save register ra on stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "sw\t$fp, 0($sp)\t\t# save register fp on stack\n\t");
	strcat(str, strbuf);

	sprintf(strbuf, "jal\tfunc_%s\t\t# call C function '%s'\n\t", symbol->name, symbol->name);
	strcat(str, strbuf);

	/* After function call, caller restores t-registers */
	sprintf(strbuf, "lw\t$t0, 44($sp)\t\t# restore register t0 from stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "lw\t$t1, 40($sp)\t\t# restore register t1 from stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "lw\t$t2, 36($sp)\t\t# restore register t2 from stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "lw\t$t3, 32($sp)\t\t# restore register t3 from stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "lw\t$t4, 28($sp)\t\t# restore register t4 from stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "lw\t$t5, 24($sp)\t\t# restore register t5 from stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "lw\t$t6, 20($sp)\t\t# restore register t6 from stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "lw\t$t7, 16($sp)\t\t# restore register t7 from stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "lw\t$t8, 12($sp)\t\t# restore register t8 from stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "lw\t$t9, 8($sp)\t\t# restore register t9 from stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "lw\t$ra, 4($sp)\t\t# restore register ra from stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "lw\t$fp, 0($sp)\t\t# restore register fp from stack\n\t");
	strcat(str, strbuf);
	sprintf(strbuf, "add\t$sp, $sp, %d\t\t# end function call: pop space off of stack", 48+paramsize);
	strcat(str, strbuf);


}


/*
 * Iterates through all child symbol tables and sets the offset attribute
 * of each symbol contained therein.  Returns an integer value containing the
 * size of all symbols.
 */
int calculate_offsets(tablelist *children, int initial_offset) {

	tablelist *tables = children;
	symboltable *currenttable;
	symbol *symbol_ptr;
	int size, offset, totalsize;
	int largestsize = 0;

	/* At the top level, this will iterate through each function */
	while(tables != NULL) {
		offset = initial_offset;
		currenttable = tables->table;
		totalsize = 0;
		currenttable->sizeofsymbols = 0;

		symbol_ptr = currenttable->symbol;

		while(symbol_ptr != NULL) {
			int i=0;

			/* Size of variable */
			if(symbol_ptr->type[BIT_CHAR]) 			size = 1;
			else if(symbol_ptr->type[BIT_SHORT])	size = 2;
			else if(symbol_ptr->type[BIT_INT])		size = 4;
			else if(symbol_ptr->type[BIT_LONG])		size = 4;

			/* Make sure everything is properly aligned */
			while(offset % size != 0) {
				printf("Offset is %d, symbol %s must be aligned on %d-byte boundary.  Incrementing by one!\n", offset, symbol_ptr->name, size);
				currenttable->sizeofsymbols++;
				totalsize++;
				offset++;
			}

			/* Get array dimensions if applicable for complete size */
			if(symbol_ptr->arrayidx != NULL) {
				arraylist *arrayptr = symbol_ptr->arrayidx;
				while(arrayptr != NULL) {
					size = size * arrayptr->size;
					arrayptr = arrayptr->next;
					i++;
				}
			}

			/* Complete size of the symbol is now in "size" variable. */

			/* Set the offset of the current symbol */
			symbol_ptr->offset = offset;

			/* Increment the offset counter */
			offset += size;

			/* Increment the total size */
			currenttable->sizeofsymbols += size;
			totalsize += size;

			/* Move on to the next symbol. */
			symbol_ptr = symbol_ptr->next;
		}

		/* All symbols done at this level, recurse if necessary for child tables */
		currenttable->sizeofsymbols += calculate_offsets(currenttable->children, offset);

		/* Update the "largest table seen" */
		if(totalsize > largestsize) largestsize = totalsize;

		/* Move to the next table */
		tables = tables->next;
	}

	/* Return the size of the largest subtable in the list (ignored for top level) */
	return largestsize;

}



/*
 * This function is called after each end-of-statement and resets the
 * register mappings (reset to zero liveness state)
 */
void reset_tregs(void) {
	int i,j;
	for(i=0; i<2; i++) {
		for(j=0; j<10; j++){
			t_registers[i][j]=0;
		}
	}
}

/*
 * This is a debug-only function, used to graphically print the current state
 * of the register mappings.
 */
void print_tregs(void) {
	int i,j;
	for(i=0; i<2; i++) {
		if(i==0) printf("is live?:  ");
		else printf("temp reg:  ");
		for(j=0; j<10; j++){
			printf("%3d ",t_registers[i][j]);
		}
		printf("\n");
	}
	printf("-----------------------------------------------------\n");
}

/*
 * Function to create a mapping of temp registers to real, permanent registers.
 * Takes the head of the ir_node list and populates a registermap list rm.
 */
void map_registers(ir_node *irn) {
	ir_node *irptr = irn;

	reset_tregs();

	while(irptr != NULL) {
		if(irptr->opcode == OP_EOS) {
			reset_tregs();
		}
		else {
			if(irptr->operand1_type == OT_REGISTER) {
				if(getmapping((int)irptr->operand1_data) == NULL) {
					setmapping(irptr, (int)irptr->operand1_data);
				}
			}

			if(irptr->operand2_type == OT_REGISTER) {
				if(getmapping((int)irptr->operand2_data) == NULL) {
					setmapping(irptr, (int)irptr->operand2_data);
				}
			}

			if(irptr->operand3_type == OT_REGISTER) {
				if(getmapping((int)irptr->operand3_data) == NULL) {
					setmapping(irptr, (int)irptr->operand3_data);
				}
			}

		}

		irptr = irptr->next;
	}

}

/*
 * Function to retrieve the mapping of a temp register to a real, permanent register.
 * Takes the int value of the temp register, returns int value of permanent register.
 */
char *getmapping(int tempregister) {
	/* Find the item in the list */
	registermap *rmp = rm;
	while(rmp != NULL) {
		if(tempregister == rmp->temp){
			return rmp->registername;
		} else {
			rmp = rmp->next;
		}
	}

	/* Otherwise, return -1 */
	return NULL;
}

/*
 * A function that does a cursory liveness analysis.
 */
int is_live(int reg, ir_node *ir_ptr) {
	/* Get the temporary register associated with the current reg */
	int temporary = t_registers[TEMPREG][reg];

	/* Create a new pointer so we can modify it */
	ir_node *irp = ir_ptr;

	/* Now go through every IR node up to the next EOS and see if that temporary is used again */
	while(irp != NULL) {

		/* If we've hit EOS, we're good! */
		if(irp->opcode == OP_EOS) {
			return 0;
		}

		/* Otherwise, check to see if any of the registers in this IR node are the same temp */
		if(irp->operand1_type == OT_REGISTER) {
			if((int)irp->operand1_data == temporary) {
				return 1;
			}
		}

		if(irp->operand2_type == OT_REGISTER) {
			if((int)irp->operand2_data == temporary) {
				return 1;
			}
		}

		if(irp->operand3_type == OT_REGISTER) {
			if((int)irp->operand3_data == temporary) {
				return 1;
			}
		}

		irp = irp->next;
	}

	return 0;
}



/*
 * Creates a registermap list node with both the temporary register and the permanent register it maps to.
 * Takes both registers as int values.
 */
void setmapping(ir_node *ir_ptr, int tempregister) {

	/* Debug printing of register mapping */
	if(is_verbose) {
		printf("Mapping TEMP register %04d to ", tempregister);
	}

	char strbuf[10];
	char *str = malloc(10);
	int j;

	/* Figure out what perm register we're going to use */
	switch(tempregister) {
	case REGISTERZERO:
		sprintf(strbuf, "$0");
		break;
	case REGISTERV0:
		sprintf(strbuf, "$v0");
		break;

	case -1: /* In the case of a void function, just set $v0=($ra) */
	case REGISTERRA:
		sprintf(strbuf, "$ra");
		break;
	default:

		for(j=0; j<10; j++){
			/* If the register is in use */
			if(t_registers[REALREG][j] == 1) {
				/* If the register is still live */
				//printf("REGISTER LIVE!\n");
				if(is_live(j, ir_ptr)) {
					// Do nothing, keep checking.
					//printf("Register %d is live!  Can't replace.\n", j);
				} else {
					// The register can be used again, reassociate.
					//printf("Register %d is no longer live!  Replacing with temp register.\n", tempregister);
					t_registers[TEMPREG][j] = tempregister;
					sprintf(strbuf, "$t%d", j);
					//printf(" * REPLACING MAPPING - setting temp register %d to register %d\n", tempregister, j);
					break;
				}
			}

			/* Otherwise, set this register in use and associate tempregister */
			else {
				//printf("REGISTER FREE!\n");
				t_registers[REALREG][j] = 1;
				t_registers[TEMPREG][j] = tempregister;
				sprintf(strbuf, "$t%d", j);
				//printf(" * NEW MAPPING IN EMPTY REG - setting temp register %d to register %d\n", tempregister, j);
				break;
			}
		}

		break;
	}

	strcpy(str, strbuf);

	/* Debug printing of register mapping */
	if(is_verbose) {
		printf("PERM register %s (j=%d)\n", str, j);
		print_tregs();
	}

	if(rm == NULL) {
		//printf("Initial run.\n");
		rm = malloc(sizeof(registermap));
		rm->next = NULL;
		rm->temp = tempregister;
		rm->registername = str;
		return;
	}

	/* Find the end of the list */
	registermap *rmp = rm;
	while(rmp->next != NULL) {
		rmp = rmp->next;
	}

	rmp->next = malloc(sizeof(registermap));
	rmp->next->next = NULL;
	rmp->next->temp = tempregister;
	rmp->next->registername = str;
}


/*
 * Function to generate MIPS assembly output.  Takes the head of the ir_node list and a pointer to the top level symbol table
 */
void generate_mips(ir_node *irn, symboltable *table) {


	char *str = (char*)malloc(5000);
	char *strbuf = (char*)malloc(5000);

	fprintf(out, "\t.data\n");
	fprintf(stdout, "\t.data\n");



	/*
	 * Print all globals.
	 */
	int size;
	symbol *symbol_ptr = table->symbol;
	while(symbol_ptr != NULL) {
		int i=0;

		/* Mark all globals as such by setting a negative offset */
		symbol_ptr->offset = -1;

		if(!symbol_ptr->type[IS_FUNCTION]) {

			/* Size of variable */
			if(symbol_ptr->type[BIT_CHAR]) 			size = 1;
			else if(symbol_ptr->type[BIT_SHORT])	size = 2;
			else if(symbol_ptr->type[BIT_INT])		size = 4;
			else if(symbol_ptr->type[BIT_LONG])		size = 4;

			/* Print out array dimensions */
			if(symbol_ptr->arrayidx != NULL) {
				arraylist *arrayptr = symbol_ptr->arrayidx;
				while(arrayptr != NULL) {
					size = size * arrayptr->size;
					arrayptr = arrayptr->next;
					i++;
				}
			}
			fprintf(out, "var_%s:\n\t.space %d\n", symbol_ptr->name, size);
			fprintf(stdout, "var_%s:\n\t.space %d\n", symbol_ptr->name, size);

		}
		symbol_ptr = symbol_ptr->next;
	}

	/* Print out any hard-coded data necessary */
	emit_hardcoded_mips_data(str);


	/*
	 * End global setup
	 */

	/* Now we should calculate offsets for all the local variables / sizes for symbol tables */
	calculate_offsets(table->children, 0);

	/* Set up the MIPS main */
	fprintf(out, "\n\t.text\nmain:\n");
	fprintf(stdout, "\n\t.text\nmain:\n");

	/*
	 * Set up stack, function start
	 *
	 * subtract 44 + (localvars) from sp
	 * store S registers: 9 registers x 4 bytes = 36 bytes
	 * store return address: 4 bytes
	 * store frame pointer: 4 bytes
	 * space for local variables
	 * set frame pointer to current stack pointer
	 *
	 * Do in reverse on function exit (return)
	 */

	/*
	 * Set up stack, function call
	 *
	 * subtract 48 + (params) from sp
	 * store T registers (10 registers x 4 bytes = 40 bytes)
	 * store return address and frame pointer: 8 bytes
	 * store parameter if any
	 *
	 * Do in reverse on return
	 *
	 */


	ir_node *irptr = irn;


	/* Print out the function call of the C main */
	symboltable *st = find_symboltable(table, "main");
	emit_function_call(st->tablefor, str);
	fprintf(out, "\t%s\n", str);
	fprintf(stdout, "\t%s\n", str);

	/* End program */
	fprintf(out, "\tli\t$v0, 10\t\t\t# syscall code to end program\n\tsyscall\n");
	fprintf(stdout, "\tli\t$v0, 10\t\t\t# syscall code to end program\n\tsyscall\n");


	while(irptr != NULL) {
		switch(irptr->opcode) {
		case OP_EOS:
			break;
		case OP_LABEL:
			sprintf(strbuf, "%s:", (char*)irptr->operand1_data);
			strcpy(str, strbuf);
			fprintf(out, "%s\n", str);
			fprintf(stdout, "%s\n", str);
			break;
		case OP_FUNCTIONCALL:
			emit_function_call((symbol*)irptr->operand1_data, str);
			fprintf(out, "\t%s\n", str);
			fprintf(stdout, "\t%s\n", str);
			break;
		case OP_FUNCTIONSTART:
			emit_function_entry(find_symboltable(table, ((symbol*)(irptr->operand1_data))->name), str);
			fprintf(out, "\t%s\n", str);
			fprintf(stdout, "\t%s\n", str);
			break;
		case OP_JR:
			if(!strcmp(getmapping((int)irptr->operand1_data), "$ra")){
				emit_function_exit(str);
			}
			fprintf(out, "\t%s\n", str);
			fprintf(stdout, "\t%s\n", str);

			ir2mips(irptr, str);
			fprintf(out, "\t%s\n", str);
			fprintf(stdout, "\t%s\n", str);
			break;
		default:
			ir2mips(irptr, str);
			fprintf(out, "\t%s\n", str);
			fprintf(stdout, "\t%s\n", str);
			break;
		}

		irptr = irptr->next;
	}

	emit_hardcoded_mips_text(str);

	free(str);

}

/*
 * A function that given an ir node, generates the appropriate MIPS assembly output
 */
void ir2mips(ir_node *irptr, char *str) {
	char *strbuf = (char*)malloc(5000);
	switch(irptr->opcode) {
	case OP_LA:
		if(((symbol *)irptr->operand2_data)->offset >= 0) {
			sprintf(strbuf, "la\t%s, %d($fp)\t\t# load address of local var '%s'", getmapping((int)irptr->operand1_data), ((symbol*)irptr->operand2_data)->offset + 44, (char*)((symbol*)irptr->operand2_data)->name);
			break;
		} else {
			sprintf(strbuf, "la\t%s, var_%s\t# load address of global var '%s'", getmapping((int)irptr->operand1_data), (char*)((symbol*)irptr->operand2_data)->name, (char*)((symbol*)irptr->operand2_data)->name);
			break;
		}
	case OP_LI:
		sprintf(strbuf, "li\t%s, %d\t\t\t# load immediate #%d into register %s", getmapping((int)irptr->operand1_data), (int)irptr->operand2_data, (int)irptr->operand2_data, getmapping((int)irptr->operand1_data));
		break;
	case OP_SIW:
		sprintf(strbuf, "sw\t%s, (%s)\t\t# store indirect word", getmapping((int)irptr->operand1_data), getmapping((int)irptr->operand2_data));
		break;
	case OP_LIW:
		sprintf(strbuf, "lw\t%s, (%s)\t\t# load word into %s from address in %s", getmapping((int)irptr->operand1_data), getmapping((int)irptr->operand2_data), getmapping((int)irptr->operand1_data), getmapping((int)irptr->operand2_data));
		break;
	case OP_MOVE:
		sprintf(strbuf, "move\t%s, %s", getmapping((int)irptr->operand1_data), getmapping((int)irptr->operand2_data));
		break;
	case OP_ADD:
		sprintf(strbuf, "add\t%s, %s, %s", getmapping((int)irptr->operand1_data), getmapping((int)irptr->operand2_data), getmapping((int)irptr->operand3_data));
		break;
	case OP_SUB:
		sprintf(strbuf, "sub\t%s, %s, %s", getmapping((int)irptr->operand1_data), getmapping((int)irptr->operand2_data), getmapping((int)irptr->operand3_data));
		break;
	case OP_MULT:
		sprintf(strbuf, "mul\t%s, %s, %s", getmapping((int)irptr->operand1_data), getmapping((int)irptr->operand2_data), getmapping((int)irptr->operand3_data));
		break;
	case OP_DIV:
		sprintf(strbuf, "div\t%s, %s, %s", getmapping((int)irptr->operand1_data), getmapping((int)irptr->operand2_data), getmapping((int)irptr->operand3_data));
		break;
	case OP_MOD:
		sprintf(strbuf, "rem\t%s, %s, %s", getmapping((int)irptr->operand1_data), getmapping((int)irptr->operand2_data), getmapping((int)irptr->operand3_data));
		break;
	case OP_ADDI:
		sprintf(strbuf, "add\t%s, %s, %d", getmapping((int)irptr->operand1_data), getmapping((int)irptr->operand2_data), (int)irptr->operand3_data);
		break;
	case OP_SLT:
		sprintf(strbuf, "slt\t%s, %s, %s", getmapping((int)irptr->operand1_data), getmapping((int)irptr->operand2_data), getmapping((int)irptr->operand3_data));
		break;
	case OP_SLTE:
		sprintf(strbuf, "sle\t%s, %s, %s", getmapping((int)irptr->operand1_data), getmapping((int)irptr->operand2_data), getmapping((int)irptr->operand3_data));
		break;
	case OP_SGT:
		sprintf(strbuf, "sgt\t%s, %s, %s", getmapping((int)irptr->operand1_data), getmapping((int)irptr->operand2_data), getmapping((int)irptr->operand3_data));
		break;
	case OP_SGTE:
		sprintf(strbuf, "sge\t%s, %s, %s", getmapping((int)irptr->operand1_data), getmapping((int)irptr->operand2_data), getmapping((int)irptr->operand3_data));
		break;
	case OP_SEQ:
		sprintf(strbuf, "seq\t%s, %s, %s", getmapping((int)irptr->operand1_data), getmapping((int)irptr->operand2_data), getmapping((int)irptr->operand3_data));
		break;
	case OP_BLEZ:
		sprintf(strbuf, "blez\t%s, %s", getmapping((int)irptr->operand1_data), (char*)irptr->operand2_data);
		break;
	case OP_BGZ:
		sprintf(strbuf, "bgtz\t%s, %s", getmapping((int)irptr->operand1_data), (char*)irptr->operand2_data);
		break;
	case OP_J:
		sprintf(strbuf, "j\t%s\t\t# Jump to label %s", (char*)irptr->operand1_data, (char*)irptr->operand1_data);
		break;
	case OP_JR:
		sprintf(strbuf, "jr\t%s\t\t\t# Jump to register %s", getmapping((int)irptr->operand1_data), getmapping((int)irptr->operand1_data));
		break;
	case OP_PARAM:
		sprintf(strbuf, "sub\t$sp, $sp, 4\t\t# push space onto stack for parameter\n\tsw\t%s, 0($sp)\t\t# store parameter on stack", getmapping((int)irptr->operand1_data));
		break;
	case OP_BITOR:
		sprintf(strbuf, "or\t%s, %s, %s", getmapping((int)irptr->operand1_data), getmapping((int)irptr->operand2_data), getmapping((int)irptr->operand3_data));
		break;
	case OP_BITAND:
		sprintf(strbuf, "and\t%s, %s, %s", getmapping((int)irptr->operand1_data), getmapping((int)irptr->operand2_data), getmapping((int)irptr->operand3_data));
		break;

	default:
		strcpy(strbuf, "# [unhandled function]");
		break;
	}

	strcpy(str, strbuf);
	free(strbuf);
	return;
}

/*
 * This function outputs hard-coded MIPS assembly for the 'data' section of the program.
 * It may contain strings or other global variables needed for the proper operation of the
 * hard-coded functions below.
 */
void emit_hardcoded_mips_data(char *str) {
	strcpy(str, "intprompt:\n\t.asciiz\t\"Please enter an integer: \"\t# Hard-coded global prompt\n");
	strcat(str, "factresponse:\n\t.asciiz\t\"! (factorial) is equal to \"\t# Hard-coded global response\n");
	strcat(str, "squareresponse:\n\t.asciiz\t\"^2 (squared) is equal to \"\t# Hard-coded global response\n");
	strcat(str, "fibresponse:\n\t.asciiz\t\"# element of Fibonacci sequence is \"\t# Hard-coded global response\n");
	fprintf(stdout, "%s", str);
	fprintf(out, "%s", str);
}

/*
 * This function outputs hard-coded MIPS assembly to handle various system functions.
 * The functions contained here will be prototyped in the C program being compiled, but
 * these are the definitions.
 */
void emit_hardcoded_mips_text(char *str) {
	/* Output static mips code for print_int(int) definition */
	strcpy(str, "func_print_int:\n");
	strcat(str, "\tsub\t$sp, $sp, 12\t\t# enter print_int: push space onto stack\n");
	strcat(str, "\tsw\t$ra, 4($sp)\t\t# save register ra on stack\n");
	strcat(str, "\tsw\t$fp, 0($sp)\t\t# save register fp on stack\n");
	strcat(str, "\tmove\t$fp, $sp\t\t# update fp with current sp\n");
	strcat(str, "\tlw\t$t0, 60($sp)\t\t# copy down int parameter from caller's stack\n");
	strcat(str, "\tsw\t$t0, 8($sp)\t\t# store int parameter into local variable\n");
	strcat(str, "\tmove\t$a0, $t0\t\t# put value into register a0 for syscall\n");
	strcat(str, "\tli\t$v0, 1\t\t\t# set syscall type to print integer\n");
	strcat(str, "\tsyscall\t\t\t\t# syscall\n");
	strcat(str, "\tli\t$a0, 10\t\t\t# put ascii value for newline into register a0 for syscall\n");
	strcat(str, "\tli\t$v0, 11\t\t\t# set syscall type to print character\n");
	strcat(str, "\tsyscall\t\t\t\t# syscall\n");
	strcat(str, "\tlw\t$ra, 4($sp)\t\t# restore register ra from stack\n");
	strcat(str, "\tlw\t$fp, 0($sp)\t\t# restore register fp from stack\n");
	strcat(str, "\tadd\t$sp, $sp, 12\t\t# exit print_int: pop space off of stack\n");
	strcat(str, "\tjr\t$ra\t\t\t# return\n");
	fprintf(stdout, "%s", str);
	fprintf(out, "%s", str);

	/* Output static mips code for print_char(char) definition */
	strcpy(str, "func_print_char:\n");
	strcat(str, "\tsub\t$sp, $sp, 12\t\t# enter print_char: push space onto stack\n");
	strcat(str, "\tsw\t$ra, 4($sp)\t\t# save register ra on stack\n");
	strcat(str, "\tsw\t$fp, 0($sp)\t\t# save register fp on stack\n");
	strcat(str, "\tmove\t$fp, $sp\t\t# update fp with current sp\n");
	strcat(str, "\tlw\t$t0, 60($sp)\t\t# copy down char parameter from caller's stack\n");
	strcat(str, "\tsw\t$t0, 8($sp)\t\t# store char parameter into local variable\n");
	strcat(str, "\tmove\t$a0, $t0\t\t# put value into register a0 for syscall\n");
	strcat(str, "\tli\t$v0, 11\t\t\t# set syscall type to print char\n");
	strcat(str, "\tsyscall\t\t\t\t# syscall\n");
	strcat(str, "\tlw\t$ra, 4($sp)\t\t# restore register ra from stack\n");
	strcat(str, "\tlw\t$fp, 0($sp)\t\t# restore register fp from stack\n");
	strcat(str, "\tadd\t$sp, $sp, 12\t\t# exit print_int: pop space off of stack\n");
	strcat(str, "\tjr\t$ra\t\t\t# return\n");
	fprintf(stdout, "%s", str);
	fprintf(out, "%s", str);

	/* Output static mips code for print_factorial(int,int) definition */
	strcpy(str, "func_print_factorial:\n");
	strcat(str, "\tsub\t$sp, $sp, 16\t\t# enter print_int: push space onto stack\n");
	strcat(str, "\tsw\t$ra, 4($sp)\t\t# save register ra on stack\n");
	strcat(str, "\tsw\t$fp, 0($sp)\t\t# save register fp on stack\n");
	strcat(str, "\tmove\t$fp, $sp\t\t# update fp with current sp\n");

	strcat(str, "\tlw\t$t0, 64($sp)\t\t# copy down int parameter from caller's stack\n");
	strcat(str, "\tsw\t$t0, 12($sp)\t\t# store int parameter into local variable\n");
	strcat(str, "\tmove\t$a0, $t0\t\t# put value into register a0 for syscall\n");
	strcat(str, "\tli\t$v0, 1\t\t\t# set syscall type to print integer\n");
	strcat(str, "\tsyscall\t\t\t\t# syscall\n");

	strcat(str, "\tla\t$a0, factresponse\t# get address of response string\n");
	strcat(str, "\tli\t$v0, 4\t\t\t# set syscall type to print string\n");
	strcat(str, "\tsyscall\t\t\t\t# syscall\n");

	strcat(str, "\tlw\t$t0, 68($sp)\t\t# copy down int parameter from caller's stack\n");
	strcat(str, "\tsw\t$t0, 8($sp)\t\t# store int parameter into local variable\n");
	strcat(str, "\tmove\t$a0, $t0\t\t# put value into register a0 for syscall\n");
	strcat(str, "\tli\t$v0, 1\t\t\t# set syscall type to print integer\n");
	strcat(str, "\tsyscall\t\t\t\t# syscall\n");

	strcat(str, "\tli\t$a0, 10\t\t\t# put ascii value for newline into register a0 for syscall\n");
	strcat(str, "\tli\t$v0, 11\t\t\t# set syscall type to print character\n");
	strcat(str, "\tsyscall\t\t\t\t# syscall\n");
	strcat(str, "\tlw\t$ra, 4($sp)\t\t# restore register ra from stack\n");
	strcat(str, "\tlw\t$fp, 0($sp)\t\t# restore register fp from stack\n");
	strcat(str, "\tadd\t$sp, $sp, 16\t\t# exit print_int: pop space off of stack\n");
	strcat(str, "\tjr\t$ra\t\t\t# return\n");
	fprintf(stdout, "%s", str);
	fprintf(out, "%s", str);

	/* Output static mips code for print_square(int,int) definition */
	strcpy(str, "func_print_square:\n");
	strcat(str, "\tsub\t$sp, $sp, 16\t\t# enter print_int: push space onto stack\n");
	strcat(str, "\tsw\t$ra, 4($sp)\t\t# save register ra on stack\n");
	strcat(str, "\tsw\t$fp, 0($sp)\t\t# save register fp on stack\n");
	strcat(str, "\tmove\t$fp, $sp\t\t# update fp with current sp\n");

	strcat(str, "\tlw\t$t0, 64($sp)\t\t# copy down int parameter from caller's stack\n");
	strcat(str, "\tsw\t$t0, 12($sp)\t\t# store int parameter into local variable\n");
	strcat(str, "\tmove\t$a0, $t0\t\t# put value into register a0 for syscall\n");
	strcat(str, "\tli\t$v0, 1\t\t\t# set syscall type to print integer\n");
	strcat(str, "\tsyscall\t\t\t\t# syscall\n");

	strcat(str, "\tla\t$a0, squareresponse\t# get address of response string\n");
	strcat(str, "\tli\t$v0, 4\t\t\t# set syscall type to print string\n");
	strcat(str, "\tsyscall\t\t\t\t# syscall\n");

	strcat(str, "\tlw\t$t0, 68($sp)\t\t# copy down int parameter from caller's stack\n");
	strcat(str, "\tsw\t$t0, 8($sp)\t\t# store int parameter into local variable\n");
	strcat(str, "\tmove\t$a0, $t0\t\t# put value into register a0 for syscall\n");
	strcat(str, "\tli\t$v0, 1\t\t\t# set syscall type to print integer\n");
	strcat(str, "\tsyscall\t\t\t\t# syscall\n");

	strcat(str, "\tli\t$a0, 10\t\t\t# put ascii value for newline into register a0 for syscall\n");
	strcat(str, "\tli\t$v0, 11\t\t\t# set syscall type to print character\n");
	strcat(str, "\tsyscall\t\t\t\t# syscall\n");
	strcat(str, "\tlw\t$ra, 4($sp)\t\t# restore register ra from stack\n");
	strcat(str, "\tlw\t$fp, 0($sp)\t\t# restore register fp from stack\n");
	strcat(str, "\tadd\t$sp, $sp, 16\t\t# exit print_int: pop space off of stack\n");
	strcat(str, "\tjr\t$ra\t\t\t# return\n");
	fprintf(stdout, "%s", str);
	fprintf(out, "%s", str);

	/* Output static mips code for print_fib(int,int) definition */
	strcpy(str, "func_print_fib:\n");
	strcat(str, "\tsub\t$sp, $sp, 16\t\t# enter print_int: push space onto stack\n");
	strcat(str, "\tsw\t$ra, 4($sp)\t\t# save register ra on stack\n");
	strcat(str, "\tsw\t$fp, 0($sp)\t\t# save register fp on stack\n");
	strcat(str, "\tmove\t$fp, $sp\t\t# update fp with current sp\n");

	strcat(str, "\tlw\t$t0, 64($sp)\t\t# copy down int parameter from caller's stack\n");
	strcat(str, "\tsw\t$t0, 12($sp)\t\t# store int parameter into local variable\n");
	strcat(str, "\tmove\t$a0, $t0\t\t# put value into register a0 for syscall\n");
	strcat(str, "\tli\t$v0, 1\t\t\t# set syscall type to print integer\n");
	strcat(str, "\tsyscall\t\t\t\t# syscall\n");

	strcat(str, "\tla\t$a0, fibresponse\t# get address of response string\n");
	strcat(str, "\tli\t$v0, 4\t\t\t# set syscall type to print string\n");
	strcat(str, "\tsyscall\t\t\t\t# syscall\n");

	strcat(str, "\tlw\t$t0, 68($sp)\t\t# copy down int parameter from caller's stack\n");
	strcat(str, "\tsw\t$t0, 8($sp)\t\t# store int parameter into local variable\n");
	strcat(str, "\tmove\t$a0, $t0\t\t# put value into register a0 for syscall\n");
	strcat(str, "\tli\t$v0, 1\t\t\t# set syscall type to print integer\n");
	strcat(str, "\tsyscall\t\t\t\t# syscall\n");

	strcat(str, "\tli\t$a0, 10\t\t\t# put ascii value for newline into register a0 for syscall\n");
	strcat(str, "\tli\t$v0, 11\t\t\t# set syscall type to print character\n");
	strcat(str, "\tsyscall\t\t\t\t# syscall\n");

	strcat(str, "\tli\t$a0, 10\t\t\t# put ascii value for newline into register a0 for syscall\n");
	strcat(str, "\tli\t$v0, 11\t\t\t# set syscall type to print character\n");
	strcat(str, "\tsyscall\t\t\t\t# syscall\n");
	strcat(str, "\tlw\t$ra, 4($sp)\t\t# restore register ra from stack\n");
	strcat(str, "\tlw\t$fp, 0($sp)\t\t# restore register fp from stack\n");
	strcat(str, "\tadd\t$sp, $sp, 16\t\t# exit print_int: pop space off of stack\n");
	strcat(str, "\tjr\t$ra\t\t\t# return\n");
	fprintf(stdout, "%s", str);
	fprintf(out, "%s", str);

	/* Output static mips code for read_int() definition */
	strcpy(str, "func_read_int:\n");
	strcat(str, "\tsub\t$sp, $sp, 8\t\t# enter read_int: push space onto stack\n");
	strcat(str, "\tsw\t$ra, 4($sp)\t\t# save register ra on stack\n");
	strcat(str, "\tsw\t$fp, 0($sp)\t\t# save register fp on stack\n");
	strcat(str, "\tmove\t$fp, $sp\t\t# update fp with current sp\n");

	strcat(str, "\tla\t$a0, intprompt\t\t# copy down int parameter from caller's stack\n");
	strcat(str, "\tli\t$v0, 4\t\t\t# set syscall type to print integer\n");
	strcat(str, "\tsyscall\t\t\t\t# syscall\n");

	strcat(str, "\tli\t$v0, 5\t\t\t# set syscall type to read integer\n");
	strcat(str, "\tsyscall\t\t\t\t# syscall\n");
	strcat(str, "\tlw\t$ra, 4($sp)\t\t# restore register ra from stack\n");
	strcat(str, "\tlw\t$fp, 0($sp)\t\t# restore register fp from stack\n");
	strcat(str, "\tadd\t$sp, $sp, 8\t\t# exit print_int: pop space off of stack\n");
	strcat(str, "\tjr\t$ra\t\t\t# return\n");
	fprintf(stdout, "%s", str);
	fprintf(out, "%s", str);
}

