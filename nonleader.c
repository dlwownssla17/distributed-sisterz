#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "dchat.h"
#include "encryption/encryption.h"
#include "model.h"
#include "RMP/rmp.h"
#include "nonleader.h"

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

void non_leader_receive_message (char* buf, rmp_address* recv_addr) {
  // case match for message type
  char message_type[20];
  char rest_message[MAX_BUFFER_LEN + 1];
  if (sscanf(buf, "%s %[^\n]", message_type, rest_message) == EOF) {
    perror("chat_non_leader: sscanf for message_type");
    exit(1);
  }
  // receive message
  if (!strcmp(MESSAGE_BROADCAST, message_type)) {
    int message_id;
    char sender_nickname[MAX_NICKNAME_LEN + 1];
    char message_payload[MAX_BUFFER_LEN + 1];
    if (sscanf(rest_message, "%d %[^= ]= %[^\n]", &message_id, sender_nickname, message_payload) == EOF) {
      perror("chat_non_leader: sscanf for MESSAGE_BROADCAST");
      exit(1);
    }
    set_clock(message_id);

    char plaintext_buffer[MAX_BUFFER_LEN];
    decrypt(message_payload, strlen(message_payload), ENCRYPTION_KEY, plaintext_buffer, MAX_BUFFER_LEN);
    printf("%s:: %s\n", sender_nickname, plaintext_buffer);
  } else if (!strcmp(MESSAGE_PARTICIPANT_UPDATE, message_type)) {
    // receive participant update
    process_participant_update(buf, 0);
  } else if (!strcmp(MESSAGE_ADD_ME, message_type)) {
    if (is_in_election()) {
      // send failure during election
      RMP_sendTo(get_socket_fd(), recv_addr, MESSAGE_JOIN_FAILURE, strlen(MESSAGE_JOIN_FAILURE) + 1);
    } else {
      // receive add me
      // process message request
      char message_request_buf[10 + MAX_IP_ADDRESS_LEN + 1 + MAX_PORT_NUM_LEN + 1];

      Participant* leader = get_leader();

      snprintf(message_request_buf, sizeof(message_request_buf), "%s %s:%s", MESSAGE_LEADER_ID, leader->ip_address, leader->port_num);

      // send message request
      if (RMP_sendTo(get_socket_fd(), recv_addr, message_request_buf, strlen(message_request_buf) + 1) < 0) {
        printf("chat_non_leader: RMP_sendTo for ADD_ME\n");
        exit(1);
      }
    }
  } else if (!strcmp(MESSAGE_HEARTBEAT, message_type)) {
    // ignore, don't have to do anything on receiving end
  } else if (!strcmp(MESSAGE_START_ELECTION, message_type)) {
    respond_to_leader_election(recv_addr);
  } else if (!strcmp(MESSAGE_STOP_ELECTION, message_type)) {
    stop_election();
  } else if (!strcmp(MESSAGE_ELECTION_VICTORY, message_type)) {
    set_new_leader(recv_addr);
  } else {
    // invalid message
    printf("chat_non_leader, invalid message received: %s\n", buf);
    exit(1);
  }
}

void nonleader_broadcast_message(char* message) {
  int socket_fd = get_socket_fd();

  Participant *curr_p;
  rmp_address curr_address;

  int message_len = strlen(message);

  TAILQ_FOREACH(curr_p, get_participants_head(), participants) {

    if(strcmp(get_own_nickname(), curr_p->nickname) == 0) {
      continue;
    }

    // get address
    if (RMP_getAddressFor(curr_p->ip_address, curr_p->port_num, &curr_address) < 0) {
      fprintf(stderr, "Broadcast RMP_getAddressFor error\n");
      exit(1);
    }
    // send message
    RMP_sendTo(socket_fd, &curr_address, message, message_len + 1);
  }
}
