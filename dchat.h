/*** dchat.h ***/

#ifndef __DCHAT_H__
#define __DCHAT_H__

/* includes */

#include <stdlib.h>
#include <unistd.h>
#include "RMP/rmp.h"
/* constants */

#ifndef MAX_BUFFER_LEN

// maximum buffer length
#define MAX_BUFFER_LEN 1024

// maximum nickname length
#define MAX_NICKNAME_LEN 20

// maximum ip address length (IPv4)
#define MAX_IP_ADDRESS_LEN 15

// maximum port num length
#define MAX_PORT_NUM_LEN 5

#define MESSAGE_ADD_ME "ADD_ME"
#define MESSAGE_PARTICIPANT_UPDATE "PARTICIPANT_UPDATE"
#define MESSAGE_JOIN_FAILURE "JOIN_FAILURE"
#define MESSAGE_LEADER_ID "LEADER_ID"
#define MESSAGE_BROADCAST "MESSAGE_BROADCAST"
#define MESSAGE_REQUEST "MESSAGE_REQUEST"
#define MESSAGE_START_ELECTION "MESSAGE_START_ELECTION"
#define MESSAGE_STOP_ELECTION "MESSAGE_STOP_ELECTION"
#define MESSAGE_HEARTBEAT "HEARTBEAT"

#ifdef DEBUG
# define IF_DEBUG(x) x
#else
# define IF_DEBUG(x) ((void)0)
#endif

#endif

int incr_clock();

void set_clock(int new_num);

int get_socket_fd();

void respond_to_leader_election(rmp_address *recv_addr);

char* get_own_nickname();

#endif