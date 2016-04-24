#include "dchat.h"
#include "model.h"
#include "RMP/rmp.h"

#ifndef __LEADER_H__
#define __LEADER_H__

void leader_receive_message(char* buf, rmp_address* recv_addr);
/**
 * Exactly what it sounds like (I'm tired)
 * @param command_buff  [description]
 * @param buff_len      [description]
 * @param join_leave_message [description]
 */
void generate_participant_update(char* command_buff, int buff_len, char* join_leave_message);

#endif