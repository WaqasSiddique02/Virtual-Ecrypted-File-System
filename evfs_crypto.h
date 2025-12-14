#ifndef EVFS_CRYPTO_H
#define EVFS_CRYPTO_H

#include <stddef.h>

// Initialize the crypto module (call once at startup)
int evfs_crypto_init(void);

// Clean up crypto resources (call at shutdown)
void evfs_crypto_cleanup(void);

// Encrypt buffer in-place using AES-256-CBC
// Returns 0 on success, -1 on error
int evfs_encrypt_buffer(char *buf, size_t size);

// Decrypt buffer in-place using AES-256-CBC
// Returns 0 on success, -1 on error
int evfs_decrypt_buffer(char *buf, size_t size);

#endif // EVFS_CRYPTO_H