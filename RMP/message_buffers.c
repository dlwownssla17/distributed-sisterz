#include <stdlib.h>
#include <string.h>
#include "../structures/map.h"
#include "message_buffers.h"



/*
 * Maps rmp_addresses to long long integers, for use as keys in a map.
 */
long long rmp_address_hash(rmp_address *address)
{
	unsigned long ip = address->sin_addr.s_addr;
	unsigned short port = address->sin_port;
	long long output = 0;
	memcpy(&output, &ip, sizeof(unsigned long));
	memcpy((&output) + sizeof(unsigned long), &port, sizeof(unsigned short));
	return output;
}



struct message_holding_buffer* get_send_buffer()
{
	static struct message_holding_buffer *buffer;
	if(buffer == NULL) {
		buffer = (struct message_holding_buffer *) malloc(sizeof(struct message_holding_buffer));
		if(buffer != NULL) {
			buffer->occupied = 0;
		}
	}
	return buffer;
}



struct message_holding_buffer* get_receive_buffer_for(rmp_address *src_address)
{
	static map *node_to_buffer_map;
	if(node_to_buffer_map == NULL) {
		node_to_buffer_map = map_new();
	}

	long long key = rmp_address_hash(src_address);
	struct message_holding_buffer *message_buffer = map_get(node_to_buffer_map, key);
	if(message_buffer == NULL) {
		message_buffer = malloc(sizeof(struct message_holding_buffer));
		if(message_buffer != NULL) {
			message_buffer->occupied = 0;
			map_put(node_to_buffer_map, key, message_buffer);
		}
	}
	return message_buffer;
}
