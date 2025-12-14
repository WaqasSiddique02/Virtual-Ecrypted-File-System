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
 * STORAGE MODULE FUNCTIONS (implemented in evfs_storage.c)
 * ============================================================================
 */

// Initialize storage system
int init_storage(void);

// Allocate storage for a file
int allocate_storage(int file_idx, size_t size);

// Read data from storage
int read_block(int file_idx, off_t offset, char *buf, size_t size);

// Write data to storage
int write_block(int file_idx, off_t offset, const char *buf, size_t size);

// Delete file storage
int delete_storage(int file_idx);

// Cleanup storage system
void cleanup_storage(void);

/*
 * ============================================================================
 * READ/WRITE OPERATIONS (implemented in evfs_readwrite.c)
 * ============================================================================
 */

// Read from file
int evfs_read(const char *path, char *buf, size_t size, off_t offset,
              struct fuse_file_info *fi);

// Write to file
int evfs_write(const char *path, const char *buf, size_t size, off_t offset,
               struct fuse_file_info *fi);

// Truncate file
int evfs_truncate(const char *path, off_t size);

// Delete file
int evfs_unlink(const char *path);

// Create directory
int evfs_mkdir(const char *path, mode_t mode);

// Remove directory
int evfs_rmdir(const char *path);

// Rename file/directory
int evfs_rename(const char *from, const char *to);

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

#endif // EVFS_H