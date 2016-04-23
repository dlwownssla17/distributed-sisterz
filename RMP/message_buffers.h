#ifndef __MESSAGE_BUFFERS_H__
#define __MESSAGE_BUFFERS_H__

#include "rmp.h"
#include "protocol.h"

struct message_holding_buffer {
	int occupied;
	enum message_type type;
	message_id id;
	char *message;
	size_t message_length;
};

/*
 * Returns a pointer to the current send buffer.
 * The send buffer holds up to one message that is in the process of being sent.
 * Allocates the buffer if it does not already exist.
 */
struct message_holding_buffer* get_send_buffer();

/*
 * Returns a pointer to the current receive buffer for a given address.
 * Internally, the receive buffer maps rmp_address's to buffers of up to one
 * message from that address.
 * Allocates the buffer if it does not already exist.
 */
struct message_holding_buffer* get_receive_buffer_for(rmp_address *src_address);

#endif