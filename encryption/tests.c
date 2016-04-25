#include <stdio.h>
#include <string.h>
#include "encryption.h"

void assert(int should_be_true, char *message)
{
	if(!should_be_true) {
		printf("Failure: %s\n", message);
	}
}

void test_edge_cases()
{
	char buffer[128];
	int num_bytes = encrypt("", 0, "asdf", buffer, 128);
	assert(num_bytes == 0, "encrypt empty message");

	num_bytes = decrypt("", 0, "asdf", buffer, 128);
	assert(num_bytes == 0, "decrypt empty message");

	num_bytes = encrypt("foo", 3, "asdf", buffer, 2);
	assert(num_bytes == -1, "encrypt with insufficient buffer");

	num_bytes = decrypt("foo", 3, "asdf", buffer, 2);
	assert(num_bytes == -1, "decrypt with insufficient buffer");
}

void test_roundtrip()
{
	const char plaintext[] = "The quick brown fox jumped over the lazy fox.";
	char ciphertext[128];
	int num_bytes = encrypt(plaintext, sizeof(plaintext), "asdf", ciphertext, 128);
	assert(num_bytes == sizeof(plaintext), "encrypt returned the wrong number of bytes");

	char decrypted_plaintext[128];
	num_bytes = decrypt(ciphertext, num_bytes, "asdf", decrypted_plaintext, 128);
	assert(num_bytes == sizeof(plaintext), "decrypt returned the wrong number of bytes");
	decrypted_plaintext[num_bytes] = 0;
	assert(strncmp(plaintext, decrypted_plaintext, sizeof(plaintext)) == 0, "decryption failed");
	printf("ciphertext: %s\n", ciphertext);
}

int main()
{
	test_edge_cases();
	test_roundtrip();
}
