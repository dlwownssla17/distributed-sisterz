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
char this_nickname[MAX_NICKNAME_LEN + 1];

int is_leader = 0;
Participant *leader;

int clock_num = 0;

ParticipantsHead *participants_head;
int num_participants = 0;

int socket_fd;

const struct timespec JOIN_ATTEMPT_WAIT_TIME = {.tv_sec = 0, .tv_nsec = 5e8};

/* functions */

/**
 * Gets the system's "em1" network interface ip address 
 * @param  ip_address Stores the address in this char* buffer
 * @return 0 if success, 1 otherwise
 */
int get_ip_address(char* ip_address) {
  struct ifaddrs *ifaddr, *curr;
  if (getifaddrs(&ifaddr) == -1) {
    perror("getifaddrs");
    exit(1);
  }
  curr = ifaddr;
  while (curr) {
    if (curr->ifa_addr && curr->ifa_addr->sa_family == AF_INET && !strcmp(curr->ifa_name, "em1")) {
      struct sockaddr_in *p_addr = (struct sockaddr_in *)curr->ifa_addr;
      char *this_ip_address = inet_ntoa(p_addr->sin_addr);
      strncpy(ip_address, this_ip_address, strlen(this_ip_address));
      break;
    }
    curr = curr->ifa_next;
  }
  freeifaddrs(ifaddr);
  if (ip_address == NULL) {
    printf("Could not find ip address.\n");
    exit(1);
  }
  
  return 0;
}

