#include "evfs.h"

int main(int argc, char *argv[]) {
    printf("==============================================\n");
    printf("  Encrypted Virtual File System (EVFS)\n");
    printf("  CS-352 Operating Systems Course Project\n");
    printf("==============================================\n");
    printf("  Module 1: Basic FUSE Framework\n");
    printf("  - Mounting and unmounting\n");
    printf("  - File system operations\n");
    printf("  - Metadata management\n");
    printf("==============================================\n");
    
    if (argc < 2) {
        fprintf(stderr, "\nUsage: %s <mountpoint> [options]\n\n", argv[0]);
        fprintf(stderr, "Options:\n");
        fprintf(stderr, "  -f        Run in foreground (see debug output)\n");
        fprintf(stderr, "  -d        Run in debug mode (very verbose)\n");
        fprintf(stderr, "  -s        Run single-threaded\n");
        fprintf(stderr, "\nExample:\n");
        fprintf(stderr, "  %s mnt -f    # Mount on 'mnt' directory in foreground\n\n", argv[0]);
        return 1;
    }
    
    printf("\nMount point: %s\n", argv[1]);
    printf("Starting FUSE filesystem...\n\n");
    
    // Start FUSE with our operations
    int ret = fuse_main(argc, argv, &evfs_oper, NULL);
    
    printf("\n==============================================\n");
    printf("  EVFS Unmounted\n");
    printf("==============================================\n");
    
    return ret;
}