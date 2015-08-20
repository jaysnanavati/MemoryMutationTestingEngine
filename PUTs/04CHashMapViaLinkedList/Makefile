GCOV_OUTPUT = *.gcda *.gcno *.gcov 
GCOV_CCFLAGS = -fprofile-arcs -ftest-coverage
CC     = gcc
CCFLAGS = -I. -Itests -g -O2 -fsigned-char $(GCOV_CCFLAGS)


all: test

main.c:
	sh tests/make-tests.sh tests/test*.c > main.c

test: main.c linked_list_hashmap.o tests/test_linked_list_hashmap.c tests/CuTest.c main.c
	$(CC) $(CCFLAGS) -o $@ $^
	./test
	gcov -bc main.c tests/test_linked_list_hashmap.c linked_list_hashmap.c

linked_list_hashmap.o: linked_list_hashmap.c
	$(CC) $(CCFLAGS) -c -o $@ $^

clean:
	rm -f main.c linked_list_hashmap.o tests $(GCOV_OUTPUT)
