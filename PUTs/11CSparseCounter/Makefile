GCOV_OUTPUT = *.gcda *.gcno *.gcov 
GCOV_CCFLAGS = -fprofile-arcs -ftest-coverage
CC     = gcc
CCFLAGS = -I. -Itests -g -O2 $(GCOV_CCFLAGS)

all: test

main.c:
	sh tests/make-tests.sh tests/test_*.c > main.c

test: main.c chunkybar.o tests/test_chunkybar.c tests/CuTest.c main.c
	$(CC) $(CCFLAGS) -o $@ $^
	./test
	gcov -bc main.c chunkybar.c

chunkybar.o: chunkybar.c
	$(CC) $(CCFLAGS) -c -o $@ $^

clean:
	rm -f main.c chunkybar.o test $(GCOV_OUTPUT)
