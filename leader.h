#include "dchat.h"
#include "model.h"
#include "RMP/rmp.h"

#ifndef __LEADER_H__
#define __LEADER_H__

/**
 * Process a command received by the leader
 * @param buf       The buffer containing the command
 * @param recv_addr The address of the sender
 */
void leader_receive_message(char* buf, rmp_address* recv_addr);

/**
 * Based on data currently in model, creates the payload of a PARTICIPANT_UPDATE
 * with the given message
 * @param command_buff  The buffer into which to write the PARTICIPANT_UPDATE body, not including the
 * "PARTICIPANT_UPDATE" itself.
 * @param buff_len      The length of the buffer
 * @param join_leave_message The message to send along with the update
 */
void generate_participant_update(char* command_buff, int buff_len, char* join_leave_message);

/**
 * Broadcast a message to all others on the chat. This method also checks for
 * unreachable nodes. In the case that a node can't be reached, a PARTICIPANT_UPDATE
 * is sent
 * @param message The message to be sent
 */
void leader_broadcast_message(char* message);

#endif