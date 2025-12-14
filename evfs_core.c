#include "evfs.h"
#include "evfs_crypto.h"


/*
 * ============================================================================
 * FUSE OPERATIONS IMPLEMENTATION
 * ============================================================================
 */

// Get file attributes
int evfs_getattr(const char *path, struct stat *stbuf) {
    printf("[GETATTR] Called for path: %s\n", path);
    
    memset(stbuf, 0, sizeof(struct stat));
    
    int idx = find_file_by_path(path);
    if (idx == -1) {
        printf("[GETATTR] Path not found: %s\n", path);
        return -ENOENT;
    }
    
    file_metadata_t *meta = &file_table[idx];
    
    if (meta->type == FTYPE_DIR) {
        stbuf->st_mode = S_IFDIR | meta->mode;
        stbuf->st_nlink = 2;
    } else {
        stbuf->st_mode = S_IFREG | meta->mode;
        stbuf->st_nlink = 1;
        stbuf->st_size = meta->size;
    }
    
    stbuf->st_uid = meta->uid;
    stbuf->st_gid = meta->gid;
    stbuf->st_atime = meta->atime;
    stbuf->st_mtime = meta->mtime;
    stbuf->st_ctime = meta->ctime;
    
    printf("[GETATTR] Success for: %s (type: %s, size: %ld)\n", 
           path, meta->type == FTYPE_DIR ? "DIR" : "FILE", meta->size);
    
    return 0;
}

// Read directory contents
int evfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                 off_t offset, struct fuse_file_info *fi) {
    (void)offset;  // Mark as intentionally unused
    (void)fi;      // Mark as intentionally unused
    
    printf("[READDIR] Called for path: %s\n", path);
    
    int dir_idx = find_file_by_path(path);
    if (dir_idx == -1) {
        printf("[READDIR] Directory not found: %s\n", path);
        return -ENOENT;
    }
    
    if (file_table[dir_idx].type != FTYPE_DIR) {
        printf("[READDIR] Not a directory: %s\n", path);
        return -ENOTDIR;
    }
    
    // Add . and ..
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    
    // Add all files in this directory
    for (int i = 1; i < MAX_FILES; i++) {
        if (file_table[i].is_used && file_table[i].parent_idx == dir_idx) {
            printf("[READDIR] Adding entry: %s\n", file_table[i].name);
            filler(buf, file_table[i].name, NULL, 0);
        }
    }
    
    printf("[READDIR] Success for: %s\n", path);
    return 0;
}

// Create a new file
int evfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void)fi;  // Mark as intentionally unused
    
    printf("[CREATE] Called for path: %s\n", path);
    
    // Check if file already exists
    if (find_file_by_path(path) != -1) {
        printf("[CREATE] File already exists: %s\n", path);
        return -EEXIST;
    }
    
    // Find free slot
    int idx = find_free_slot();
    if (idx == -1) {
        printf("[CREATE] No free slots available\n");
        return -ENOSPC;
    }
    
    // Extract filename from path (assuming single-level for now)
    const char *filename = path + 1; // skip leading '/'
    
    // Create new file metadata
    strncpy(file_table[idx].name, filename, MAX_FILENAME - 1);
    file_table[idx].type = FTYPE_FILE;
    file_table[idx].mode = mode;
    file_table[idx].uid = getuid();
    file_table[idx].gid = getgid();
    file_table[idx].atime = time(NULL);
    file_table[idx].mtime = time(NULL);
    file_table[idx].ctime = time(NULL);
    file_table[idx].size = 0;
    file_table[idx].is_used = 1;
    file_table[idx].parent_idx = 0; // root directory
    
    printf("[CREATE] File created successfully: %s (index: %d)\n", path, idx);
    return 0;
}

// Open a file
int evfs_open(const char *path, struct fuse_file_info *fi) {
    (void)fi;  // Mark as intentionally unused
    
    printf("[OPEN] Called for path: %s\n", path);
    
    int idx = find_file_by_path(path);
    if (idx == -1) {
        printf("[OPEN] File not found: %s\n", path);
        return -ENOENT;
    }
    
    if (file_table[idx].type != FTYPE_FILE) {
        printf("[OPEN] Not a file: %s\n", path);
        return -EISDIR;
    }
    
    printf("[OPEN] Success for: %s\n", path);
    return 0;
}

// Initialize filesystem
void *evfs_init(struct fuse_conn_info *conn) {
    (void)conn;
    
    printf("[INIT] Initializing EVFS...\n");
    
    // Initialize crypto module FIRST
    if (evfs_crypto_init() != 0) {
        fprintf(stderr, "[INIT] Failed to initialize crypto module\n");
        return NULL;
    }
    
    init_filesystem();
    return NULL;
}
// Destroy filesystem
void evfs_destroy(void *private_data) {
    (void)private_data;
    
    printf("[DESTROY] Cleaning up EVFS...\n");
    
    // Print final file table state
    print_file_table();
    
    // Cleanup storage
    cleanup_storage();
    
    // Cleanup crypto module LAST
    evfs_crypto_cleanup();
    
    printf("[DESTROY] EVFS cleanup complete\n");
}
// Set file timestamps
int evfs_utimens(const char *path, const struct timespec ts[2]) {
    printf("[UTIMENS] Called for path: %s\n", path);
    
    int idx = find_file_by_path(path);
    if (idx == -1) {
        printf("[UTIMENS] File not found: %s\n", path);
        return -ENOENT;
    }
    
    // Update access time and modification time
    if (ts != NULL) {
        file_table[idx].atime = ts[0].tv_sec;
        file_table[idx].mtime = ts[1].tv_sec;
    } else {
        // If ts is NULL, set to current time
        time_t now = time(NULL);
        file_table[idx].atime = now;
        file_table[idx].mtime = now;
    }
    
    printf("[UTIMENS] Updated timestamps for: %s\n", path);
    return 0;
}

/*
 * ============================================================================
 * FUSE OPERATIONS STRUCTURE
 * ============================================================================
 */

struct fuse_operations evfs_oper = {
    .init       = evfs_init,
    .destroy    = evfs_destroy,
    .getattr    = evfs_getattr,
    .readdir    = evfs_readdir,
    .create     = evfs_create,
    .open       = evfs_open,
    .read       = evfs_read,
    .write      = evfs_write,
    .truncate   = evfs_truncate,
    .unlink     = evfs_unlink,
    .mkdir      = evfs_mkdir,
    .rmdir      = evfs_rmdir,
    .rename     = evfs_rename,
    .utimens    = evfs_utimens,
};