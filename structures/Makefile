CFLAGS=-g -Wall
CC=clang

.PHONY: clean test	

map.o: map.c map.h
	$(CC) $(CFLAGS) -c map.c

tests: tests.c map.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f *.o

test: tests
	valgrind ./tests
