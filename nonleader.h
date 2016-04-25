#include "dchat.h"
#include "model.h"
#include "RMP/rmp.h"

/**
 * Takes in a command and updates the participant list, leader, num_participants
 * @param command The PARTICIPANT_UPDATE command
 * @param joining If true, prints current participants. Otherwise, prints joining message
 */
void process_participant_update (char* command, int joining);

/**
 * Process a command received by a non-leader
 * @param buf       The buffer containing the command
 * @param recv_addr The address of the sender
 */
void non_leader_receive_message (char* buf, rmp_address* recv_addr);

/**
 * Sends a command from a non-leader node. The command is best-effort. Nothing is done if
 * the recipient is unreachable. This function is only used during elections.
 * 
 * @param message The command to send
 */
void nonleader_broadcast_message(char* message);
