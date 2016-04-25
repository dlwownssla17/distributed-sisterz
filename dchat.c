/*** dchat.c ***/

//#define DEBUG 1

/* includes */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <time.h>
#include "dchat.h"
#include "model.h"
#include "RMP/rmp.h"
#include "leader.h"
#include "encryption/encryption.h"
#include "nonleader.h"

/* global variables */
char this_nickname[MAX_NICKNAME_LEN + 1];
int clock_num = 0;
int socket_fd;

// true if an election has been started since the last election victory
int in_election = 0;
// true if local node was the last to send out an ELECTION_START
int started_election = 0;

struct timespec last_election_start;

const struct timespec JOIN_ATTEMPT_WAIT_TIME = {.tv_sec = 0, .tv_nsec = 5e8};
const struct timeval SELECT_PERIOD = {.tv_sec = 0, .tv_usec = 1e5};

const int HEARTBEAT_INTERVAL = 3000;

// number of milliseconds to wait after an ELECTION_START before declaring victory
const int VICTORY_WAIT_MILLIS = 3000;
// number of milliseconds to wait after an ELECTION_START before RESTARTING
// (assuming node that sent ELECTION_START has died)
const int RESTART_WAIT_MILLIS = 6000;

/* functions */

/**
 * Gets the system's "em1" network interface ip address 
 * @param  ip_address Stores the address in this char* buffer
 * @return 0 if success, 1 otherwise
 */
