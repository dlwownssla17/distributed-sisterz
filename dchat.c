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

char nickname[MAX_NICKNAME_LEN + 1];
char ip_address[MAX_IP_ADDRESS_LEN];
int port_num = 0;
int clock_num = 0;
int is_leader = 0;
Participant *leader;

Participant *p;
ParticipantsHead *participants_head;
Message *m;
MessagesHead *messages_head;

int num_participants = 0;

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
  
  
  printf("\nYou left the chat.\n");
  
  return 0;
}

// run dchat
int main(int argc, char** argv) {
  // error if invalid number of arguments
  if (argc != 2 && argc != 3) {
    printf("Usage: dchat <NICKNAME> [<ADDR:PORT>]\n");
    exit(1);
  }
  int nickname_len = strlen(argv[1]);
  // error if too long nickname
  if (nickname_len > MAX_NICKNAME_LEN) {
    printf("Nickname is too long; please keep it under %d characters.\n", MAX_NICKNAME_LEN);
    exit(1);
  }

  // check for spaces in nickname
  char* space = " ";
  if (strcspn(argv[0], space) < nickname_len) {
    printf("Nickname must not contain spaces\n");
    exit(1);
  }

  strcpy(nickname, argv[0]);
    
  // start a new chat group as the leader
  if (argc == 2) {
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
  
  return 0;
}