CFLAGS=-Wall -Wpedantic -std=c99 

all: main

main: lisp.o reader.o
	gcc -c main.c -o main.o
	gcc main.o lisp.o reader.o tokenizer.o runtime_functions.o -o lisp_main

tests: lisp.o
	gcc -c lisp_test.c -o lisp_test.o $(CFLAGS)
	gcc lisp_test.o lisp.o -o lisp_test

reader.o: tokenizer.o lisp.o
	gcc -c reader.c -o reader.o $(CFLAGS)

lisp.o: runtime_functions.o
	gcc -c lisp.c -o lisp.o $(CFLAGS)

tokenizer.o:
	flex lex.yy
	gcc -c lex.yy.c -o tokenizer.o

runtime_functions.o:
	gcc -c runtime_functions.c -o runtime_functions.o $(CFLAGS)

clean:
	rm -f *.o
	rm -f lisp_test
	rm -f lisp_main
