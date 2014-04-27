CC     = gcc
CCFLAGS = -g -fno-omit-frame-pointer -fno-common -fsigned-char 

all: tests

main.c:
	sh make-tests.sh > main.c

tests: main.c fixed_arraylist.o test_fixedarraylist.c CuTest.o main.c
	$(CC) $(CCFLAGS) -o $@ $^
	./tests

fixed_arraylist.o: fixed_arraylist.c
	$(CC) $(CCFLAGS) -c -o $@ $^

CuTest.o: CuTest.c
	$(CC) $(CCFLAGS) -c -o $@ $^

clean:
	rm -f main.c fixed_arraylist.o tests
