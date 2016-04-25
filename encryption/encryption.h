#ifndef __ENCRYPTION_H__
#define __ENCRYPTION_H__

/*
 * Encrypts the given plaintext with the given key, placing up to buffer_len
 * bytes of ciphertext into ciphertext_buffer.
 * Returns the number of bytes written to the buffer on success or -1 on error.
 * If the ciphertext cannot entirely fit in the buffer, an error will be returned.
 */
int encrypt(const char *plaintext, size_t plaintext_len, const char *key,
            char *ciphertext_buffer, size_t buffer_len);

/*
 * Decrypts the given ciphertext with the given key, placing up to buffer_len
 * bytes of plaintext into plaintext_buffer.
 * Returns the number of bytes written to the buffer on success or -1 on error.
 * If the plaintext cannot entirely fit in the buffer, an error will be returned.
 */
int decrypt(const char *ciphertext, size_t ciphertext_len, const char *key,
            char *plaintext_buffer, size_t buffer_len);

#endif
