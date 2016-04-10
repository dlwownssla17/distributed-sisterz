#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include "protocol.h"
#include "rmp.h"



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



int RMP_createSocket(rmp_address *address)
{
	// Create socket
	int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(socket_fd == -1) {
		perror(NULL);
		return -1;
	}

	// Bind socket
	int status = bind(socket_fd,
	                  (struct sockaddr *) address, sizeof(rmp_address));
	if(status == -1) {
		perror(NULL);
		return -1;
	}

	return socket_fd;
}



int RMP_sendTo(int socket_fd, rmp_address *destination,
               const void *buffer, int num_bytes)
{
	int num_bytes_sent = send_rmp_datagram(socket_fd, destination,
	                                       DATA, random(),
               		 					   buffer, num_bytes);
	return num_bytes_sent;
}



int RMP_listen(int socket_fd, void *buffer, size_t len,
               rmp_address *src_address)
{
	// Receive a message
	struct sockaddr_in internal_src_address;
	enum message_type type;
	long int message_id;
	int num_bytes_received = receive_rmp_datagram(socket_fd, &internal_src_address,
	                                              &type, &message_id,
	                                              buffer, len);
    if(num_bytes_received == -1) {
        return -1;
    }
    if(src_address != NULL) {
		memcpy(src_address, &internal_src_address, sizeof(rmp_address));
	}

    return num_bytes_received;
}



void RMP_closeSocket(int socket_fd)
{
	close(socket_fd);
}
