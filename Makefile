all:
	@flex lex.l
	@bison -d yacc.y
	@gcc parser.c lex.yy.c yacc.tab.c node.c symbol.c irnode.c token.c mips.c -ly -o sgcc
clean:
	@- rm -f lex.yy.* y.tab.* sgcc core *.output *.tab.* *.o *~ .*.swp *.s
test: all
	@echo
	@./sgcc test.c
	@echo
	@spim -file test.s
	@echo
