CFLAGS=-g -Wall
CC=clang

.PHONY: clean test	

libmap.a: map.o
	ar -cvq $@ map.o

map.o: map.c map.h
	$(CC) $(CFLAGS) -c map.c

tests: tests.c libmap.a
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f libmap.a *.o

test: tests
	make clean
	valgrind ./tests
