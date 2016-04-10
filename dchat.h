/*** dchat.h ***/

/* includes */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/queue.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/* structs */



/* constants */

// maximum buffer length
#define MAX_BUFFER_LEN 128

/* functions */

// start a new chat group as the leader
int start_chat(char *usr);

// join an existing chat group
int join_chat(char *usr, char *addr_port);

// chat (until user decides to leave chat or crash)
int chat();

// leave chat (free all relevant data structures)
int exit_chat();