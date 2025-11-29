#ifndef EVFS_H
#define EVFS_H

#define FUSE_USE_VERSION 31
#define _POSIX_C_SOURCE 200809L

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

/*
 * ============================================================================
 * CONSTANTS AND CONFIGURATION
 * ============================================================================
 */

#define MAX_FILENAME 256
#define MAX_FILES 100
#define BLOCK_SIZE 4096

// File types
typedef enum {
    FTYPE_DIR,
    FTYPE_FILE
} file_type_t;

// Metadata for each file/directory
typedef struct {
    char name[MAX_FILENAME];
    file_type_t type;
    mode_t mode;
    uid_t uid;
    gid_t gid;
    time_t atime;  // access time
    time_t mtime;  // modification time
    time_t ctime;  // change time
    off_t size;
    int is_used;   // 1 if this entry is valid, 0 if free
    int parent_idx; // index of parent directory (-1 for root)
} file_metadata_t;

/*
 * ============================================================================
 * GLOBAL VARIABLES (declared as extern, defined in evfs_metadata.c)
 * ============================================================================
 */

extern file_metadata_t file_table[MAX_FILES];
extern int initialized;

/*
 * ============================================================================
 * METADATA MANAGEMENT FUNCTIONS (implemented in evfs_metadata.c)
 * ============================================================================
 */

// Initialize the file system
void init_filesystem(void);

// Find file by path, returns index or -1 if not found
int find_file_by_path(const char *path);

// Find an empty slot in file_table, returns index or -1 if no space
int find_free_slot(void);

// Print file table for debugging
void print_file_table(void);

/*
 * ============================================================================
 * FUSE OPERATIONS (implemented in evfs_core.c)
 * ============================================================================
 */

// Get file attributes
int evfs_getattr(const char *path, struct stat *stbuf);

// Read directory contents
int evfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                 off_t offset, struct fuse_file_info *fi);

// Create a new file
int evfs_create(const char *path, mode_t mode, struct fuse_file_info *fi);

// Open a file
int evfs_open(const char *path, struct fuse_file_info *fi);

// Initialize filesystem
void *evfs_init(struct fuse_conn_info *conn);

// Destroy filesystem
void evfs_destroy(void *private_data);

// Set file timestamps
int evfs_utimens(const char *path, const struct timespec ts[2]);

/*
 * ============================================================================
 * FUSE OPERATIONS STRUCTURE
 * ============================================================================
 */

extern struct fuse_operations evfs_oper;

/*
 * ============================================================================
 * ENCRYPTION MODULE (to be implemented by team member)
 * ============================================================================
 */

// These functions will be implemented in evfs_encryption.c
// void encrypt_data(const char *plaintext, char *ciphertext, size_t size);
// void decrypt_data(const char *ciphertext, char *plaintext, size_t size);

/*
 * ============================================================================
 * STORAGE MODULE (to be implemented by team member)
 * ============================================================================
 */

// These functions will be implemented in evfs_storage.c
// int read_block(int file_idx, off_t offset, char *buf, size_t size);
// int write_block(int file_idx, off_t offset, const char *buf, size_t size);
// int init_storage(const char *backing_file);

#endif // EVFS_H