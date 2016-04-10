CFLAGS=-g -Wall
CC=clang

.PHONY: clean test	

librmp.a: rmp.o protocol.o
	ar -cvq $@ rmp.o protocol.o

protocol.o: protocol.c protocol.h
	$(CC) $(CFLAGS) -c protocol.c

rmp.o: rmp.c rmp.h protocol.o
	$(CC) $(CFLAGS) -c rmp.c

test_server: tests/test_server.c librmp.a
	$(CC) $(CFLAGS) -o $@ $^

test_client: tests/test_client.c librmp.a
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f librmp.a *.o tests/*.o

test: test_server test_client
	make clean
	./test_server &
	sleep 1
	./test_client