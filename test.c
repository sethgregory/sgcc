/*
 * Test program suite for SGCC
 * Seth Gregory
 * 
 * shows functionality:
 *  - function calls and stack management
 *  - parameter passing and return values (constant, variable, void)
 *  - local and global variables
 *  - boolean logic
 *  - loops and conditionals
 *  - recursion
 */


/*
 * Function prototypes for hard-coded MIPS functions
 */
int read_int(void);
void print_int(int);
void print_factorial(int a, int b);
void print_square(int a, int b);
void print_fib(int a, int b);

/*
 * Globally declared variables, space is calculated and added to .data segment
 */
int globalint;
short globalshort;
char globalchar[6];
unsigned long int globalusl;

/*
 * Takes an integer and returns the integer value of its factorial
 */
int factorial(int a) {
	if(a==1 || a==0) return 1;
	else {
		int rv;
		rv = a * factorial(a-1);
		return rv;
	}
}

/*
 * Takes an integer and returns the nth value in the Fibonacci sequence as int
 */
int fibonacci(int a) {
	if(a == 0) return 0;
	else if(a == 1) return 1;
	else {
		int fib1, fib2;
		fib1 = fibonacci(a-1);
		fib2 = fibonacci(a-2);
		a = fib1+fib2;
		return a;
	}
}

/*
 * Takes an integer and returns its square as an int.
 */
int square(int a) {
	a = a * a;
	return a;
}

/*
 * C main function
 */
void main(void) {

	/* Local variables */
	int input, output;

	/* Test a very long statement - only uses 6 registers */
	input=10;
	output=5;
	/* really lengthy way to bitwise-or (2|3) */
	globalint = input + output * 1 & 2 | 3 + 4 / 5 * 6;
	/* Should print "3" at the start of each run */
	print_int(globalint);

	/* Endless loop - read an int and perform operations */
	while(1) {
		input = read_int();

		output = factorial(input);
		print_factorial(output, input);

		output = square(input);
		print_square(output, input);

		output = fibonacci(input);
		print_fib(output, input);	
	}

	/* Void return */
	return;
	
}
