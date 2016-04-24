/* model.c
 * Includes a list of participants, 
 */
#include "model.h"
#include "dchat.h"
#include <string.h>

/* Globals */

int is_leader;
Participant *leader;
ParticipantsHead participants_head;

int num_participants;

/* controller functions */

// Should be called when program first starts to initialized data structures
void model_init() {
  TAILQ_INIT(&participants_head);
  leader = NULL;
  num_participants = 0;
}

// empty list of participants, clears leader
int empty_list() {
  // remove all participants
  Participant *curr_p;
  Participant *prev_p = NULL;
  TAILQ_FOREACH(curr_p, &participants_head, participants) {
    if (prev_p) {
      free_participant(prev_p);
    }
    TAILQ_REMOVE(&participants_head, curr_p, participants);
    prev_p = curr_p;
  }
  if (prev_p) {
    free_participant(prev_p);
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
  TAILQ_INSERT_TAIL(&participants_head, p, participants);

  // update number of participants
  num_participants++;
}

int get_is_leader() {
  return is_leader;
}

void set_is_leader(int i_is_leader) {
  is_leader = i_is_leader;
}

Participant* get_leader() {
  return leader;
}

/* Should be used only for iterating */
ParticipantsHead* get_participants_head() {
  return &participants_head;
}

void free_participant(Participant* p) {
  free(p->nickname);
  free(p->ip_address);
  free(p->port_num);
  free(p);
}

/* sets the leader to be the node with matching ip address and port number
 * Must also set other is_leader's to false
 */
void set_leader_by_addr(char* ip_address, char* port_num) {
  Participant *curr_p;

  TAILQ_FOREACH(curr_p, get_participants_head(), participants) {
    if (!strcmp(ip_address, curr_p->ip_address) && !strcmp(port_num, curr_p->port_num)) {
      leader = curr_p;
      curr_p->is_leader = 1;
    } else {
      curr_p->is_leader = 0;
    }
  }
}

/* sets the leader to be the node with matching nickname
 * Must also set other is_leader's to false
 */
void set_leader_by_nickname(char* nickname) {
  Participant *curr_p;

  TAILQ_FOREACH(curr_p, get_participants_head(), participants) {
    if (!strcmp(nickname, curr_p->nickname)) {
      leader = curr_p;
      curr_p->is_leader = 1;
    } else {
      curr_p->is_leader = 0;
    }
  }
}
