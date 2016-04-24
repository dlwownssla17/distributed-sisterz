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

  if (!strcmp(MESSAGE_ADD_ME, command_type)) {
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

    leader_broadcast_message(participant_update);
    printf("%s\n", join_message);
  } else if (!strcmp(MESSAGE_REQUEST, command_type)) {
    char sender_nickname[MAX_NICKNAME_LEN];
    char payload[MAX_BUFFER_LEN];
    // TODO: check for right # of matches
    if (sscanf(rest_command, "%[^ =]= %[^\n]", sender_nickname, payload) == EOF) {
      perror("chat_non_leader: sscanf for command_type");
      exit(1);
    }

    char message_broadcast[1024];
    snprintf(message_broadcast, sizeof(message_broadcast), "%s %d %s= %s",
      MESSAGE_BROADCAST, incr_clock(), sender_nickname, payload);

    leader_broadcast_message(message_broadcast);

    printf("%s:: %s\n", sender_nickname, payload);
  } else if (!strcmp(MESSAGE_HEARTBEAT, command_type)) {
    // ignore, don't have to do anything on receiving end
  } else if(!strcmp(MESSAGE_START_ELECTION, command_type)) {
    respond_to_leader_election(recv_addr);
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
  command_buff += snprintf(command_buff, buff_len, "%s @%s:%s:%s ",
    MESSAGE_PARTICIPANT_UPDATE, leader->nickname, leader->ip_address, leader->port_num);

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

void leader_broadcast_message(char* message) {
  int socket_fd = get_socket_fd();
  Participant *curr_p;
  Participant *prev_p = NULL;
  int remove_next = 0;

  rmp_address curr_address;

  int message_len = strlen(message);

  int need_update = 0;

  char update_buff[MAX_BUFFER_LEN];
  int update_buff_pos = 0;

  // TODO: fix iteration
  TAILQ_FOREACH(curr_p, get_participants_head(), participants) {
    if (prev_p && remove_next) {
      // free previous
      free_participant(prev_p);
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
        TAILQ_REMOVE(get_participants_head(), curr_p, participants);
        // mark for deletion next
        remove_next = 1;
      }
    }
  }

  if (prev_p && remove_next) {
    free_participant(prev_p);
  }

  if (need_update) {
    char update_command[1024];
    char leave_message[1024];

    // get rid of last comma
    update_buff[update_buff_pos - 2] = '\0';
    snprintf(leave_message, 1024, "NOTICE %s left the chat or crashed", update_buff);
    generate_participant_update(update_command, sizeof(update_command), leave_message);
    leader_broadcast_message(update_command);
    printf("%s\n", leave_message);
  }
}