void get_ip_address(char* ip_address) {
  struct ifaddrs *ifaddr, *curr;
  if (getifaddrs(&ifaddr) == -1) {
    perror("getifaddrs");
    exit(1);
  }
  curr = ifaddr;
  while (curr) {
    if (curr->ifa_addr && curr->ifa_addr->sa_family == AF_INET && !strcmp(curr->ifa_name, "em1")) {
      struct sockaddr_in *p_addr = (struct sockaddr_in *)curr->ifa_addr;
      strncpy(ip_address, inet_ntoa(p_addr->sin_addr),
        MAX_IP_ADDRESS_LEN + 1);
      break;
    }
    curr = curr->ifa_next;
  }
  freeifaddrs(ifaddr);
  if (ip_address == NULL) {
    printf("Could not find ip address.\n");
    exit(1);
  }
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
  
  model_init();

  set_is_leader(1);

  // set leader
  insert_participant(this_nickname, ip_address, port_num, 1);
  
  printf("%s started a new chat, listening on %s:%s\n", this_nickname, ip_address, port_num);

  printf("Waiting for others to join...\n");
  
  return 0;
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
  
  // initialize participant list
  model_init();

  set_is_leader(0);

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
  char add_me_cmd[sizeof(MESSAGE_ADD_ME) + MAX_NICKNAME_LEN + 2];
  snprintf(add_me_cmd, sizeof(add_me_cmd), "%s %s", MESSAGE_ADD_ME, this_nickname);

  // try to join
  for (failed_join_attempts = 0; failed_join_attempts < 10; failed_join_attempts++) {
    // send ADD_ME
    if (RMP_sendTo(socket_fd, &suspected_leader, add_me_cmd, strlen(add_me_cmd) + 1) < 0) {
      fprintf(stderr, "RMP_sendTo error\n");
      exit(1);
    }

    // receive response
    int status;
    while((status = RMP_listen(socket_fd, recv_buff, sizeof(recv_buff), &recv_addr)) == 0);
    if (status < 0) {
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
    } else if (!strcmp(MESSAGE_PARTICIPANT_UPDATE, command_type)) {

      printf("Succeeded, current users:\n");

      process_participant_update(recv_buff, 1);
      return 1;
    } else if (!strcmp(MESSAGE_JOIN_NICKNAME_FAILURE, command_type)) {
      printf("Participant with nickname '%s' already exists.\n", this_nickname);
      exit(1);
    } else if (!strcmp(MESSAGE_JOIN_FAILURE, command_type)) {
      // wait 500ms and retry
      IF_DEBUG(printf("Join attempt #%d failed\n", failed_join_attempts + 1));
      nanosleep(&JOIN_ATTEMPT_WAIT_TIME, NULL);
      continue;
    } else if (!strcmp(MESSAGE_LEADER_ID, command_type)) {
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

void get_leader_addr(rmp_address* leader_addr) {
  Participant* leader = get_leader();

  // get rmp_address of leader
  RMP_getAddressFor(leader->ip_address, leader->port_num, leader_addr);
}

void recv_stdin(char* buf, int num_bytes) {
  char ciphertext_buffer[MAX_BUFFER_LEN];
  encrypt(buf, num_bytes, ENCRYPTION_KEY, ciphertext_buffer, MAX_BUFFER_LEN);

  if (get_is_leader()) {
    // send out message directly
    char message_broadcast[MAX_BUFFER_LEN];

    snprintf(message_broadcast, sizeof(message_broadcast), "%s %d %s= %s",
      MESSAGE_BROADCAST, clock_num++, this_nickname, ciphertext_buffer);

    leader_broadcast_message(message_broadcast);

    printf("%s:: %s\n", this_nickname, buf);
  } else {
    // send message request
    char message_request_buf[MAX_BUFFER_LEN];

    snprintf(message_request_buf, sizeof(message_request_buf), "%s %s= %s", MESSAGE_REQUEST, this_nickname, ciphertext_buffer);

    rmp_address leader_addr;

    get_leader_addr(&leader_addr);

    int status = RMP_sendTo(socket_fd, &leader_addr, message_request_buf, strlen(message_request_buf) + 1);
    if (status == RMP_E_TIMEOUT) {
      start_election();
    } else if (status < 0){
      perror("recv_stdin non-leader\n");
      exit(1);
    }
  }
}

// Send out a START_ELECTION
void start_election() {
  IF_DEBUG(printf("starting election\n"));
  nonleader_broadcast_message(MESSAGE_START_ELECTION);
  in_election = 1;
  started_election = 1;
  clock_gettime(CLOCK_MONOTONIC, &last_election_start);
}

/* Receive a ELECTION_STOP message */
void stop_election() {
  IF_DEBUG(printf("receive ELECTION_STOP\n"));

  in_election = 1;
  started_election = 0;
}

void respond_to_leader_election(rmp_address *recv_addr)
{
  IF_DEBUG(printf("respond_to_leader_election\n"));
  // Get initiator nickname
  Participant *curr_p;
  rmp_address participant_address;
  char *sender_nickname = NULL;
  TAILQ_FOREACH(curr_p, get_participants_head(), participants) {
    RMP_getAddressFor(curr_p->ip_address, curr_p->port_num, &participant_address);
    if (memcmp(recv_addr, &participant_address, sizeof(rmp_address)) == 0) {
      sender_nickname = curr_p->nickname;
      break;
    }
  }

  if (sender_nickname == NULL) {
    fprintf(stderr, "Unknown sender nickname\n");
  }

  if(strcmp(this_nickname, sender_nickname) > 0) {
    RMP_sendTo(socket_fd, recv_addr, MESSAGE_STOP_ELECTION, sizeof(MESSAGE_STOP_ELECTION) + 1);
    // start new election
    start_election();
  } else {
    // heed to new election
    in_election = 1;
    started_election = 0;
    clock_gettime(CLOCK_MONOTONIC, &last_election_start);
  }
}

/* After receiving ELECTION_VICTORY */
void set_new_leader(rmp_address* recv_addr) {
  IF_DEBUG(printf("Setting new leader\n"));
  in_election = 0;
  started_election = 0;
  char port_num[MAX_PORT_NUM_LEN + 1];

  // get leader's port_num
  sprintf(port_num, "%d", RMP_getPortFrom(recv_addr));

  // get leader's ip
  char* ip_address = inet_ntoa(recv_addr->sin_addr);

  set_leader_by_addr(ip_address, port_num);

  set_is_leader(0);
}

void declare_victory() {
  IF_DEBUG(printf("Declaring victory\n"));
  in_election = 0;
  started_election = 0;

  set_leader_by_nickname(this_nickname);

  set_is_leader(1);

  leader_broadcast_message(MESSAGE_ELECTION_VICTORY);

  char update_command[MAX_BUFFER_LEN], victory_message[MAX_BUFFER_LEN];

  snprintf(victory_message, 1024, "NOTICE %s is the new leader", this_nickname);
  generate_participant_update(update_command, sizeof(update_command), victory_message);
  leader_broadcast_message(update_command);
  printf("%s\n", victory_message);

  if (get_num_participants() == 1) {
    printf("Waiting for others to join...\n");
  }
}

void send_heartbeat() {
  char* heartbeat = MESSAGE_HEARTBEAT;

  if (get_is_leader()) {
    // send to everyone
    leader_broadcast_message(heartbeat);
  } else {
    // send only to leader
    rmp_address leader_addr;

    get_leader_addr(&leader_addr);

    int status = RMP_sendTo(socket_fd, &leader_addr, heartbeat, strlen(heartbeat) + 1);
    if (status == RMP_E_TIMEOUT) {
      start_election();
    } else if (status < 0) {
      perror("heartbeat non-leader");
      exit(1);
    }
  }
}

// chat
void chat() {
  // set up select
  fd_set all_fds, read_fds;

  // initialize all_fds fd sets
  FD_ZERO(&all_fds);
  
  // add socket fd and stdin fd to all_fds fd set and update current max fd
  FD_SET(socket_fd, &all_fds);
  FD_SET(STDIN_FILENO, &all_fds);
  
  // set up heartbeat state
  struct timespec last_heartbeat;

  clock_gettime(CLOCK_MONOTONIC, &last_heartbeat);

  while (1) {
    // update read_fds to all_fds
    read_fds = all_fds;

    struct timeval select_period = SELECT_PERIOD;

    // select for non-blocking I/O
    if ((select(socket_fd + 1, &read_fds, NULL, NULL, &select_period)) == -1) {
      perror("chat_leader: select");
      exit(1);
    }

    char buf[MAX_BUFFER_LEN];

    if (FD_ISSET(STDIN_FILENO, &read_fds)) {
      // read from std in
      int num_bytes = read(STDIN_FILENO, buf, MAX_BUFFER_LEN);

      if (num_bytes > 1) {
        // if only one participant, just wait
        if (get_num_participants() != 1) {
          buf[num_bytes - 1] = '\0';

          recv_stdin(buf, num_bytes);
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
        perror("chat_leader: RMP_listen");
        exit(1);
      } else if(num_bytes == 0) {
        continue;
      }

      buf[num_bytes - 1] = '\0';

      if (get_is_leader()) {
        leader_receive_message(buf, &recv_addr);
      } else {
        non_leader_receive_message(buf, &recv_addr);
      }
    }

    struct timespec curr_time;
    clock_gettime(CLOCK_MONOTONIC, &curr_time);


    if (!in_election && TIME_DIFF(curr_time, last_heartbeat) >= HEARTBEAT_INTERVAL) {
      // heartbeat
      send_heartbeat();

      // reset for next heartbeat
      last_heartbeat = curr_time;
    }

    if (!get_is_leader()) {
      if (in_election && started_election
          && TIME_DIFF(curr_time, last_election_start) >= VICTORY_WAIT_MILLIS) {
        declare_victory();
      } else if (in_election && !started_election
          && TIME_DIFF(curr_time, last_election_start) >= RESTART_WAIT_MILLIS) {
        start_election();
      }
    }
  }
}

// leave chat (free all relevant data structures)
void exit_chat() {
  // empty list of participants
  empty_list();
  
  RMP_closeSocket(socket_fd);
  
  printf("\nYou left the chat.\n");
}

// run dchat
int main(int argc, char** argv) {
  // set line buffering
  setvbuf(stdout, NULL, _IOLBF, 1024);
  setvbuf(stderr, NULL, _IOLBF, 1024);

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
  } else {
    // join an existing chat group
    join_chat(argv[2]);
  }
  
  // chat (while loop until exit chat)
  chat();
  
  // exit chat
  exit_chat();
  
  return 0;
}

int incr_clock() {
  return clock_num++;
}

void set_clock(int new_num) {
  clock_num = new_num;
}

int get_socket_fd() {
  return socket_fd;
}

char* get_own_nickname() {
  return this_nickname;
}

int is_in_election() {
  return in_election;
}

void set_in_election(int i) {
  in_election = i;
}
