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
#include <ifaddrs.h>
#include "dchat.h"
#include "RMP/rmp.h"

/* global variables */

char nickname[MAX_NICKNAME_LEN + 1];
char ip_address[MAX_IP_ADDRESS_LEN + 1];
char port_num[MAX_PORT_NUM_LEN + 1];
int clock_num = 0; // only used by the leader
int is_leader = 0;
Participant *leader;

Participant *p;
ParticipantsHead *participants_head;
Message *m; // only used by the leader
MessagesHead *messages_head; // only used by the leader

int num_participants = 0;

rmp_address rmp_addr;
int socket_fd = 0;

/* functions */

// set ip address
int set_ip_address() {
  struct ifaddrs *ifaddr, *tmp;
  if (getifaddrs(&ifaddr) == -1) {
    perror("getifaddrs");
    exit(1);
  }
  tmp = ifaddr;
  while (tmp) {
    if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET && !strcmp(tmp->ifa_name, "em1")) {
      struct sockaddr_in *p_addr = (struct sockaddr_in *)tmp->ifa_addr;
      char *this_ip_address = inet_ntoa(p_addr->sin_addr);
      strncpy(ip_address, this_ip_address, strlen(this_ip_address));
      break;
    }
    tmp = tmp->ifa_next;
  }
  freeifaddrs(ifaddr);
  if (ip_address == NULL) {
    printf("Could not find ip address.\n");
    exit(1);
  }
  
  return 0;
}

// start a new chat group as the leader
int start_chat(char *usr) {
  // set nickname
  strncpy(nickname, usr, strlen(usr));
  
  // set ip_address
  set_ip_address();
  
  // create socket
  RMP_getAddressFor(ip_address, "0", &rmp_addr);
  socket_fd = RMP_createSocket(&rmp_addr);
  
  // set port_num
  sprintf(port_num, "%d", RMP_getPortFrom(&rmp_addr));
  
  // set is_leader
  is_leader = 1;
  
  // set leader
  leader = malloc(sizeof(Participant));
  leader->nickname = malloc(strlen(nickname));
  strncpy(leader->nickname, nickname, strlen(nickname));
  leader->ip_address = malloc(strlen(ip_address));
  strncpy(leader->ip_address, ip_address, strlen(ip_address));
  leader->port_num = malloc(strlen(port_num));
  strncpy(leader->port_num, port_num, strlen(port_num));
  leader->is_leader = is_leader;
  
  // initialize list of participants and list of messages
  participants_head = malloc(sizeof(ParticipantsHead));
  TAILQ_INIT(participants_head);
  messages_head = malloc(sizeof(MessagesHead));
  TAILQ_INIT(messages_head);
  
  // add participant (leader) to list of participants
  TAILQ_INSERT_TAIL(participants_head, leader, participants);

  // update number of participants
  num_participants++;
  
  printf("MY NICKNAME: %s\n", nickname);
  printf("MY IP ADDRESS: %s\n", ip_address);
  printf("MY PORT NUM: %s\n", port_num);
  
  return 0;
}

// join an existing chat group
int join_chat(char *usr, char *addr, char *port) {
  int failed_join_attempts;
  rmp_address suspected_leader;

  // get own ip address
  // create socket
  RMP_getAddressFor(ip_address, "0", &rmp_addr);
  socket_fd = RMP_createSocket(&rmp_addr);
  
  // set port_num
  sprintf(port_num, "%d", RMP_getPortFrom(&rmp_addr));
  
  // set is_leader
  is_leader = 0;


  if (RMP_getAddressFor(addr, port, &suspected_leader) < 0) {
    fprintf(stderr, "RMP_getAddressFor error\n");
    exit(1);
  }

  for (failed_join_attempts = 0; failed_join_attempts < 10; failed_join_attempts++) {

  }
  
  return 0;
}

// chat (until user decides to leave chat or crash)
int chat() {
  
  
  return 0;
}

// leave chat (free all relevant data structures)
int exit_chat() {
  // empty list of participants and list of messages
  empty_lists();
  
  // free list of participants and list of messages
  free(participants_head);
  free(messages_head);
  
  RMP_closeSocket(socket_fd);
  
  printf("\nYou left the chat.\n");
  
  return 0;
}

// empty list of participants and list of messages
int empty_lists() {
  // remove all participants
  Participant *curr_p = p;
  Participant *prev_p = NULL;
  TAILQ_FOREACH(curr_p, participants_head, participants) {
    if (prev_p) {
      free(prev_p->nickname);
      free(prev_p->ip_address);
      free(prev_p->port_num);
      free(prev_p);
    }
    TAILQ_REMOVE(participants_head, curr_p, participants);
    prev_p = curr_p;
  }
  if (prev_p) {
    free(prev_p->nickname);
    free(prev_p->ip_address);
    free(prev_p->port_num);
    free(prev_p);
  }

  // remove all messages
  Message *curr_m = m;
  Message *prev_m = NULL;
  TAILQ_FOREACH(curr_m, messages_head, messages) {
    if (prev_m) {
      free(prev_m->msg);
      free(prev_m);
    }
    TAILQ_REMOVE(messages_head, curr_m, messages);
    prev_m = curr_m;
  }
  if (prev_m) {
    free(prev_m->msg);
    free(prev_m);
  }

  // set number of participants back to zero
  num_participants = 0;

  return 0;
}

// run dchat
int main(int argc, char** argv) {
  // error if invalid number of arguments
  if (argc != 2 && argc != 3) {
    printf("Usage: dchat <NICKNAME> [<ADDR> <PORT>]\n");
    exit(1);
  }
  int nickname_len = strlen(argv[1]);
  // error if too long nickname
  if (nickname_len > MAX_NICKNAME_LEN) {
    printf("Nickname is too long; please keep it under %d characters.\n", MAX_NICKNAME_LEN);
    exit(1);
  }
  // error if spaces or at-symbol in nickname
  char *space = "@ ";
  if (strcspn(argv[1], space) < nickname_len) {
    printf("Nickname must not contain spaces or at-symbol.\n");
    exit(1);
  }
    
  // start a new chat group as the leader
  if (argc == 2) {
    start_chat(argv[1]);
  }
  // join an existing chat group
  else {
    join_chat(argv[1], argv[2], argv[3]);
  }
  
  // chat (while loop until exit chat)
  chat();
  
  // exit chat
  exit_chat();
  
  return 0;
}