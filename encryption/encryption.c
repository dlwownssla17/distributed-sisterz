#include <string.h>
#include "encryption.h"

int encrypt(const char *plaintext, size_t plaintext_len, const char *key,
            char *ciphertext_buffer, size_t buffer_len)
{
	const size_t key_len = strlen(key);
	int i;
	for(i = 0; i < plaintext_len; i++) {
		if(i == buffer_len) {
			return -1;
		}

		ciphertext_buffer[i] = plaintext[i] + key[i % key_len];
	}

	return i;
}

int decrypt(const char *ciphertext, size_t ciphertext_len, const char *key,
            char *plaintext_buffer, size_t buffer_len)
{
	const size_t key_len = strlen(key);
	int i;
	for(i = 0; i < ciphertext_len; i++) {
		if(i == buffer_len) {
			return -1;
		}

		plaintext_buffer[i] = ciphertext[i] - key[i % key_len];
	}

	return i;
}
