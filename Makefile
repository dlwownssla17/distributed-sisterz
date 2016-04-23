# macros

OBJS = *.o
EXEC = dchat
CFLAGS = -g
CC = cc
DEPS = dchat.h RMP/rmp.h

me_a_sandwich : dchat

# dchat program

dchat : dchat.o RMP/librmp.a
	$(CC) -o $@ $^

dchat.o : dchat.c $(DEPS)
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
