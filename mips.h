#ifndef MIPS_H_
#define MIPS_H_

/* Defined constants to be used for the register map */
#define REALREG	0
#define TEMPREG	1

/* Type definition for registermap struct */
typedef struct registermap {
	struct registermap *next;
	int temp;
	char *registername;
} registermap;


/* Function prototypes */
char *getmapping(int);
void setmapping(ir_node*, int);
void emit_function_entry(symboltable*, char*);
void emit_function_exit(char*);
void emit_function_call(symbol*, char*);
int calculate_offsets(tablelist*, int);
void reset_tregs(void);
void print_tregs(void);
void map_registers(ir_node*);
int is_live(int, ir_node*);
void generate_mips(ir_node*, symboltable*);
void ir2mips(ir_node*, char*);
void emit_hardcoded_mips_data(char *);
void emit_hardcoded_mips_text(char *);


#endif /* MIPS_H_ */
