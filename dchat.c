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
int clock_num = 0;
int is_leader = 0;
Participant *leader;

Participant *p;
ParticipantsHead *participants_head;

int num_participants = 0;

rmp_address rmp_addr;
int socket_fd, new_socket_fd;
fd_set all_fds, read_fds;
int fd_max, i;
struct timeval tv;

char buf[MAX_BUFFER_LEN];
int num_bytes;

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
  char* currPos = strchr(command, '@') + 1;

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
      fprintf(stderr, "At: %s\n", currPos);
      exit(1);
    } else if (scan_ret < 3) {
      fprintf(stderr, "Invalid participant at: %s\n", currPos);
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
int join_chat(char *i_nickname, char *addr_port) {
  int failed_join_attempts;
  rmp_address suspected_leader, recv_addr;
  char recv_buff[MAX_BUFFER_LEN];

  // set nickname
  strncpy(nickname, i_nickname, strlen(i_nickname));

  // get own ip address
  set_ip_address(ip_address);

  // create socket
  if (RMP_getAddressFor(ip_address, "0", &rmp_addr) < 0) {
    fprintf(stderr, "RMP_getAddressFor 1 error\n");
    exit(1);
  }

  if ((socket_fd = RMP_createSocket(&rmp_addr)) < 0) {
    fprintf(stderr, "RMP_createSocket error\n");
    exit(1);
  }
  
  // get my port_num
  sprintf(port_num, "%d", RMP_getPortFrom(&rmp_addr));
  
  // set is_leader to false
  is_leader = 0;

  printf("%s joining a new chat on %s, listening on %s:%s\n", nickname, addr_port,
    ip_address, port_num);

  // try to set up suspected_leader
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
  
  fprintf(stderr, "Sorry, no chat is active on %d, try again later.\nBye.\n", addr_port);
  exit(1);

  return 0;
}

// chat (until user decides to leave chat or crash)
int chat() {
  
  // TODO: reduce duplicate code between chat_non_leader() and chat_leader()
  
  // non-leader chat
  if (!is_leader) {
    chat_non_leader();
  } else {
    // leader chat
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
          else if (num_bytes > 1) {
            buf[num_bytes - 1] = '\0';
            
            // process message request
            char message_request_buf[16 + MAX_NICKNAME_LEN + 2 + MAX_BUFFER_LEN + 1];
            snprintf(message_request_buf, sizeof(message_request_buf), "MESSAGE_REQUEST %s= %s", nickname, buf);

            // get rmp_address of leader
            RMP_getAddressFor(leader->ip_address, leader->port_num, &rmp_addr);
            if ((num_bytes = RMP_sendTo(socket_fd, &rmp_addr, message_request_buf,
                strlen(message_request_buf) + 1)) == -1) {
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
          }
          // receive participant update
          else if (!strcmp("PARTICIPANT_UPDATE", message_type)) {
            process_participant_update(buf, 1);
          }
          // receive start election
          else if (!strcmp("START_ELECTION", message_type)) {
            
          }
          // receive add me
          else if (!strcmp("ADD_ME", message_type)) {
            char sender_nickname[MAX_NICKNAME_LEN + 1];
            if (sscanf(rest_message, "%s", sender_nickname) == EOF) {
              perror("chat_non_leader: sscanf for ADD_ME");
              exit(1);
            }
            
            // process message request
            char message_request_buf[10 + MAX_IP_ADDRESS_LEN + 1 + MAX_PORT_NUM_LEN + 1];
            snprintf(message_request_buf, sizeof(message_request_buf), "LEADER_ID %s %s", leader->ip_address, leader->port_num);
            
            // send message request
            if ((num_bytes = RMP_sendTo(socket_fd, &rmp_addr, message_request_buf, strlen(message_request_buf) + 1)) == -1) {
              printf("chat_non_leader: RMP_sendTo for ADD_ME\n");
              exit(1);
            }
          }
          // invalid message
          else {
            printf("chat_non_leader, invalid message received: %s\n", buf);
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

  Participant *curr_p = p;

  TAILQ_FOREACH(curr_p, participants_head, participants) {
    if (!curr_p->is_leader) {
      command_buff += snprintf(command_buff, buff_len, "%s:%s:%s ",
        curr_p->nickname, curr_p->ip_address, curr_p->port_num);
    }
  }

  command_buff += snprintf(command_buff, buff_len, "= %s", join_leave_message);
}

void broadcast_message(char* message) {
  Participant *curr_p = p;
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
    update_buff[update_buff_pos - 2] = ' ';
    update_buff[update_buff_pos - 1] = '\0';
    snprintf(leave_message, 1024, "%s left the chat or crashed", update_buff);
    generate_participant_update(update_command, sizeof(update_command), leave_message);
    broadcast_message(update_command);
  }
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
    int num_bytes;
    char buf[MAX_BUFFER_LEN];
    // iterate through all handlers
    for (i = 0; i <= fd_max; i++) {
      if (FD_ISSET(i, &read_fds)) {
        // send (multicast) message or leave chat (Ctrl-D)
        if (i == STDIN) {
          if ((num_bytes = read(i, buf, MAX_BUFFER_LEN)) == -1) {
            perror("chat_leader: read");
            exit(1);
          }
          if (num_bytes > 1) {

            buf[num_bytes - 1] = '\0';
            char message_broadcast[1024];
            snprintf(message_broadcast, sizeof(message_broadcast), "MESSAGE_BROADCAST %d %s= %s",
              clock_num++, nickname, buf);

            broadcast_message(message_broadcast);

            printf("%s:: %s\n", nickname, buf);
          }
        } else {
          if ((num_bytes = RMP_listen(socket_fd, buf, MAX_BUFFER_LEN, &rmp_addr)) == -1) {
            printf("chat_leader: RMP_listen\n");
            exit(1);
          }

          buf[num_bytes - 1] = '\0';

          // RECEIVE MESSAGE
          char command_type[20];
          char rest_command[MAX_BUFFER_LEN + 1];
          if (sscanf(buf, "%s %[^\n]", command_type, rest_command) == EOF) {
            perror("chat_non_leader: sscanf for command_type");
            exit(1);
          }

          if (!strcmp("ADD_ME", command_type)) {
            // create new participant
            char port_num[6];
            sprintf(port_num, "%d", RMP_getPortFrom(&rmp_addr));

            // TODO: fix other strncpy's to have +1
            Participant *new_participant = malloc(sizeof(Participant));
            new_participant->nickname = malloc(strlen(rest_command) + 1);
            strncpy(new_participant->nickname, rest_command, strlen(nickname) + 1);
            new_participant->ip_address = inet_ntoa(rmp_addr.sin_addr);
            new_participant->port_num = malloc(strlen(port_num) + 1);
            strncpy(new_participant->port_num, port_num, strlen(port_num) + 1);

            new_participant->is_leader = 0;
            
            // add participant to list of participants
            TAILQ_INSERT_TAIL(participants_head, new_participant, participants);

            num_participants++;

            // send out participant update
            char join_message[MAX_BUFFER_LEN];
            snprintf(join_message, sizeof(join_message), "%s has joined on %s:%s",
              rest_command, new_participant->ip_address, new_participant->port_num);
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