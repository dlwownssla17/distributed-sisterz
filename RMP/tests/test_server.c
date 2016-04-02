#include <stdio.h>
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
	rmp_address my_address;
	int status = RMP_getAddressFor(LISTENER_IP, LISTENER_PORT, &my_address);
	assert(status == 0, "RMP_getAddressFor status wasn't 0");

	int my_socket = RMP_createSocket(&my_address);
	assert(my_socket != -1, "RMP_createSocket failed");

	int buffer_size = 128;
	char buffer[buffer_size];
	rmp_address src_address;
	int num_bytes_received = RMP_listen(my_socket, buffer, buffer_size, &src_address);
	assert(num_bytes_received == sizeof(MESSAGE), "RMP_listen didn't return the expected number of bytes");

	int bytes_sent = RMP_sendTo(my_socket, &src_address, MESSAGE2, sizeof(MESSAGE2));
	assert(bytes_sent = sizeof(MESSAGE2), "RMP_sendTo sent too few bytes");

	RMP_closeSocket(my_socket);

	printf("Server exited\n");
}
