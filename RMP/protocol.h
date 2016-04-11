#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

enum message_type { DATA, ACK, SYNACK };
typedef long int message_id;

/*
 * Sends the given payload with the given metadata over the given socket to the
 * given address.
 * Returns the number of bytes sent or ­-1 on error.
 */
int send_rmp_datagram(int socket_fd, struct sockaddr_in *destination,
                      enum message_type type, message_id id,
               		  const void *payload, int num_bytes);

/*
 * Receives a message on the given socket.  Places its sender address in
 * src_address, the metadata in type and id, and up to len bytes of payload in
 * payload.
 * Returns the number of bytes received or ­-1 on error.
 */
int receive_rmp_datagram(int socket_fd, struct sockaddr_in *src_address,
                         enum message_type *type, message_id *id,
                         void *payload, size_t len);
#endif
