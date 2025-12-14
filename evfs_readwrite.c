#include "evfs.h"

/*
 * ============================================================================
 * READ/WRITE OPERATIONS MODULE
 * ============================================================================
 */

/*
 * Read data from a file
 */
int evfs_read(const char *path, char *buf, size_t size, off_t offset,
              struct fuse_file_info *fi) {
    (void)fi; // Unused parameter
    
    printf("[READ] Called for path: %s (size: %zu, offset: %ld)\n", 
           path, size, offset);
    
    // Find the file
    int idx = find_file_by_path(path);
    if (idx == -1) {
        printf("[READ] File not found: %s\n", path);
        return -ENOENT;
    }
    
    // Check if it's a file
    if (file_table[idx].type != FTYPE_FILE) {
        printf("[READ] Not a file: %s\n", path);
        return -EISDIR;
    }
    
    // Check bounds
    if (offset >= file_table[idx].size) {
        printf("[READ] Offset beyond file size\n");
        return 0; // EOF
    }
    
    // Adjust size if reading past end of file
    if (offset + (off_t)size > file_table[idx].size) {
        size = file_table[idx].size - offset;
        printf("[READ] Adjusted size to %zu to fit file bounds\n", size);
    }
    
    // Read from storage
    int bytes_read = read_block(idx, offset, buf, size);
    if (bytes_read < 0) {
        printf("[READ] Failed to read from storage\n");
        return -EIO;
    }
    
    // Update access time
    file_table[idx].atime = time(NULL);
    
    printf("[READ] Successfully read %d bytes from %s\n", bytes_read, path);
    return bytes_read;
}

/*
 * Write data to a file
 */
int evfs_write(const char *path, const char *buf, size_t size, off_t offset,
               struct fuse_file_info *fi) {
    (void)fi; // Unused parameter
    
    printf("[WRITE] Called for path: %s (size: %zu, offset: %ld)\n", 
           path, size, offset);
    
    // Find the file
    int idx = find_file_by_path(path);
    if (idx == -1) {
        printf("[WRITE] File not found: %s\n", path);
        return -ENOENT;
    }
    
    // Check if it's a file
    if (file_table[idx].type != FTYPE_FILE) {
        printf("[WRITE] Not a file: %s\n", path);
        return -EISDIR;
    }
    
    // Write to storage
    int bytes_written = write_block(idx, offset, buf, size);
    if (bytes_written < 0) {
        printf("[WRITE] Failed to write to storage\n");
        return -EIO;
    }
    
    // Update file size if necessary
    off_t new_size = offset + bytes_written;
    if (new_size > file_table[idx].size) {
        file_table[idx].size = new_size;
        printf("[WRITE] Updated file size to %ld\n", new_size);
    }
    
    // Update modification and change times
    time_t now = time(NULL);
    file_table[idx].mtime = now;
    file_table[idx].ctime = now;
    
    printf("[WRITE] Successfully wrote %d bytes to %s\n", bytes_written, path);
    return bytes_written;
}

/*
 * Truncate a file to a specified size
 */
int evfs_truncate(const char *path, off_t size) {
    printf("[TRUNCATE] Called for path: %s (size: %ld)\n", path, size);
    
    // Find the file
    int idx = find_file_by_path(path);
    if (idx == -1) {
        printf("[TRUNCATE] File not found: %s\n", path);
        return -ENOENT;
    }
    
    // Check if it's a file
    if (file_table[idx].type != FTYPE_FILE) {
        printf("[TRUNCATE] Not a file: %s\n", path);
        return -EISDIR;
    }
    
    // If growing the file, we might need to write zeros
    if (size > file_table[idx].size) {
        printf("[TRUNCATE] Growing file from %ld to %ld\n", 
               file_table[idx].size, size);
        
        // Write zeros to extend the file
        size_t zero_size = size - file_table[idx].size;
        char *zero_buf = calloc(1, zero_size);
        if (zero_buf) {
            write_block(idx, file_table[idx].size, zero_buf, zero_size);
            free(zero_buf);
        }
    }
    
    // Update file size
    file_table[idx].size = size;
    
    // Update modification and change times
    time_t now = time(NULL);
    file_table[idx].mtime = now;
    file_table[idx].ctime = now;
    
    printf("[TRUNCATE] File truncated to %ld bytes\n", size);
    return 0;
}

