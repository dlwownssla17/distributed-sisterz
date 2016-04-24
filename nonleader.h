#include "dchat.h"
#include "model.h"
#include "RMP/rmp.h"

/**
 * Takes in a command and updates the participant list, leader, num_participants
 * @param command The PARTICIPANT_UPDATE command
 * @param joining If true, prints current participants. Otherwise, prints joining message
 */
void process_participant_update (char* command, int joining);

void non_leader_receive_message (char* buf, rmp_address* recv_addr);