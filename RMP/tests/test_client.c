#include <stdio.h>
#include <string.h>
#include "test_constants.h"
#include "../rmp.h"

void assert(int should_be_true, char *message)
{
	if(!should_be_true) {
		printf("Client test error: %s\n", message);
	}
}

int main()
{
	// Get socket address
	rmp_address my_address;
	int status = RMP_getAddressFor(CLIENT_IP, CLIENT_PORT, &my_address);
	assert(status == 0, "RMP_getAddressFor status wasn't 0");

	// Create socket
	int my_socket = RMP_createSocket(&my_address);
	assert(my_socket != -1, "RMP_createSocket failed");

	// Get recipient address
	rmp_address listener_address;
	status = RMP_getAddressFor(LISTENER_IP, LISTENER_PORT, &listener_address);
	assert(status == 0, "RMP_getAddressFor status wasn't 0");

	// Send first message
	int bytes_sent = RMP_sendTo(my_socket, &listener_address, MESSAGE, sizeof(MESSAGE));
	assert(bytes_sent == sizeof(MESSAGE), "RMP_sendTo sent the wrong number of bytes for MESSAGE");

	// Receive first message response
	int buffer_size = 128;
	char buffer[buffer_size];
	int num_bytes_received;
	while((num_bytes_received = RMP_listen(my_socket, buffer, buffer_size, NULL)) == 0);
	assert(num_bytes_received == sizeof(MESSAGE2), "RMP_listen didn't return the expected number of bytes for MESSAGE2");
	assert(strncmp(buffer, MESSAGE2, sizeof(MESSAGE2)) == 0, "RMP_listen didn't get the right message for MESSAGE2.");

	// Send second message
	bytes_sent = RMP_sendTo(my_socket, &listener_address, MESSAGE3, sizeof(MESSAGE3));
	assert(bytes_sent == sizeof(MESSAGE3), "RMP_sendTo sent the wrong number of bytes for MESSAGE3");

	// Receive second message response
	while((num_bytes_received = RMP_listen(my_socket, buffer, buffer_size, NULL)) == 0);
	assert(num_bytes_received == sizeof(MESSAGE4), "RMP_listen didn't return the expected number of bytes for MESSAGE4");
	assert(strncmp(buffer, MESSAGE4, sizeof(MESSAGE4)) == 0, "RMP_listen didn't get the right message for MESSAGE4.");

	// Send third message, expect no acknowledgment
	bytes_sent = RMP_sendTo(my_socket, &listener_address, MESSAGE, sizeof(MESSAGE));
	assert(bytes_sent == -2, "RMP_sendTo didn't return properly on ack timeout");

	// Close socket
	RMP_closeSocket(my_socket);

	printf("Client exited\n");
}
