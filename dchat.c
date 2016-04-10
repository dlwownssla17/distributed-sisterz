/*** dchat.c ***/

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
#include "dchat.h"

/* global variables */

char username[MAX_BUFFER_LENGTH];

/* functions */

// start a new chat group as the leader
int start_chat(char *usr) {
  
  
  return 0;
}

// join an existing chat group
int join_chat(char *usr, char *addr_port) {
  
  
  return 0;
}

// chat (until user decides to leave chat or crash)
int chat() {
  
  
  return 0;
}

// leave chat (free all relevant data structures)
int exit_chat() {
  
  
  return 0;
}

// run dchat
int main(int argc, char** argv) {
  // error if invalid number of arguments
  if (argc != 2 && argc != 3) {
    printf("Usage: dchat <USER> -OR- dchat <USER> <ADDR:PORT>\n");
    exit(1);
  }
  // start a new chat group as the leader
  else if (argc == 2) {
    start_chat(argv[1]);
  }
  // join an existing chat group
  else {
    join_chat(argv[1], argv[2]);
  }
  
  // chat (while loop until exit chat)
  chat();
  
  // exit chat
  exit_chat();
  
  printf("\nWhuddup!!!\n");
  
  return 0;
}