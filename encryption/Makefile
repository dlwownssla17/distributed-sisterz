CFLAGS=-g -Wall
CC=clang

.PHONY: clean test	

encryption.o: encryption.c encryption.h
	$(CC) $(CFLAGS) -c encryption.c

test_encryption: tests.c encryption.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f *.o

test: test_encryption
	./test_encryption
