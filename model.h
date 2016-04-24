#include <sys/queue.h>

#ifndef __MODEL_H__
#define __MODEL_H__

/* Typedef */
// participant struct
typedef struct Participant {
  char *nickname;
  char *ip_address;
  char *port_num;
  int is_leader;
  TAILQ_ENTRY(Participant) participants;
} Participant;

// list of participants
typedef TAILQ_HEAD(ParticipantsHead, Participant) ParticipantsHead;

// empty list of participants, clears leader
int empty_list();

/* Inserts a participant to the list with the given info
 * Sets the global leader variable if necessary and increases number of participants
 */
void insert_participant(char* nickname, char* ip_address, char* port_num, int is_leader);

/* checks if the current list of participants contains a participant with
 * given nickname
 */
int contains_participant(char* nickname);

// Should be called when program first starts to initialized data structures
void model_init();

int get_is_leader();

/* sets the leader to be the node with matching ip address and port number
 * Must also set other is_leader's to false
 */
void set_leader_by_addr(char* ip_address, char* port_num);

/* sets the leader to be the node with matching nickname
 * Must also set other is_leader's to false
 */
void set_leader_by_nickname(char* nickname);

void set_is_leader(int i_is_leader);

Participant* get_leader();

/* Should be used only for iterating */
ParticipantsHead* get_participants_head();

/* Frees the participant and all of its dynamically allocated fields */
void free_participant(Participant* p);

char* get_leader_nickname();

int get_num_participants();

void decr_num_participants();

#endif