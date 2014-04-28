CC=gcc
CFLAGS=-g -Wall -Ihashset
VPATH = hashset

all: hashset.o
	$(CC) $(CFLAGS) -o Controller controller.c  hashset/hashset.o valgrindEval.c CFGEval.c gstats.c libxml_commons.c -I/usr/include/libxml2  -Llibconfig -Llibxml2 -lconfig -lxml2

hashset.o:
	cd hashset && make

clean:
	rm -f *.o *.log *.xml all
	rm -rf mutation_out
	cd hashset && make clean

.PHONY: clean
