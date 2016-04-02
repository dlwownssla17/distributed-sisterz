CFLAGS=-g -Wall
CC=clang

.PHONY: clean test

test_server: tests/test_server.c librmp.a
	$(CC) $(CFLAGS) -o $@ $^

test_client: tests/test_client.c librmp.a
	$(CC) $(CFLAGS) -o $@ $^	

librmp.a: rmp.o
	ar -cvq $@ rmp.o

rmp.o: rmp.c rmp.h
	$(CC) $(CFLAGS) -c rmp.c

clean:
	rm -f librmp.a *.o tests/*.o

test: test_server test_client
	./test_server &
	sleep 1
	./test_client
