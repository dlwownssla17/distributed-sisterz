#include <stdio.h>
#include "../rmp.h"

void assert(int should_be_true, char *message)
{
	if(!should_be_true) {
		printf("Local test error: %s\n", message);
	}
}

void test_getPortFrom_fixed()
{
	rmp_address address;
	RMP_getAddressFor("127.0.0.1", "52000", &address);
	int socket_fd = RMP_createSocket(&address);

	int result = RMP_getPortFrom(&address);
	assert(result == 52000, "RMP_getPortFrom should return a correct fixed port");

	RMP_closeSocket(socket_fd);
}

void test_getPortFrom_ephemeral()
{
	rmp_address address;
	RMP_getAddressFor("127.0.0.1", "0", &address);
	int socket_fd = RMP_createSocket(&address);

	int result = RMP_getPortFrom(&address);
	assert(result != 0, "RMP_getPortFrom should return a correct ephemeral port");

	RMP_closeSocket(socket_fd);
}

int main()
{
	test_getPortFrom_fixed();
	test_getPortFrom_ephemeral();
}
