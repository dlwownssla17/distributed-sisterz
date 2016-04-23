/*** dchat.h ***/

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

/* structs */

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

/* constants */

// maximum buffer length
#define MAX_BUFFER_LEN 1024

// maximum nickname length
#define MAX_NICKNAME_LEN 20

// maximum ip address length (IPv4)
#define MAX_IP_ADDRESS_LEN 15

// maximum port num length
#define MAX_PORT_NUM_LEN 5

/* functions */

// chat (until user decides to leave chat or crash)
int chat();

// leave chat (free all relevant data structures)
int exit_chat();

#ifdef DEBUG
# define IF_DEBUG(x) x
#else
# define IF_DEBUG(x) ((void)0)
#endif
