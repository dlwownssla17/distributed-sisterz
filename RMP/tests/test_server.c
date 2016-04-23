#include <stdio.h>
#include <string.h>
#include "test_constants.h"
#include "../rmp.h"

void assert(int should_be_true, char *message)
{
	if(!should_be_true) {
		printf("Server test error: %s\n", message);
	}
}

int main()
{
	// Get socket address
	rmp_address my_address;
	int status = RMP_getAddressFor(LISTENER_IP, LISTENER_PORT, &my_address);
	assert(status == 0, "RMP_getAddressFor status wasn't 0");

	// Create socket
	int my_socket = RMP_createSocket(&my_address);
	assert(my_socket != -1, "RMP_createSocket failed");

	// Receive first message
	int buffer_size = 128;
	char buffer[buffer_size];
	rmp_address src_address;
	int num_bytes_received;
	while((num_bytes_received = RMP_listen(my_socket, buffer, buffer_size, &src_address)) == 0);
	assert(num_bytes_received == sizeof(MESSAGE), "RMP_listen didn't return the expected number of bytes");
	assert(strncmp(buffer, MESSAGE, sizeof(MESSAGE)) == 0, "RMP_listen didn't get the right message.");

	// Send first message response
	int bytes_sent = RMP_sendTo(my_socket, &src_address, MESSAGE2, sizeof(MESSAGE2));
	assert(bytes_sent == sizeof(MESSAGE2), "RMP_sendTo sent the wrong number of bytes");

	// Receive second message
	while((num_bytes_received = RMP_listen(my_socket, buffer, buffer_size, &src_address)) == 0);
	assert(num_bytes_received == sizeof(MESSAGE3), "RMP_listen didn't return the expected number of bytes for MESSAGE3");
	assert(strncmp(buffer, MESSAGE3, sizeof(MESSAGE3)) == 0, "RMP_listen didn't get the right message for MESSAGE3.");

	// Send second message response
	bytes_sent = RMP_sendTo(my_socket, &src_address, MESSAGE4, sizeof(MESSAGE4));
	assert(bytes_sent == sizeof(MESSAGE4), "RMP_sendTo sent the wrong number of bytes for MESSAGE4");

	// Close socket
	RMP_closeSocket(my_socket);

	printf("Server exited\n");
}
