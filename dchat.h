/*** dchat.h ***/

#ifndef __DCHAT_H__
#define __DCHAT_H__

/* includes */

#include <stdlib.h>
#include <unistd.h>
#include "RMP/rmp.h"
/* constants */

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
#define MESSAGE_START_ELECTION "START_ELECTION"
#define MESSAGE_STOP_ELECTION "ELECTION_STOP"
#define MESSAGE_ELECTION_VICTORY "ELECTION_VICTORY"
#define MESSAGE_HEARTBEAT "HEARTBEAT"

#ifdef DEBUG
# define IF_DEBUG(x) x
#else
# define IF_DEBUG(x) ((void)0)
#endif

// returns the difference between two timespecs in milliseconds
#define TIME_DIFF(x, y) ((x.tv_sec - y.tv_sec) * 1000 + (x.tv_nsec - y.tv_nsec) / 1000000L)

int incr_clock();

void set_clock(int new_num);

int get_socket_fd();

void respond_to_leader_election(rmp_address *recv_addr);

char* get_own_nickname();

int is_in_election();

void set_in_election(int i);

// Send out a START_ELECTION
void start_election();

/* Receive a ELECTION_STOP message */
void stop_election();

/* After receiving ELECTION_VICTORY */
void set_new_leader(rmp_address* recv_addr);

#endif