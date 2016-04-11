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
#include <time.h>
#include "dchat.h"
#include "RMP/rmp.h"

/* global variables */

char nickname[MAX_NICKNAME_LEN + 1];
char ip_address[MAX_IP_ADDRESS_LEN + 1];
char port_num[MAX_PORT_NUM_LEN + 1];
int clock_num = 0;
int is_leader = 0;
Participant *leader;

Participant *p;
ParticipantsHead *participants_head;

int num_participants = 0;

rmp_address rmp_addr;
int socket_fd = 0;

struct timespec JOIN_ATTEMPT_WAIT_TIME = {.tv_sec = 0, .tv_nsec = 5e8};

/* functions */

// set ip address
int set_ip_address(char* ip_address) {
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
  set_ip_address(ip_address);
  
  // create socket
  RMP_getAddressFor(ip_address, "0", &rmp_addr);
  socket_fd = RMP_createSocket(&rmp_addr);
  
  // set port_num
  sprintf(port_num, "%d", RMP_getPortFrom(&rmp_addr));
  
  // set is_leader
  is_leader = 1;
  
  // set leader
  leader = malloc(sizeof(Participant));
  leader->nickname = malloc(strlen(nickname) + 1);
  strncpy(leader->nickname, nickname, strlen(nickname));
  leader->ip_address = malloc(strlen(ip_address) + 1);
  strncpy(leader->ip_address, ip_address, strlen(ip_address));
  leader->port_num = malloc(strlen(port_num) + 1);
  strncpy(leader->port_num, port_num, strlen(port_num));
  leader->is_leader = is_leader;
  
  // initialize list of participants
  participants_head = malloc(sizeof(ParticipantsHead));
  TAILQ_INIT(participants_head);
  
  // add participant (leader) to list of participants
  TAILQ_INSERT_TAIL(participants_head, leader, participants);

  // update number of participants
  num_participants++;
  
  printf("%s started a new chat, listening on %s:%s\n", nickname, ip_address, port_num);
  
  return 0;
}

/**
 * Takes in a command and updates the participant list, leader, num_participants
 * @param command The PARTICIPANT_UPDATE command
 * @param joining If true, prints current participants. Otherwise, prints joining message
 */
void process_participant_update (char* command, int joining) {
  // start by clearing current list
  empty_list();

  // skip over "PARTICIPANT_UPDATE"
  char* currPos = strchr(command, '@');

  // skip leading spaces
  while (currPos[0] == ' ') {
    currPos++;
  }

  int is_first = 1;

  char ip_address[MAX_IP_ADDRESS_LEN + 1];
  char port_num[6];
  char nickname[MAX_NICKNAME_LEN + 1];

  Participant *new_participant;

  while (currPos[0] != '\0' && currPos[0] != '=') {
    int scan_ret = sscanf(currPos, "%[^@: ]:%[0-9.]:%[0-9]", nickname, ip_address, port_num);
    if (scan_ret == EOF) {
      // EOF or error
      perror("process_participant_update sscanf");
      fprintf(stderr, "At: %s\n", command);
      exit(1);
    } else if (scan_ret < 3) {
      fprintf(stderr, "Invalid participant at: %s\n", command);
      exit(1);
    }

    // create new participant
    new_participant = malloc(sizeof(Participant));
    new_participant->nickname = malloc(strlen(nickname) + 1);
    strncpy(new_participant->nickname, nickname, strlen(nickname));
    new_participant->ip_address = malloc(strlen(ip_address) + 1);
    strncpy(new_participant->ip_address, ip_address, strlen(ip_address));
    new_participant->port_num = malloc(strlen(port_num) + 1);
    strncpy(new_participant->port_num, port_num, strlen(port_num));
    new_participant->is_leader = is_first;

    if (is_first) {
      leader = new_participant;
    }
    
    // add participant to list of participants
    TAILQ_INSERT_TAIL(participants_head, new_participant, participants);

    num_participants++;

    // print output
    if (joining) {
      printf("%s %s:%s%s\n", nickname, ip_address, port_num, is_first ? " (Leader)" : "");
    }

    // step forward
    int chars_to_skip = strcspn(currPos, " =");

    currPos += chars_to_skip;

    // skip leading spaces
    while (currPos[0] == ' ') {
      currPos++;
    }

    is_first = 0;
  }
  // get payload
  if (currPos[0] == '=') {
    currPos++;

    // skip leading spaces
    while (currPos[0] == ' ') {
      currPos++;
    }

    if (!joining) {
      printf("NOTICE %s\n", currPos);
    }
  } else {
    fprintf(stderr, "Unexpected EOF in parsing PARTICIPANT_UPDATE\n");
    exit(1);
  }
  // check for error case
}

