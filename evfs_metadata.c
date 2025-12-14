#include "evfs.h"

// Global file table
file_metadata_t file_table[MAX_FILES];
int initialized = 0;
// Initialize the file system
void init_filesystem(void) {
    if (initialized) {
        printf("[METADATA] Filesystem already initialized\n");
        return;
    }
    
    printf("[METADATA] Initializing filesystem metadata...\n");
    
    memset(file_table, 0, sizeof(file_table));
    
    // Create root directory (/)
    strcpy(file_table[0].name, "/");
    file_table[0].type = FTYPE_DIR;
    file_table[0].mode = 0755;
    file_table[0].uid = getuid();
    file_table[0].gid = getgid();
    file_table[0].atime = time(NULL);
    file_table[0].mtime = time(NULL);
    file_table[0].ctime = time(NULL);
    file_table[0].size = BLOCK_SIZE;
    file_table[0].is_used = 1;
    file_table[0].parent_idx = -1;
    
    // Initialize storage system
    if (init_storage() < 0) {
        fprintf(stderr, "[METADATA] Failed to initialize storage\n");
        return;
    }
    
    initialized = 1;
    printf("[INIT] File system initialized with root directory\n");
    print_file_table();
}

// Find file by path
int find_file_by_path(const char *path) {
    if (strcmp(path, "/") == 0) {
        return 0; // root directory
    }
    
    // Search for the file in file_table
    for (int i = 1; i < MAX_FILES; i++) {
        if (file_table[i].is_used) {
            // Build full path for this file
            char full_path[MAX_FILENAME * 2];
            strcpy(full_path, "/");
            strcat(full_path, file_table[i].name);
            
            if (strcmp(full_path, path) == 0) {
                return i;
            }
        }
    }
    
    return -1; // not found
}

// Find an empty slot in file_table
int find_free_slot(void) {
    for (int i = 1; i < MAX_FILES; i++) {
        if (!file_table[i].is_used) {
            return i;
        }
    }
    return -1;
}

// Print file table for debugging
void print_file_table(void) {
    printf("\n========== FILE TABLE ==========\n");
    printf("IDX | USED | TYPE | NAME\n");
    printf("--------------------------------\n");
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].is_used) {
            printf("%3d | %4d | %4s | %s\n", 
                   i, 
                   file_table[i].is_used,
                   file_table[i].type == FTYPE_DIR ? "DIR" : "FILE",
                   file_table[i].name);
        }
    }
    printf("================================\n\n");
}