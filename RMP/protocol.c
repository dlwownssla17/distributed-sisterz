#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include "protocol.h"


struct header {
	enum message_type type;
	message_id id;
};



int send_rmp_datagram(int socket_fd, struct sockaddr_in *destination,
                      enum message_type type, message_id id,
               		  const void *payload, int num_bytes)
{
	// Allocate a buffer for [header + payload]
	size_t header_size = sizeof(struct header);
	void *send_buffer = malloc(num_bytes + header_size);
	if(send_buffer == NULL) {
		return -1;
	}

	// Construct the header
	struct header header = { 0 };
	header.type = type;
	header.id = id;

	// Fill the buffer
	memcpy(send_buffer, &header, header_size);
	memcpy(send_buffer + header_size, payload, num_bytes);

	// Send the buffer
	int num_bytes_sent = sendto(socket_fd, send_buffer, num_bytes + header_size, 0, 
	                            (struct sockaddr *) destination,
	                            sizeof(struct sockaddr_in));
	if(num_bytes_sent == -1) {
		perror("sendto failed in send_rmp_datagram");
		free(send_buffer);
		return -1;
	}

	// Cleanup
	free(send_buffer);
	return num_bytes_sent - header_size;
}



int receive_rmp_datagram(int socket_fd, struct sockaddr_in *src_address,
                         enum message_type *type, message_id *id,
						 void *payload, size_t len)
{
	// Allocate a buffer for [header + payload]
	size_t header_size = sizeof(struct header);
	void *receive_buffer = malloc(len + header_size);
	if(receive_buffer == NULL) {
		return -1;
	}

	// Receive a message
	socklen_t address_len = sizeof(struct sockaddr_in);
    int bytes_received = recvfrom(socket_fd, receive_buffer, len + header_size, 0,
                                  (struct sockaddr *) src_address, &address_len);
    if(bytes_received == -1) {
    	if(errno == EAGAIN || errno == EWOULDBLOCK) {  // recvfrom timed out
    		free(receive_buffer);
    		return -2;
    	} else {
	        perror("recvfrom failed in receive_rmp_datagram");
	        free(receive_buffer);
	        return -1;
    	}
    }

    // Parse out the header
    struct header *header = receive_buffer;
    *type = header->type;
    *id = header->id;

    // Parse out the payload
    size_t payload_size = bytes_received - header_size;
    size_t copy_size = len < payload_size ? len : payload_size;
    memcpy(payload, receive_buffer + header_size, copy_size);

    free(receive_buffer);
    return payload_size;
}
