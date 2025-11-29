# Encrypted Virtual File System (EVFS)

**CS-352 Operating Systems Course Project**

## Project Overview

This project implements an encrypted virtual file system using FUSE (Filesystem in Userspace). The system automatically encrypts all data before storing it and decrypts it when reading, providing transparent encryption to users.

## Project Structure

```
encrypted_vfs/
├── evfs.h              # Main header file with all declarations
├── main.c              # Entry point and main function
├── evfs_core.c         # Core FUSE operations (Module 1) ✅
├── evfs_metadata.c     # Metadata management (Module 1) ✅
├── evfs_encryption.c   # Encryption/Decryption (Module 2) 🔄
├── evfs_storage.c      # Persistent storage (Module 3) 🔄
├── evfs_readwrite.c    # Read/Write operations (Module 2) 🔄
├── Makefile           # Build configuration
└── mnt/               # Default mount point
```

## Module Status

### ✅ Module 1: Basic FUSE Framework (COMPLETED)
**Implemented by:** [Your Name]  
**Files:** `evfs_core.c`, `evfs_metadata.c`

**Features:**
- ✅ FUSE initialization and cleanup
- ✅ Mount/Unmount operations
- ✅ File metadata management
- ✅ Path resolution and file lookup
- ✅ Directory listing (readdir)
- ✅ File attribute retrieval (getattr)
- ✅ File creation (create)
- ✅ File opening (open)
- ✅ Timestamp updates (utimens)

### 🔄 Module 2: Read/Write with Encryption
**To be implemented by:** [Team Member Name]  
**Files to create:** `evfs_readwrite.c`, `evfs_encryption.c`

**Required Functions:**
```c
// In evfs_readwrite.c
int evfs_read(const char *path, char *buf, size_t size, off_t offset, 
              struct fuse_file_info *fi);
int evfs_write(const char *path, const char *buf, size_t size, off_t offset,
               struct fuse_file_info *fi);

// In evfs_encryption.c
void encrypt_data(const char *plaintext, char *ciphertext, size_t size);
void decrypt_data(const char *ciphertext, char *plaintext, size_t size);
```

### 🔄 Module 3: Persistent Storage
**To be implemented by:** [Team Member Name]  
**File to create:** `evfs_storage.c`

**Required Functions:**
```c
int init_storage(const char *backing_file);
int read_block(int file_idx, off_t offset, char *buf, size_t size);
int write_block(int file_idx, off_t offset, const char *buf, size_t size);
void close_storage(void);
```

### 🔄 Module 4: Advanced Operations
**To be implemented by:** [Team Member Name]  
**Optional features:**
- Directory creation (mkdir)
- File/directory deletion (unlink, rmdir)
- File renaming (rename)
- Symbolic links
- Permission management

## Building the Project

### Prerequisites

```bash
sudo apt-get update
sudo apt-get install libfuse-dev pkg-config build-essential
```

### Compilation

```bash
# Clean previous build
make clean

# Build the project
make

# You should see: "Build successful! Executable: ./evfs"
```

### Running Tests

```bash
# Create mount point and mount filesystem
make run

# Or manually:
mkdir -p mnt
./evfs -f mnt
```

## Testing the Current Implementation

In a **new terminal** (while EVFS is running):

```bash
# Navigate to project directory
cd encrypted_vfs

# Test 1: List empty directory
ls -la mnt/

# Test 2: Create files
touch mnt/test.txt
touch mnt/file1.txt mnt/file2.txt

# Test 3: Verify files exist
ls -la mnt/

# Test 4: Check file attributes
stat mnt/test.txt

# Test 5: View file table
# (Will be shown when you unmount with Ctrl+C)
```

## Unmounting

```bash
# Method 1: Press Ctrl+C in the terminal running ./evfs

# Method 2: From another terminal
fusermount -u mnt

# Method 3: Using Makefile
make unmount
```

## For Team Members: Adding Your Module

### Step 1: Create Your Source File

Create `evfs_yourmodule.c`:

```c
#include "evfs.h"

// Implement your functions here
int your_function(void) {
    printf("[YOUR_MODULE] Function called\n");
    // Your implementation
    return 0;
}
```

### Step 2: Declare Functions in evfs.h

Add your function declarations:

```c
// In evfs.h, add under appropriate section:
int your_function(void);
```

### Step 3: Update Makefile

Add your object file:

```makefile
# In Makefile, update OBJS line:
OBJS = main.o evfs_core.o evfs_metadata.o evfs_yourmodule.o

# Add compilation rule:
evfs_yourmodule.o: evfs_yourmodule.c evfs.h
	$(CC) $(CFLAGS) -c evfs_yourmodule.c
```

### Step 4: Update FUSE Operations (if needed)

If you're adding new FUSE operations, update `evfs_oper` in `evfs_core.c`:

```c
struct fuse_operations evfs_oper = {
    .init       = evfs_init,
    // ... existing operations ...
    .read       = evfs_read,     // Your new operation
    .write      = evfs_write,    // Your new operation
};
```

### Step 5: Rebuild and Test

```bash
make clean
make
make run
```

## Code Style Guidelines

- Use descriptive variable names
- Add comments for complex logic
- Print debug messages with module prefix: `[MODULE_NAME]`
- Handle all error cases
- Return appropriate errno values
- Keep functions focused and modular

## Common Error Codes

```c
-ENOENT    // No such file or directory
-EEXIST    // File already exists
-ENOSPC    // No space left on device
-ENOTDIR   // Not a directory
-EISDIR    // Is a directory
-EACCES    // Permission denied
-EINVAL    // Invalid argument
```

## Debugging Tips

### Enable Verbose Debug Output

```bash
./evfs -f -d mnt
```

### View System Calls

```bash
strace -e trace=file ./evfs -f mnt
```

### Check Mount Status

```bash
mount | grep evfs
mountpoint mnt
```

### Force Unmount if Stuck

```bash
sudo umount -l mnt
# or
fusermount -u mnt
```

## Troubleshooting

### Problem: "Transport endpoint is not connected"
```bash
fusermount -u mnt
./evfs -f mnt
```

### Problem: "Mount point does not exist"
```bash
mkdir -p mnt
./evfs -f mnt
```

### Problem: Compilation errors with FUSE
```bash
pkg-config --cflags --libs fuse
# If empty, reinstall:
sudo apt-get install --reinstall libfuse-dev
```

## Next Steps

1. ✅ **Module 1 Complete** - Basic framework working
2. 🎯 **Next: Module 2** - Implement read/write with in-memory storage
3. 🎯 **Then: Module 3** - Add encryption layer
4. 🎯 **Finally: Module 4** - Add persistent storage

## Resources

- [FUSE Documentation](https://libfuse.github.io/doxygen/)
- [FUSE Tutorial](https://www.cs.nmsu.edu/~pfeiffer/fuse-tutorial/)
- [Linux System Calls](https://man7.org/linux/man-pages/man2/syscalls.2.html)

## Team Communication

- Keep your module branch updated
- Document your changes in comments
- Test thoroughly before merging
- Update this README with your progress

---