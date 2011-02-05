#ifndef NODE_H_
#include "node.h"
#endif

#ifndef SYMBOL_H_
#include "symbol.h"
#endif

#ifndef IRNODE_H_
#define IRNODE_H_


/* Special defined registers for IR nodes - map to non-T-registers in MIPS */
#define REGISTERZERO 	65535
#define REGISTERV0		65534
#define REGISTERRA		65533

/* Enumeration type for LVALUE/RVALUE */
typedef enum t_valuetype { L_VALUE, R_VALUE } valuetype;

/* Enumeration type for IR opcodes */
typedef enum t_operation {
	OP_LABEL, OP_FUNCTIONSTART, OP_FUNCTIONCALL, OP_PARAM,
	OP_ADD, OP_SUB, OP_MULT, OP_DIV, OP_MOD,
	OP_ADDI,
	OP_LA, OP_LIW, OP_SIW, OP_MOVE, OP_LI,
	OP_SLT, OP_SLTE, OP_SGT, OP_SGTE, OP_SEQ,
	OP_BLEZ, OP_BGZ, OP_J, OP_JR,
	OP_DO, OP_WHILE, OP_FOR, OP_IF, OP_ELSE,
	OP_BITAND, OP_BITOR,
	OP_EOS} operation;

/* Enumeration type for IR operand types */
typedef enum t_operandtype { OT_NULL, OT_REGISTER, OT_LABEL, OT_IMMEDIATE, OT_POINTER } operandtype;

void print_mips(symboltable *);
int generate_ir_node(node*, valuetype);

/* Typedef for ir_node struct */
typedef struct ir_node {
	struct ir_node *next;
	struct ir_node *prev;
	struct node *opnode;
	operation opcode;
	operandtype operand1_type;
	void *operand1_data;
	operandtype operand2_type;
	void *operand2_data;
	operandtype operand3_type;
	void *operand3_data;
	char *comment;
} ir_node;

/* Function prototypes */
void build_ir(node*);
ir_node *new_ir_node(char*);
void set_operand(ir_node*, int, operandtype, void*);
void set_comment(ir_node*);
void append_ir_node(ir_node *irn);
ir_node *find_ir_node(int, int);
void print_ir_list(void);
char *new_label(char*);
void print_operand(operandtype, void*);


#endif /* IRNODE_H_ */
