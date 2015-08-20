GCOV_OUTPUT = *.gcda *.gcno *.gcov 
GCOV_CCFLAGS = -fprofile-arcs -ftest-coverage
CC     = gcc
CCFLAGS = -I. -Itests -g -fsigned-char $(GCOV_CCFLAGS)


all: test

main.c:
	sh tests/make-tests.sh tests/test_*.c > main.c

test: main.c avl_tree.o tests/test_avl_tree.c tests/CuTest.c main.c
	$(CC) $(CCFLAGS) -o $@ $^
	./test
	gcov -bc main.c tests/test_avl_tree.c avl_tree.c

avl_tree.o: avl_tree.c
	$(CC) $(CCFLAGS) -c -o $@ $^

clean:
	rm -f main.c avl_tree.o test $(GCOV_OUTPUT)