// join an existing chat group
int join_chat(char *nickname, char *addr_port) {
  int failed_join_attempts;
  rmp_address suspected_leader, recv_addr;
  char recv_buff[MAX_BUFFER_LEN];

  // split addr_port into addr and port
  char addr[MAX_IP_ADDRESS_LEN + 1];
  char port[6];

  int colon_pos = strcspn(addr_port, ":");

  if (colon_pos == strlen(addr_port)) {
    fprintf(stderr, "Address must be of the form <ip>:<port>\n");
    exit(1);
  }

  strncpy(addr, addr_port, sizeof(addr));
  addr[colon_pos] = '\0';

  char my_port[6];
  strncpy(my_port, addr_port + colon_pos + 1, sizeof(my_port));

  // get own ip address
  set_ip_address(ip_address);

  // create socket
  RMP_getAddressFor(ip_address, "0", &rmp_addr);
  socket_fd = RMP_createSocket(&rmp_addr);
  
  // print my port_num
  sprintf(port_num, "%d", RMP_getPortFrom(&rmp_addr));
  
  // set is_leader to false
  is_leader = 0;

  printf("%s joining a new chat on %s, listening on %s:%s\n", nickname, addr_port,
    ip_address, port_num);

  if (RMP_getAddressFor(addr, port, &suspected_leader) < 0) {
    fprintf(stderr, "RMP_getAddressFor error\n");
    exit(1);
  }

  char add_me_cmd[8 + MAX_NICKNAME_LEN];
  snprintf(add_me_cmd, sizeof(add_me_cmd), "ADD_ME %s", nickname);

  // try to join
  for (failed_join_attempts = 0; failed_join_attempts < 10; failed_join_attempts++) {
    // send ADD_ME
    if (RMP_sendTo(socket_fd, &suspected_leader, add_me_cmd, strlen(add_me_cmd) + 1) < 0) {
      fprintf(stderr, "RMP_sendTo error\n");
      exit(1);
    }
    
    // receive response
    RMP_listen(socket_fd, recv_buff, sizeof(recv_buff), &recv_addr);

    // TODO: check that recv_addr equals suspected_leader
    char command_type[20];
    if (sscanf(recv_buff, "%s ", command_type) == EOF) {
      // EOF or error
      perror("sscanf");
      fprintf(stderr, "On command: %s\n", recv_buff);
      exit(1);
    } if (strcmp("PARTICIPANT_UPDATE", command_type)) {
      // initialize list of participants 
      participants_head = malloc(sizeof(ParticipantsHead));
      TAILQ_INIT(participants_head);

      printf("Succeeded, current users:\n");

      process_participant_update(recv_buff, 1);
      return 1;
    } else if (strcmp("JOIN_FAILURE", command_type)) {
      // wait 500ms and retry
      IF_DEBUG(printf("Join attempt #%d failed\n", failed_join_attempts + 1));
      nanosleep(&JOIN_ATTEMPT_WAIT_TIME, NULL);
      continue;
    } else if (strcmp("LEADER_ID", command_type)) {
      // parse LEADER_ID message
      if (sscanf(recv_buff, "%s %[0-9.]:%[0-9]", command_type, addr, port) == EOF) {
        // EOF or error
        perror("sscanf");
        fprintf(stderr, "On command: %s\n", recv_buff);
        exit(1);
      }
      // set new leader
      RMP_getAddressFor(addr, port, &suspected_leader);
      IF_DEBUG(printf("Received LEADER_ID and redirecting\n"));
    } else {
      fprintf(stderr, "Unexpected command: %s\n", recv_buff);
      exit(1);
    }
  }
  
  fprintf(stderr, "Sorry, no chat is active on %d, try again later.\nBye.\n", addr_port);
  exit(1);

  return 0;
}

// chat (until user decides to leave chat or crash)
int chat() {
  
  
  return 0;
}

// leave chat (free all relevant data structures)
int exit_chat() {
  // empty list of participants
  empty_list();
  
  // free list of participants
  free(participants_head);
  
  RMP_closeSocket(socket_fd);
  
  printf("\nYou left the chat.\n");
  
  return 0;
}

// empty list of participants
int empty_list() {
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

  // set number of participants back to zero
  num_participants = 0;

  // should have been freed
  leader = NULL;

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
  // error if spaces or at-symbol in nickname
  if (strcspn(argv[1], ":@ ") < nickname_len) {
    printf("Nickname must not contain spaces, colon, or at-symbol.\n");
    exit(1);
  }
    
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