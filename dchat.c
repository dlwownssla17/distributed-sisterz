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
#include <sys/time.h>
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
int socket_fd, new_socket_fd;
fd_set all_fds, read_fds;
int fd_max, i;
struct timeval tv;

char buf[MAX_BUFFER_LEN];
int num_bytes;

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
  if (RMP_getAddressFor(ip_address, "0", &rmp_addr) == -1) {
    printf("start_chat: RMP_getAddressFor\n");
    exit(1);
  }
  if ((socket_fd = RMP_createSocket(&rmp_addr)) == -1) {
    printf("start_chat: RMP_createSocket\n");
    exit(1);
  }
  
  // set port_num
  int rmp_port;
  if ((rmp_port = RMP_getPortFrom(&rmp_addr)) == -1) {
    printf("start_chat: RMP_getPortFrom\n");
    exit(1);
  }
  sprintf(port_num, "%d", rmp_port);
  
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

  strncpy(port, addr_port + colon_pos + 1, sizeof(port));

  // get own ip address
  set_ip_address(ip_address);

  // create socket
  RMP_getAddressFor(ip_address, "0", &rmp_addr);
  socket_fd = RMP_createSocket(&rmp_addr);
  
  // print port_num
  sprintf(port_num, "%d", RMP_getPortFrom(&rmp_addr));
  
  // set not is_leader
  is_leader = 0;

  if (RMP_getAddressFor(addr, port, &suspected_leader) < 0) {
    fprintf(stderr, "RMP_getAddressFor error\n");
    exit(1);
  }

  char add_me_cmd[8 + MAX_NICKNAME_LEN];
  snprintf(add_me_cmd, sizeof(add_me_cmd), "ADD_ME %s", nickname);

  for (failed_join_attempts = 0; failed_join_attempts < 10; failed_join_attempts++) {
    // ADD_ME
    RMP_sendTo(socket_fd, &suspected_leader, add_me_cmd, strlen(add_me_cmd) + 1);
    
    // receive response
    RMP_listen(socket_fd, recv_buff, sizeof(recv_buff), &recv_addr);

    // TODO: check that recv_addr equals suspected_leader
    char command_type[20];
    if (sscanf(recv_buff, "%s ", command_type)) {

    }
    if (strcmp("PARTICIPANT_UPDATE", command_type)) {
      
    }

  }
  
  return 0;
}

// chat (until user decides to leave chat or crash)
int chat() {
  
  // TODO: reduce duplicate code between chat_non_leader() and chat_leader()
  
  // non-leader chat
  if (is_leader) {
    chat_non_leader();
  }
  // leader chat
  else {
    chat_leader();
  }
  
  return 0;
}

int select_setup() {
  // set timeout params
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  
  // initialize read_fds and all_fds fd sets
  FD_ZERO(&read_fds);
  FD_ZERO(&all_fds);
  
  // add socket fd and stdin fd to all_fds fd set and update current max fd
  FD_SET(socket_fd, &all_fds);
  FD_SET(STDIN, &all_fds);
  
  fd_max = socket_fd;
  
  return 0;
}

// chat for non-leader
int chat_non_leader() {
  select_setup();
  
  while (1) {
    // update read_fds to all_fds
    read_fds = all_fds;
    
    // select for non-blocking I/O
    if ((select(fd_max + 1, &read_fds, NULL, NULL, &tv)) == -1) {
      perror("chat_non_leader: select");
      exit(1);
    }

    int break_loop = 0;
    // iterate through all handlers
    for (i = 0; i <= fd_max; i++) {
      if (FD_ISSET(i, &read_fds)) {
	// send message or leave chat (Ctrl-D)
	if (i == STDIN) {
	  // error case
	  if ((num_bytes = read(i, buf, MAX_BUFFER_LEN)) == -1) {
	    perror("chat_non_leader: read");
	    exit(1);
	  }
	  // send message
	  else if (num_bytes > 0) {
	    buf[num_bytes - 1] = '\0';
	    
	    // process message request
	    char message_request_buf[MAX_NICKNAME_LEN + 2 + MAX_BUFFER_LEN + 1];
	    snprintf(message_request_buf, sizeof(16 + MAX_NICKNAME_LEN + 2 + MAX_BUFFER_LEN), "MESSAGE_REQUEST %s= %s", nickname, buf);
	    
	    // get rmp_address of leader
	    RMP_getAddressFor(leader->ip_address, leader->port_num, &rmp_addr);
	    if ((num_bytes = RMP_sendTo(socket_fd, &rmp_addr, message_request_buf, num_bytes)) == -1) {
	      printf("chat_non_leader: RMP_sendTo\n");
	      exit(1);
	    }
	  }
	  // leave chat
	  else {
	    break_loop = 1;
	    break;
	  }
	}
	// receive message
	else {
	  // read in message
	  RMP_getAddressFor(leader->ip_address, leader->port_num, &rmp_addr);
	  if ((num_bytes = RMP_listen(socket_fd, buf, MAX_BUFFER_LEN, &rmp_addr)) == -1) {
	    printf("chat_non_leader: RMP_listen\n");
	    exit(1);
	  }
	  buf[num_bytes - 1] = '\0';
	  
	  // case match for message type
	  char message_type[20];
	  char rest_message[MAX_BUFFER_LEN + 1];
	  if (sscanf(buf, "%s %[^\n]", message_type, rest_message) == EOF) {
	    perror("chat_non_leader: sscanf for message_type");
	    exit(1);
	  }
	  // receive message
	  if (!strcmp("MESSAGE_BROADCAST", message_type)) {
	    int message_id;
	    char sender_nickname[MAX_NICKNAME_LEN + 1];
	    char message_payload[MAX_BUFFER_LEN + 1];
	    if (sscanf(rest_message, "%d %s= %[^\n]", message_id, sender_nickname, message_payload) == EOF) {
	      perror("chat_non_leader: sscanf for MESSAGE_BROADCAST");
	      exit(1);
	    }
	    clock_num = message_id;
	    
	  }
	  // receive participant update
	  else if (!strcmp("PARTICIPANT_UPDATE", message_type)) {
	    
	  }
	  // receive start election
	  else if (!strcmp("START_ELECTION", message_type)) {
	    
	  }
	  // receive add me
	  else if (!strcmp("ADD_ME", message_type)) {
	    
	  }
	  // invalid message
	  else {
	    printf("chat_non_leader: Invalid message received.\n");
	    exit(1);
	  }
	}
      }
    }
    // break out of nested loop
    if (break_loop) {
      break;
    }
  }
  
  return 0;
}

// chat for leader
int chat_leader() {
  select_setup();
  
  while (1) {
    // update read_fds to all_fds
    read_fds = all_fds;
    
    // select for non-blocking I/O
    if ((select(fd_max + 1, &read_fds, NULL, NULL, &tv)) == -1) {
      perror("chat_leader: select");
      exit(1);
    }

    int break_loop = 0;
    // iterate through all handlers
    for (i = 0; i <= fd_max; i++) {
      if (FD_ISSET(i, &read_fds)) {
	// send (multicast) message or leave chat (Ctrl-D)
	if (i == STDIN) {
	  
	}
	// receive message
	else {
	  
	}
      }
    }
    // break out of nested loop
    if (break_loop) {
      break;
    }
  }
  
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
  if (strcspn(argv[1], "@ ") < nickname_len) {
    printf("Nickname must not contain spaces or at-symbol.\n");
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