#include <sys/queue.h>

/* Typedef */
// participant struct
typedef struct Participant {
  char *nickname;
  char *ip_address;
  char *port_num;
  int is_leader; // TODO: is this necessary?
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

// Should be called when program first starts to initialized data structures
void model_init();

int get_is_leader();

void set_is_leader(int i_is_leader);

Participant* get_leader();

/* Should be used only for iterating */
ParticipantsHead* get_participants_head();

/* Frees the participant and all of its dynamically allocated fields */
void free_participant(Participant* p);