CFLAGS=-g -Wall
CC=clang

.PHONY: clean test	

librmp.a: rmp.o protocol.o map.o
	ar -cvq $@ $^

protocol.o: protocol.c protocol.h
	$(CC) $(CFLAGS) -c protocol.c

rmp.o: rmp.c rmp.h protocol.o
	$(CC) $(CFLAGS) -c rmp.c

map.o:
	cd ../structures && make
	mv ../structures/map.o .

test_local: tests/test_local.c librmp.a
	$(CC) $(CFLAGS) -o $@ $^

test_server: tests/test_server.c librmp.a
	$(CC) $(CFLAGS) -o $@ $^

test_client: tests/test_client.c librmp.a
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f *.a *.o tests/*.o

test: test_local test_server test_client
	./test_local
	./test_server &
	sleep 1
	./test_client
