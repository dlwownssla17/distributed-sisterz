#ifndef __RMP_H__
#define __RMP_H__

#include <netinet/in.h>

#define MAX_MESSAGE_SIZE 65000
#define ACK_TIMEOUT_USEC 20000  // 20 ms
#define NUM_RETRIES 2

#define RMP_E_TIMEOUT -2

typedef struct sockaddr_in rmp_address;

/*
 * Parses the given IP address and port number into the rmp_address in *address_dst. 
 * Returns 0 on success and ­-1 on error.
 */
int RMP_getAddressFor(const char *ip, const char *port, rmp_address *address_dst);

/*
 * Returns the port number stored in the given address.
 */
int RMP_getPortFrom(rmp_address *address);

/*
 * Sets up an RMP socket that can send message and be listened on for incoming
 * connections on the given address.  If the given port number is 0 (i.e. is a
 * request for an ephemeral port), then the given address will be updated with
 * the chosen port number.
 * Returns a socket file descriptor or ­-1 on error.
 */
int RMP_createSocket(rmp_address *address);

/*
 * Attempts to sends numBytes bytes of the specified buffer to the given
 * destination over the specified socket.
 * Will send up to 65,502 bytes of data at a time, all remaining bytes will be truncated.
 * Will block and retry sending until the recipient acknowledges the message OR
 * at least the timeout value of 100ms is reached.
 * Returns the number of bytes sent or ­RMP_E_TIMEOUT on acknowledgment timeout
 * or -1 on error.
 */
int RMP_sendTo(int socket_fd, rmp_address *destination,
               const void *buffer, int num_bytes);

/*
 * Blocks for a message to be received on socket_fd and places up to len bytes
 * of it in the given buffer. Also places the sender's address in *src_addr if
 * it is non-null.
 * Returns the number of bytes received or ­-1 on error.
 */
int RMP_listen(int socket_fd, void *buffer, size_t len, rmp_address *src_address);

/*
 * Closes the given socket and cleans up any associated state.
 */
void RMP_closeSocket(int socket_fd);

#endif
