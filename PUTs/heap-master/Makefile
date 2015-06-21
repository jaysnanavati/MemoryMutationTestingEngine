GCOV_OUTPUT = *.gcda *.gcno *.gcov 
GCOV_CCFLAGS = -fprofile-arcs -ftest-coverage
CC     = gcc
CCFLAGS = -I. -Itests -g $(GCOV_CCFLAGS)


all: test

main.c:
	sh tests/make-tests.sh tests/test_*.c > main.c

test: main.c heap.o tests/test_heap.c tests/CuTest.c main.c
	$(CC) $(CCFLAGS) -o $@ $^
	./test
	gcov -bc heap.c

heap.o: heap.c
	$(CC) $(CCFLAGS) -c -o $@ $^

clean:
	rm -f main.c heap.o $(GCOV_OUTPUT)