// empty list of participants
int empty_list() {
  // remove all participants
  Participant *curr_p;
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

/* Inserts a participant to the list with the given info
 * Sets the global leader variable if necessary and increases number of participants
 */
void insert_participant(char* nickname, char* ip_address, char* port_num, int is_leader) {
  Participant* p = malloc(sizeof(Participant));
  p->nickname = malloc(MAX_NICKNAME_LEN + 1);
  strncpy(p->nickname, nickname, MAX_NICKNAME_LEN + 1);
  p->ip_address = malloc(MAX_IP_ADDRESS_LEN + 1);
  strncpy(p->ip_address, ip_address, MAX_IP_ADDRESS_LEN + 1);
  p->port_num = malloc(MAX_PORT_NUM_LEN + 1);
  strncpy(p->port_num, port_num, MAX_PORT_NUM_LEN + 1);
  p->is_leader = is_leader;
  
  if (is_leader) {
    leader = p;
  }
  
  // add participant to list of participants
  TAILQ_INSERT_TAIL(participants_head, p, participants);

  // update number of participants
  num_participants++;
}

// start a new chat group as the leader
int start_chat() {
  char ip_address[MAX_IP_ADDRESS_LEN + 1];
  char port_num[MAX_PORT_NUM_LEN + 1];

  // set ip_address
  get_ip_address(ip_address);

  rmp_address this_addr;
  
  // create socket
  if (RMP_getAddressFor(ip_address, "0", &this_addr) == -1) {
    printf("start_chat: RMP_getAddressFor\n");
    exit(1);
  }
  if ((socket_fd = RMP_createSocket(&this_addr)) == -1) {
    printf("start_chat: RMP_createSocket\n");
    exit(1);
  }
  
  // set port_num
  sprintf(port_num, "%d", RMP_getPortFrom(&this_addr));
  
  // set is_leader
  is_leader = 1;
  
  // initialize list of participants
  participants_head = malloc(sizeof(ParticipantsHead));
  TAILQ_INIT(participants_head);
  
  // set leader
  insert_participant(this_nickname, ip_address, port_num, 1);
  
  printf("%s started a new chat, listening on %s:%s\n", this_nickname, ip_address, port_num);
  
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
  char* currPos = strchr(command, '@') + 1;

  // skip leading spaces
  while (currPos[0] == ' ') {
    currPos++;
  }

  int is_first = 1;

  char ip_address[MAX_IP_ADDRESS_LEN + 1];
  char port_num[6];
  char nickname[MAX_NICKNAME_LEN + 1];

  while (currPos[0] != '\0' && currPos[0] != '=') {
    int scan_ret = sscanf(currPos, "%[^@: ]:%[0-9.]:%[0-9]", nickname, ip_address, port_num);
    if (scan_ret == EOF) {
      // EOF or error
      perror("process_participant_update sscanf");
      fprintf(stderr, "At: %s\n", currPos);
      exit(1);
    } else if (scan_ret < 3) {
      fprintf(stderr, "Invalid participant at: %s\n", currPos);
      exit(1);
    }

    // add current participant to list
    // leader always first in list
    insert_participant(nickname, ip_address, port_num, is_first);

    // print this participant
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

  if (joining) {
    printf("\n");
  }

  // get payload
  if (currPos[0] == '=') {
    currPos++;

    // skip leading spaces
    while (currPos[0] == ' ') {
      currPos++;
    }

    // if not new to chat, print out join / leave message
    if (!joining) {
      printf("%s\n", currPos);
    }
  } else {
    fprintf(stderr, "Unexpected EOF in parsing PARTICIPANT_UPDATE\n");
    exit(1);
  }
  // check for error case
}

// join an existing chat group
int join_chat(char *addr_port) {
  int failed_join_attempts;
  rmp_address this_addr, suspected_leader, recv_addr;
  char recv_buff[MAX_BUFFER_LEN];
  char ip_address[MAX_IP_ADDRESS_LEN + 1];
  char port_num[MAX_PORT_NUM_LEN + 1];

  // get own ip address
  get_ip_address(ip_address);

  // create socket
  if (RMP_getAddressFor(ip_address, "0", &this_addr) < 0) {
    fprintf(stderr, "RMP_getAddressFor 1 error\n");
    exit(1);
  }

  if ((socket_fd = RMP_createSocket(&this_addr)) < 0) {
    fprintf(stderr, "RMP_createSocket error\n");
    exit(1);
  }
  
  // get my port_num
  sprintf(port_num, "%d", RMP_getPortFrom(&this_addr));
  
  // set is_leader to false
  is_leader = 0;

  printf("%s joining a new chat on %s, listening on %s:%s\n", this_nickname, addr_port,
    ip_address, port_num);

  // set up suspected_leader, i.e. hint address to use to try to join chat
  // split addr_port into addr and port
  char addr[MAX_IP_ADDRESS_LEN + 1];

  int colon_pos = strcspn(addr_port, ":");

  if (colon_pos == strlen(addr_port)) {
    fprintf(stderr, "Address must be of the form <ip>:<port>\n");
    exit(1);
  }

  strncpy(addr, addr_port, sizeof(addr));
  addr[colon_pos] = '\0';

  char port[6];
  strncpy(port, addr_port + colon_pos + 1, sizeof(port));

  if (RMP_getAddressFor(addr, port, &suspected_leader) < 0) {
    fprintf(stderr, "RMP_getAddressFor 2 error\n");
    exit(1);
  }

  // generate an ADD_ME command
  char add_me_cmd[8 + MAX_NICKNAME_LEN];
  snprintf(add_me_cmd, sizeof(add_me_cmd), "ADD_ME %s", this_nickname);

  // try to join
  for (failed_join_attempts = 0; failed_join_attempts < 10; failed_join_attempts++) {
    // send ADD_ME
    if (RMP_sendTo(socket_fd, &suspected_leader, add_me_cmd, strlen(add_me_cmd) + 1) < 0) {
      fprintf(stderr, "RMP_sendTo error\n");
      exit(1);
    }

    // receive response
    if (RMP_listen(socket_fd, recv_buff, sizeof(recv_buff), &recv_addr) < 0) {
      fprintf(stderr, "RMP_listen error\n");
      exit(1);
    }

    // TODO: check that recv_addr equals suspected_leader
    char command_type[20];
    if (sscanf(recv_buff, "%s ", command_type) == EOF) {
      // EOF or error
      perror("sscanf");
      fprintf(stderr, "On command: %s\n", recv_buff);
      exit(1);
    } else if (!strcmp("PARTICIPANT_UPDATE", command_type)) {
      // initialize list of participants 
      participants_head = malloc(sizeof(ParticipantsHead));
      TAILQ_INIT(participants_head);

      printf("Succeeded, current users:\n");

      process_participant_update(recv_buff, 1);
      return 1;
    } else if (!strcmp("JOIN_FAILURE", command_type)) {
      // wait 500ms and retry
      IF_DEBUG(printf("Join attempt #%d failed\n", failed_join_attempts + 1));
      nanosleep(&JOIN_ATTEMPT_WAIT_TIME, NULL);
      continue;
    } else if (!strcmp("LEADER_ID", command_type)) {
      // parse LEADER_ID message
      if (sscanf(recv_buff, "%s %[0-9.]:%[0-9]", command_type, addr, port) == EOF) {
        // EOF or error
        perror("sscanf");
        fprintf(stderr, "On command: %s\n", recv_buff);
        exit(1);
      }

      // set new leader
      if (RMP_getAddressFor(addr, port, &suspected_leader) < 0) {
        fprintf(stderr, "RMP_getAddressFor 3 error\n");
        exit(1);
      }
      IF_DEBUG(printf("Received LEADER_ID and redirecting\n"));
    } else {
      fprintf(stderr, "Unexpected command: %s\n", recv_buff);
      exit(1);
    }
  }
  
  fprintf(stderr, "Sorry, no chat is active on %s, try again later.\nBye.\n", addr_port);
  exit(1);

  return 0;
}

/**
 * Exactly what it sounds like (I'm tired)
 * @param command_buff  [description]
 * @param buff_len      [description]
 * @param join_leave_message [description]
 */
void generate_participant_update(char* command_buff, int buff_len, char* join_leave_message) {
  // TODO: handle buffer overflow
  command_buff += snprintf(command_buff, buff_len, "PARTICIPANT_UPDATE @%s:%s:%s ",
    leader->nickname, leader->ip_address, leader->port_num);

  Participant *curr_p;

  TAILQ_FOREACH(curr_p, participants_head, participants) {
    if (!curr_p->is_leader) {
      command_buff += snprintf(command_buff, buff_len, "%s:%s:%s ",
        curr_p->nickname, curr_p->ip_address, curr_p->port_num);
    }
  }

  command_buff += snprintf(command_buff, buff_len, "= %s", join_leave_message);
}

void broadcast_message(char* message) {
  Participant *curr_p;
  Participant *prev_p = NULL;
  int remove_next = 0;

  rmp_address curr_address;

  int message_len = strlen(message);

  int need_update = 0;

  char update_buff[MAX_BUFFER_LEN];
  int update_buff_pos = 0;

  TAILQ_FOREACH(curr_p, participants_head, participants) {
    if (prev_p && remove_next) {
      // free previous
      free(prev_p->nickname);
      free(prev_p->ip_address);
      free(prev_p->port_num);
      free(prev_p);
    }

    remove_next = 0;

    if (!curr_p->is_leader) {
      // get address
      if (RMP_getAddressFor(curr_p->ip_address, curr_p->port_num, &curr_address) < 0) {
        fprintf(stderr, "Broadcast RMP_getAddressFor error\n");
        exit(1);
      }
      // send message
      if (RMP_sendTo(socket_fd, &curr_address, message, message_len + 1) < 0) {
        // node could not be reached, so remove it and PARTICIPANT_UPDATE
        need_update = 1;
        update_buff_pos += snprintf(update_buff + update_buff_pos, MAX_BUFFER_LEN - update_buff_pos,
          "%s, ", curr_p->nickname);
        // logically delete
        TAILQ_REMOVE(participants_head, curr_p, participants);
        // mark for deletion node
        remove_next = 1;
      }
    }
  }

  if (prev_p && remove_next) {
    free(prev_p->nickname);
    free(prev_p->ip_address);
    free(prev_p->port_num);
    free(prev_p);
  }

  if (need_update) {
    char update_command[1024];
    char leave_message[1024];

    // get rid of last comma
    update_buff[update_buff_pos - 2] = '\0';
    snprintf(leave_message, 1024, "NOTICE %s left the chat or crashed", update_buff);
    generate_participant_update(update_command, sizeof(update_command), leave_message);
    broadcast_message(update_command);
    printf("%s\n", leave_message);
  }
}

void leader_receive_message(char* buf, rmp_address* recv_addr) {
  // RECEIVE MESSAGE
  char command_type[20];
  char rest_command[MAX_BUFFER_LEN + 1];
  if (sscanf(buf, "%s %[^\n]", command_type, rest_command) == EOF) {
    perror("chat_non_leader: sscanf for command_type");
    exit(1);
  }

  if (!strcmp("ADD_ME", command_type)) {
    // TODO: check for duplicate nicknames

    // create new participant
    char port_num[6];
    sprintf(port_num, "%d", RMP_getPortFrom(recv_addr));

    char* ip_address = inet_ntoa(recv_addr->sin_addr);

    insert_participant(rest_command, ip_address, port_num, 0);

    // send out participant update
    char join_message[MAX_BUFFER_LEN];
    snprintf(join_message, sizeof(join_message), "NOTICE %s joined on %s:%s",
      rest_command, ip_address, port_num);
    char participant_update[MAX_BUFFER_LEN];
    generate_participant_update(participant_update, sizeof(participant_update),
      join_message);
    broadcast_message(participant_update);
    printf("%s\n", join_message);
  } else if (!strcmp("MESSAGE_REQUEST", command_type)) {
    char sender_nickname[MAX_NICKNAME_LEN];
    char payload[MAX_BUFFER_LEN];
    // TODO: check for right # of matches
    if (sscanf(rest_command, "%[^ =]= %[^\n]", sender_nickname, payload) == EOF) {
      perror("chat_non_leader: sscanf for command_type");
      exit(1);
    }

    char message_broadcast[1024];
    snprintf(message_broadcast, sizeof(message_broadcast), "MESSAGE_BROADCAST %d %s= %s",
      clock_num++, sender_nickname, payload);

    broadcast_message(message_broadcast);

    printf("%s:: %s\n", sender_nickname, payload);
  } else {
    printf("Unrecognized command: %s\n", buf);
  }
}

void non_leader_receive_message (char* buf) {
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
    if (sscanf(rest_message, "%d %[^= ]= %[^\n]", &message_id, sender_nickname, message_payload) == EOF) {
      perror("chat_non_leader: sscanf for MESSAGE_BROADCAST");
      exit(1);
    }
    clock_num = message_id;
    printf("%s:: %s\n", sender_nickname, message_payload);
  } else if (!strcmp("PARTICIPANT_UPDATE", message_type)) {
    // receive participant update
    process_participant_update(buf, 0);
  } else if (!strcmp("START_ELECTION", message_type)) {
    // receive start election
    // TODO: elections
  } else if (!strcmp("ADD_ME", message_type)) {
    // receive add me
    // process message request
    char message_request_buf[10 + MAX_IP_ADDRESS_LEN + 1 + MAX_PORT_NUM_LEN + 1];
    snprintf(message_request_buf, sizeof(message_request_buf), "LEADER_ID %s:%s", leader->ip_address, leader->port_num);
    
    rmp_address leader_addr;
    RMP_getAddressFor(leader->ip_address, leader->port_num, &leader_addr);

    // send message request
    if (RMP_sendTo(socket_fd, &leader_addr, message_request_buf, strlen(message_request_buf) + 1) < 0) {
      printf("chat_non_leader: RMP_sendTo for ADD_ME\n");
      exit(1);
    }
  } else {
    // invalid message
    printf("chat_non_leader, invalid message received: %s\n", buf);
    exit(1);
  }
}

// chat
int chat() {
  // set up select
  fd_set all_fds, read_fds;

  // initialize all_fds fd sets
  FD_ZERO(&all_fds);
  
  // add socket fd and stdin fd to all_fds fd set and update current max fd
  FD_SET(socket_fd, &all_fds);
  FD_SET(STDIN_FILENO, &all_fds);
  
  while (1) {
    // update read_fds to all_fds
    read_fds = all_fds;

    // select for non-blocking I/O
    if ((select(socket_fd + 1, &read_fds, NULL, NULL, NULL)) == -1) {
      perror("chat_leader: select");
      exit(1);
    }

    char buf[MAX_BUFFER_LEN];

    if (FD_ISSET(STDIN_FILENO, &read_fds)) {
      // read from std in
      int num_bytes = read(STDIN_FILENO, buf, MAX_BUFFER_LEN);

      if (num_bytes > 1) {
        buf[num_bytes - 1] = '\0';

        if (is_leader) {
          // send out message directly
          char message_broadcast[MAX_BUFFER_LEN];

          snprintf(message_broadcast, sizeof(message_broadcast), "MESSAGE_BROADCAST %d %s= %s",
            clock_num++, this_nickname, buf);

          broadcast_message(message_broadcast);

          printf("%s:: %s\n", this_nickname, buf);
        } else {
          // send message request
          char message_request_buf[MAX_BUFFER_LEN];

          snprintf(message_request_buf, sizeof(message_request_buf), "MESSAGE_REQUEST %s= %s", this_nickname, buf);

          rmp_address leader_addr;

          // get rmp_address of leader
          if (RMP_getAddressFor(leader->ip_address, leader->port_num, &leader_addr) == -1) {
            printf("%s ; %s\n", leader->ip_address, leader->port_num);
            printf("chat: RMP_getAddressFor\n");
            exit(1);
          }
          if ((num_bytes = RMP_sendTo(socket_fd, &leader_addr, message_request_buf,
              strlen(message_request_buf) + 1)) == -1) {
            printf("chat: RMP_sendTo\n");
            exit(1);
          }
        }
      } else if (num_bytes == 0) {
        // CTRL+D was pressed
        break;
      } else {
        perror("chat: read");
      }
    }
    if (FD_ISSET(socket_fd, &read_fds)) {
      // read input over network
      int num_bytes;

      rmp_address recv_addr;

      if ((num_bytes = RMP_listen(socket_fd, buf, MAX_BUFFER_LEN, &recv_addr)) == -1) {
        printf("chat_leader: RMP_listen\n");
        exit(1);
      }

      buf[num_bytes - 1] = '\0';

      if (is_leader) {
        leader_receive_message(buf, &recv_addr);
      } else {
        non_leader_receive_message(buf);
      }
    }
  }
  
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

  // set nickname
  strncpy(this_nickname, argv[1], sizeof(this_nickname));

  // start a new chat group as the leader
  if (argc == 2) {
    start_chat();
  }
  // join an existing chat group
  else {
    join_chat(argv[2]);
  }
  
  // chat (while loop until exit chat)
  chat();
  
  // exit chat
  exit_chat();
  
  return 0;
}
