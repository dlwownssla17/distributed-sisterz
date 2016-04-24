# macros

OBJS = *.o
EXEC = dchat
CFLAGS = -g -Wall
CC = clang
DEPS = dchat.h RMP/rmp.h model.h leader.h nonleader.h

me_a_sandwich : dchat

# dchat program

dchat : dchat.o model.o RMP/librmp.a leader.o nonleader.o
	$(CC) -o $@ $^

dchat.o : dchat.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

model.o : model.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

leader.o : leader.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

nonleader.o : nonleader.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

RMP/librmp.a : 
	cd RMP && make

# generic commands

%.o : %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

% : %.o
	$(CC) -o $@ $^

# clean

clean :
	rm -rf $(EXEC) $(OBJS)
	cd RMP && make clean

# check for todos
todo:
	! grep -n -H --color=always "//\s*TODO" *.c *.h
