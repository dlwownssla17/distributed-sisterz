#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include "protocol.h"
#include "../structures/map.h"
#include "rmp.h"

struct message_holding_buffer {
	int occupied;
	enum message_type type;
	message_id id;
	char *message;
	size_t message_length;
};



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


/*
 * Receives and processes a message, placing its information in the given
 * arguments.
 * Returns the number of bytes received on success and -1 on failure.
 */
int receive_and_process_message(int socket_fd, struct sockaddr_in *src_address,
                                enum message_type *type, message_id *id,
                                void *result_buffer, size_t buffer_size)
{
	// Wait for a message
	int num_bytes_received = receive_rmp_datagram(socket_fd,
	                                              src_address, type, id,
	                                              result_buffer, buffer_size);
    if(num_bytes_received == -1) {
        return -1;
    }

    // Process the message
    struct message_holding_buffer *message_buffer;
    int status;
    switch(*type) {
    	case DATA:
    	    message_buffer = get_receive_buffer_for(src_address);
    		// Clear any previous data
    		if(message_buffer->occupied) {
    			free(message_buffer->message);
    		}

    		// Store the message and ACK it
			message_buffer->occupied = 1;
			message_buffer->type = *type;
			message_buffer->id = *id;
			message_buffer->message = (char *) malloc(num_bytes_received * sizeof(char *));
			memcpy(message_buffer->message, result_buffer, num_bytes_received);
			message_buffer->message_length = num_bytes_received;
    		status = send_rmp_datagram(socket_fd, src_address, ACK, *id, NULL, 0);
    		if(status == -1) {
    			return -1;
    		}
    		break;
    	case ACK:
    		// Unconditionally send a SYN_ACK
    		status = send_rmp_datagram(socket_fd, src_address, SYN_ACK, *id, NULL, 0);
    		if(status == -1) {
    			return -1;
    		}
    		// Clear the send buffer if applicable
    		struct message_holding_buffer *message_buffer = get_send_buffer();
    		if(message_buffer->occupied && *id == message_buffer->id) {
    			message_buffer->occupied = 0;
    		}
    		break;
    	case SYN_ACK:
    		// If applicable, cheat and return the original message's buffer
    		// and length.
    	    message_buffer = get_receive_buffer_for(src_address);
    	    if(message_buffer->occupied && *id == message_buffer->id) {
    	    	size_t copy_size = buffer_size < message_buffer->message_length ? buffer_size : message_buffer->message_length;
    	    	memcpy(result_buffer, message_buffer->message, copy_size);
    	    	num_bytes_received = message_buffer->message_length;
    	    	message_buffer->occupied = 0;
    	    	free(message_buffer->message);
    	    }
    		break;
    }

    return num_bytes_received;
}



int RMP_getAddressFor(const char *ip, const char *port, rmp_address *address_dst)
{
	// Set up connection information
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	struct addrinfo *node_info;
	int status = getaddrinfo(ip, port, &hints, &node_info);
	if(status != 0) {
		fprintf(stderr, "%s\n", gai_strerror(status));
		return -1;
	}

	memcpy(address_dst, node_info->ai_addr, sizeof(rmp_address));
	freeaddrinfo(node_info);
	return 0;
}



int RMP_getPortFrom(rmp_address *address)
{
	return ntohs(address->sin_port);
}



int RMP_createSocket(rmp_address *address)
{
	// Create socket
	int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(socket_fd == -1) {
		perror("socket failed in RMP_createSocket");
		return -1;
	}

	// Bind socket
	int status = bind(socket_fd,
	                  (struct sockaddr *) address, sizeof(rmp_address));
	if(status == -1) {
		perror("bind failed in RMP_createSocket");
		return -1;
	}

	// Update address
	socklen_t address_size = sizeof(rmp_address);
	status = getsockname(socket_fd,
	                     (struct sockaddr *) address, &address_size);
	if(status == -1) {
		perror("getsockname failed in RMP_createSocket");
	}

	return socket_fd;
}



int RMP_sendTo(int socket_fd, rmp_address *destination,
               const void *buffer, int num_bytes)
{
	// Send data message
	message_id new_id = random();
	enum message_type type = DATA;
	int num_bytes_sent = send_rmp_datagram(socket_fd, destination,
	                                       type, new_id,
               		 					   buffer, num_bytes);

	struct message_holding_buffer* outgoing_buffer = get_send_buffer();
	outgoing_buffer->occupied = 1;
	outgoing_buffer->type = type;
	outgoing_buffer->id = new_id;
	outgoing_buffer->message = (void *)buffer;
	outgoing_buffer->message_length = num_bytes;

	// Wait for ACK
	struct sockaddr_in recv_src_address;
	enum message_type recv_type;
	message_id recv_id;
	char *result_buffer = (char *) malloc(MAX_MESSAGE_SIZE * sizeof(char *));
	do {
		int status = receive_and_process_message(socket_fd, &recv_src_address,
	                                &recv_type, &recv_id,
	                                result_buffer, MAX_MESSAGE_SIZE);
		if(status == -1) {
			free(result_buffer);
			return -1;
		}		
	} while(memcmp(&recv_src_address, destination, sizeof(rmp_address)) != 0 ||
	        recv_type != ACK ||
	        recv_id != new_id);

	free(result_buffer);
	return num_bytes_sent;
}



int RMP_listen(int socket_fd, void *buffer, size_t len, rmp_address *src_address)
{
	struct sockaddr_in recv_src_address;
	enum message_type recv_type;
	message_id recv_id;
	int num_bytes_received;
	do {
		num_bytes_received = receive_and_process_message(socket_fd, &recv_src_address,
	                                			 			 &recv_type, &recv_id,
	                                			 			 buffer, len);
		if(num_bytes_received == -1) {
			return -1;
		}		
	} while(recv_type != SYN_ACK);

    // Other stuff
    if(src_address != NULL) {
		memcpy(src_address, &recv_src_address, sizeof(rmp_address));
	}

    return num_bytes_received;
}



void RMP_closeSocket(int socket_fd)
{
	close(socket_fd);
}
