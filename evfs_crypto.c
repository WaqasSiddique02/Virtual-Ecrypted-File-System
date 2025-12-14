#include "evfs_crypto.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <string.h>
#include <stdio.h>

// AES-256 requires 32-byte key
static unsigned char aes_key[32];
// AES block size is 16 bytes
static unsigned char aes_iv[16];

int evfs_crypto_init(void) {
    printf("[CRYPTO] Initializing AES-256 encryption...\n");
    
    // WARNING: This uses a hardcoded key for demonstration
    // In production, use proper key derivation (PBKDF2) or secure key storage
    const char *passphrase = "evfs_secure_passphrase_2025";
    
    // Derive key from passphrase using SHA-256
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        fprintf(stderr, "[CRYPTO] Failed to create MD context\n");
        return -1;
    }
    
    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL) != 1) {
        EVP_MD_CTX_free(mdctx);
        return -1;
    }
    
    if (EVP_DigestUpdate(mdctx, passphrase, strlen(passphrase)) != 1) {
        EVP_MD_CTX_free(mdctx);
        return -1;
    }
    
    unsigned int len;
    if (EVP_DigestFinal_ex(mdctx, aes_key, &len) != 1) {
        EVP_MD_CTX_free(mdctx);
        return -1;
    }
    EVP_MD_CTX_free(mdctx);
    
    // Use a fixed IV for simplicity (NOT recommended for production)
    // In production, generate random IV per encryption and store it
    memset(aes_iv, 0x42, 16);
    
    printf("[CRYPTO] AES-256 initialized successfully\n");
    return 0;
}

void evfs_crypto_cleanup(void) {
    printf("[CRYPTO] Cleaning up crypto module...\n");
    
    // Zero out sensitive key material
    memset(aes_key, 0, sizeof(aes_key));
    memset(aes_iv, 0, sizeof(aes_iv));
    
    printf("[CRYPTO] Crypto cleanup complete\n");
}

int evfs_encrypt_buffer(char *buf, size_t size) {
    if (!buf || size == 0) return -1;
    
    // For sizes not multiple of 16, we only encrypt up to the last full block
    // This is acceptable since BLOCK_SIZE should be a multiple of 16
    size_t encrypt_size = (size / 16) * 16;
    
    // If size is not a multiple of 16, round up and pad
    if (size % 16 != 0) {
        encrypt_size = size; // We'll pad it
        size_t remainder = size % 16;
        // Pad with zeros (assumes buffer has been allocated with enough space)
        memset(buf + size, 0, 16 - remainder);
        encrypt_size = size + (16 - remainder);
    }
    
    if (encrypt_size == 0) {
        return 0; // Nothing to encrypt
    }
    
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        fprintf(stderr, "[CRYPTO] Failed to create cipher context\n");
        return -1;
    }
    
    // Initialize encryption with AES-256-CBC
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, aes_key, aes_iv) != 1) {
        fprintf(stderr, "[CRYPTO] Encryption init failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    
    // Disable padding - we handle it manually
    EVP_CIPHER_CTX_set_padding(ctx, 0);
    
    int len;
    
    // Encrypt the data in-place
    if (EVP_EncryptUpdate(ctx, (unsigned char*)buf, &len, 
                          (unsigned char*)buf, encrypt_size) != 1) {
        fprintf(stderr, "[CRYPTO] Encryption update failed (size: %zu)\n", encrypt_size);
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    
    int final_len;
    // Finalize encryption
    if (EVP_EncryptFinal_ex(ctx, (unsigned char*)buf + len, &final_len) != 1) {
        fprintf(stderr, "[CRYPTO] Encryption final failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    
    EVP_CIPHER_CTX_free(ctx);
    printf("[CRYPTO] Encrypted %zu bytes (padded to %zu)\n", size, encrypt_size);
    return 0;
}

int evfs_decrypt_buffer(char *buf, size_t size) {
    if (!buf || size == 0) return -1;
    
    // Calculate actual decrypt size (must be multiple of 16)
    size_t decrypt_size = (size / 16) * 16;
    
    if (decrypt_size == 0) {
        return 0; // Nothing to decrypt
    }
    
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        fprintf(stderr, "[CRYPTO] Failed to create cipher context\n");
        return -1;
    }
    
    // Initialize decryption with AES-256-CBC
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, aes_key, aes_iv) != 1) {
        fprintf(stderr, "[CRYPTO] Decryption init failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    
    // Disable padding
    EVP_CIPHER_CTX_set_padding(ctx, 0);
    
    int len;
    
    // Decrypt the data in-place
    if (EVP_DecryptUpdate(ctx, (unsigned char*)buf, &len, 
                          (unsigned char*)buf, decrypt_size) != 1) {
        fprintf(stderr, "[CRYPTO] Decryption update failed (size: %zu)\n", decrypt_size);
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    
    int final_len;
    // Finalize decryption
    if (EVP_DecryptFinal_ex(ctx, (unsigned char*)buf + len, &final_len) != 1) {
        fprintf(stderr, "[CRYPTO] Decryption final failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    
    EVP_CIPHER_CTX_free(ctx);
    printf("[CRYPTO] Decrypted %zu bytes\n", decrypt_size);
    return 0;
}
