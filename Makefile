# macros

OBJS = *.o
EXEC = dchat
CFLAGS = -g
CC = cc
DEPS = dchat.h RMP/rmp.h

me_a_sandwich : dchat

# dchat program

dchat : dchat.o

# generic commands

%.o : %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

% : %.o
	$(CC) -o $@ $^

# clean

clean :
	rm -rf $(EXEC) $(OBJS)