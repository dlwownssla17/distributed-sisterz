#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "dchat.h"
#include "model.h"
#include "RMP/rmp.h"
#include "leader.h"

void leader_receive_message(char* buf, rmp_address* recv_addr) {
  // RECEIVE MESSAGE
  char command_type[20];
  char rest_command[MAX_BUFFER_LEN + 1];
  if (sscanf(buf, "%s %[^\n]", command_type, rest_command) == EOF) {
    perror("chat_non_leader: sscanf for command_type");
    exit(1);
  }

  if (!strcmp("ADD_ME", command_type)) {
    // check for duplicate names
    if (contains_participant(rest_command)) {
      char *join_nickname_failure = "JOIN_NICKNAME_FAILURE";

      // send join nickname failure
      if (RMP_sendTo(get_socket_fd(), recv_addr, join_nickname_failure, strlen(join_nickname_failure) + 1) < 0) {
        printf("chat_leader: RMP_sendTo for JOIN_NICKNAME_FAILURE\n");
        exit(1);
      }

      return;
    }

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
      incr_clock(), sender_nickname, payload);

    broadcast_message(message_broadcast);

    printf("%s:: %s\n", sender_nickname, payload);
  } else if (!strcmp("HEARTBEAT", command_type)) {
    // ignore, don't have to do anything on receiving end
  } else {
    printf("Unrecognized command: %s\n", buf);
  }
}

/**
 * Exactly what it sounds like (I'm tired)
 * @param command_buff  [description]
 * @param buff_len      [description]
 * @param join_leave_message [description]
 */
void generate_participant_update(char* command_buff, int buff_len, char* join_leave_message) {
  Participant* leader = get_leader();

  // TODO: handle buffer overflow
  command_buff += snprintf(command_buff, buff_len, "PARTICIPANT_UPDATE @%s:%s:%s ",
    leader->nickname, leader->ip_address, leader->port_num);

  Participant *curr_p;

  // TODO: fix traversal
  TAILQ_FOREACH(curr_p, get_participants_head(), participants) {
    if (!curr_p->is_leader) {
      command_buff += snprintf(command_buff, buff_len, "%s:%s:%s ",
        curr_p->nickname, curr_p->ip_address, curr_p->port_num);
    }
  }

  command_buff += snprintf(command_buff, buff_len, "= %s", join_leave_message);
}