/*
 * Delete a file (unlink)
 */
int evfs_unlink(const char *path) {
    printf("[UNLINK] Called for path: %s\n", path);
    
    // Find the file
    int idx = find_file_by_path(path);
    if (idx == -1) {
        printf("[UNLINK] File not found: %s\n", path);
        return -ENOENT;
    }
    
    // Check if it's a file
    if (file_table[idx].type != FTYPE_FILE) {
        printf("[UNLINK] Not a file: %s\n", path);
        return -EISDIR;
    }
    
    // Delete storage
    delete_storage(idx);
    
    // Mark entry as unused
    file_table[idx].is_used = 0;
    memset(file_table[idx].name, 0, MAX_FILENAME);
    
    printf("[UNLINK] File deleted successfully: %s\n", path);
    return 0;
}

/*
 * Create a directory
 */
int evfs_mkdir(const char *path, mode_t mode) {
    printf("[MKDIR] Called for path: %s\n", path);
    
    // Check if directory already exists
    if (find_file_by_path(path) != -1) {
        printf("[MKDIR] Directory already exists: %s\n", path);
        return -EEXIST;
    }
    
    // Find free slot
    int idx = find_free_slot();
    if (idx == -1) {
        printf("[MKDIR] No free slots available\n");
        return -ENOSPC;
    }
    
    // Extract directory name from path
    const char *dirname = path + 1; // skip leading '/'
    
    // Create new directory metadata
    strncpy(file_table[idx].name, dirname, MAX_FILENAME - 1);
    file_table[idx].type = FTYPE_DIR;
    file_table[idx].mode = mode | 0755; // Ensure execute permission for directories
    file_table[idx].uid = getuid();
    file_table[idx].gid = getgid();
    file_table[idx].atime = time(NULL);
    file_table[idx].mtime = time(NULL);
    file_table[idx].ctime = time(NULL);
    file_table[idx].size = BLOCK_SIZE; // Standard directory size
    file_table[idx].is_used = 1;
    file_table[idx].parent_idx = 0; // root directory (for now)
    
    printf("[MKDIR] Directory created successfully: %s (index: %d)\n", path, idx);
    return 0;
}

/*
 * Remove a directory
 */
int evfs_rmdir(const char *path) {
    printf("[RMDIR] Called for path: %s\n", path);
    
    // Find the directory
    int idx = find_file_by_path(path);
    if (idx == -1) {
        printf("[RMDIR] Directory not found: %s\n", path);
        return -ENOENT;
    }
    
    // Check if it's a directory
    if (file_table[idx].type != FTYPE_DIR) {
        printf("[RMDIR] Not a directory: %s\n", path);
        return -ENOTDIR;
    }
    
    // Check if directory is empty
    for (int i = 1; i < MAX_FILES; i++) {
        if (file_table[i].is_used && file_table[i].parent_idx == idx) {
            printf("[RMDIR] Directory not empty: %s\n", path);
            return -ENOTEMPTY;
        }
    }
    
    // Mark entry as unused
    file_table[idx].is_used = 0;
    memset(file_table[idx].name, 0, MAX_FILENAME);
    
    printf("[RMDIR] Directory removed successfully: %s\n", path);
    return 0;
}

/*
 * Rename a file or directory
 */
int evfs_rename(const char *from, const char *to) {
    printf("[RENAME] Called: %s -> %s\n", from, to);
    
    // Find source file
    int from_idx = find_file_by_path(from);
    if (from_idx == -1) {
        printf("[RENAME] Source not found: %s\n", from);
        return -ENOENT;
    }
    
    // Check if destination exists
    int to_idx = find_file_by_path(to);
    if (to_idx != -1) {
        printf("[RENAME] Destination already exists: %s\n", to);
        return -EEXIST;
    }
    
    // Extract new name
    const char *new_name = to + 1; // skip leading '/'
    
    // Update name
    strncpy(file_table[from_idx].name, new_name, MAX_FILENAME - 1);
    file_table[from_idx].ctime = time(NULL);
    
    printf("[RENAME] Renamed successfully: %s -> %s\n", from, to);
    return 0;
}