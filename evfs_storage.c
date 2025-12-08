#include "evfs.h"

/*
 * ============================================================================
 * STORAGE MODULE - Manages persistent storage in backing file
 * ============================================================================
 */

#define BACKING_FILE "evfs_data.bin"
#define MAX_FILE_SIZE (10 * 1024 * 1024) // 10MB per file max

// Storage metadata for each file
typedef struct {
    off_t storage_offset;  // Where this file's data starts in backing file
    size_t allocated_size; // How much space is allocated
} storage_info_t;

static storage_info_t storage_table[MAX_FILES];
static int backing_fd = -1;
static off_t next_free_offset = 0;

/*
 * Initialize the storage system
 */
int init_storage(void) {
    printf("[STORAGE] Initializing storage system...\n");
    
    // Open or create backing file
    backing_fd = open(BACKING_FILE, O_RDWR | O_CREAT, 0666);
    if (backing_fd < 0) {
        perror("[STORAGE] Failed to open backing file");
        return -1;
    }
    
    // Get file size
    struct stat st;
    if (fstat(backing_fd, &st) < 0) {
        perror("[STORAGE] Failed to stat backing file");
        close(backing_fd);
        return -1;
    }
    
    // If file is empty, initialize it
    if (st.st_size == 0) {
        printf("[STORAGE] Creating new backing file\n");
        next_free_offset = 0;
    } else {
        printf("[STORAGE] Using existing backing file (size: %ld bytes)\n", st.st_size);
        next_free_offset = st.st_size;
    }
    
    // Initialize storage table
    for (int i = 0; i < MAX_FILES; i++) {
        storage_table[i].storage_offset = -1;
        storage_table[i].allocated_size = 0;
    }
    
    printf("[STORAGE] Storage system initialized successfully\n");
    return 0;
}

/*
 * Allocate storage space for a file
 */
int allocate_storage(int file_idx, size_t size) {
    if (file_idx < 0 || file_idx >= MAX_FILES) {
        printf("[STORAGE] Invalid file index: %d\n", file_idx);
        return -1;
    }
    
    if (size > MAX_FILE_SIZE) {
        printf("[STORAGE] Requested size too large: %zu\n", size);
        return -EFBIG;
    }
    
    // Round up to block size
    size_t alloc_size = ((size + BLOCK_SIZE - 1) / BLOCK_SIZE) * BLOCK_SIZE;
    if (alloc_size == 0) alloc_size = BLOCK_SIZE;
    
    printf("[STORAGE] Allocating %zu bytes for file %d at offset %ld\n", 
           alloc_size, file_idx, next_free_offset);
    
    storage_table[file_idx].storage_offset = next_free_offset;
    storage_table[file_idx].allocated_size = alloc_size;
    next_free_offset += alloc_size;
    
    return 0;
}

/*
 * Read data from storage
 */
int read_block(int file_idx, off_t offset, char *buf, size_t size) {
    if (file_idx < 0 || file_idx >= MAX_FILES) {
        printf("[STORAGE] Invalid file index: %d\n", file_idx);
        return -1;
    }
    
    if (storage_table[file_idx].storage_offset < 0) {
        printf("[STORAGE] No storage allocated for file %d\n", file_idx);
        // Return zeros for unallocated storage
        memset(buf, 0, size);
        return size;
    }
    
    off_t read_offset = storage_table[file_idx].storage_offset + offset;
    
    printf("[STORAGE] Reading %zu bytes from file %d at offset %ld (physical: %ld)\n",
           size, file_idx, offset, read_offset);
    
    // Seek to position
    if (lseek(backing_fd, read_offset, SEEK_SET) < 0) {
        perror("[STORAGE] Failed to seek");
        return -1;
    }
    
    // Read data
    ssize_t bytes_read = read(backing_fd, buf, size);
    if (bytes_read < 0) {
        perror("[STORAGE] Failed to read");
        return -1;
    }
    
    printf("[STORAGE] Successfully read %zd bytes\n", bytes_read);
    return bytes_read;
}

/*
 * Write data to storage
 */
int write_block(int file_idx, off_t offset, const char *buf, size_t size) {
    if (file_idx < 0 || file_idx >= MAX_FILES) {
        printf("[STORAGE] Invalid file index: %d\n", file_idx);
        return -1;
    }
    
    // Check if we need to allocate or expand storage
    if (storage_table[file_idx].storage_offset < 0) {
        // First write - allocate storage
        if (allocate_storage(file_idx, offset + size) < 0) {
            return -1;
        }
    } else if (offset + size > (off_t)storage_table[file_idx].allocated_size) {
        // Need more space - reallocate
        printf("[STORAGE] Need to expand storage for file %d\n", file_idx);
        
        size_t new_size = offset + size;
        off_t old_offset = storage_table[file_idx].storage_offset;
        size_t old_size = storage_table[file_idx].allocated_size;
        
        // Allocate new space
        if (allocate_storage(file_idx, new_size) < 0) {
            return -1;
        }
        
        // Copy existing data to new location
        if (old_size > 0) {
            char *temp_buf = malloc(old_size);
            if (temp_buf) {
                lseek(backing_fd, old_offset, SEEK_SET);
                ssize_t rd = read(backing_fd, temp_buf, old_size);
                if (rd > 0) {
                    lseek(backing_fd, storage_table[file_idx].storage_offset, SEEK_SET);
                    write(backing_fd, temp_buf, rd);
                }
                free(temp_buf);
            }
        }
    }
    
    off_t write_offset = storage_table[file_idx].storage_offset + offset;
    
    printf("[STORAGE] Writing %zu bytes to file %d at offset %ld (physical: %ld)\n",
           size, file_idx, offset, write_offset);
    
    // Seek to position
    if (lseek(backing_fd, write_offset, SEEK_SET) < 0) {
        perror("[STORAGE] Failed to seek");
        return -1;
    }
    
    // Write data
    ssize_t bytes_written = write(backing_fd, buf, size);
    if (bytes_written < 0) {
        perror("[STORAGE] Failed to write");
        return -1;
    }
    
    // Sync to disk
    fsync(backing_fd);
    
    printf("[STORAGE] Successfully wrote %zd bytes\n", bytes_written);
    return bytes_written;
}

/*
 * Delete file storage
 */
int delete_storage(int file_idx) {
    if (file_idx < 0 || file_idx >= MAX_FILES) {
        return -1;
    }
    
    printf("[STORAGE] Freeing storage for file %d\n", file_idx);
    
    // Mark storage as free
    storage_table[file_idx].storage_offset = -1;
    storage_table[file_idx].allocated_size = 0;
    
    return 0;
}

/*
 * Cleanup storage system
 */
void cleanup_storage(void) {
    printf("[STORAGE] Cleaning up storage system...\n");
    
    if (backing_fd >= 0) {
        fsync(backing_fd);
        close(backing_fd);
        backing_fd = -1;
    }
    
    printf("[STORAGE] Storage system cleaned up\n");
}