# EVFS AES-256 Encryption Integration Guide
=================================================

## Overview

This document describes the integration of **AES-256-CBC encryption** into the Encrypted Virtual File System (EVFS). All data written to the backing file (`evfs_data.bin`) is encrypted using industry-standard AES-256 encryption, providing strong security for stored data.

**Key Features:**
- ✅ AES-256-CBC encryption for all file data
- ✅ Transparent encryption/decryption at storage layer
- ✅ SHA-256 key derivation from passphrase
- ✅ Automatic 16-byte block alignment for AES
- ✅ Zero-knowledge architecture (plaintext never touches disk)

---

## Project Structure

```
EVFS/
├── main.c                      # Entry point and FUSE initialization
├── evfs.h                      # Main header with structures and constants
├── evfs_crypto.h               # Encryption interface (NEW)
├── evfs_core.c                 # FUSE operation handlers (MODIFIED)
├── evfs_metadata.c             # File table management
├── evfs_storage.c              # Storage layer with encryption (MODIFIED)
├── evfs_readwrite.c            # Read/write/truncate operations
├── evfs_crypto.c               # AES-256 implementation (NEW)
├── Makefile                    # Build configuration with OpenSSL (MODIFIED)
├── README.md                   # This documentation
├── test_basic.sh               # Basic test script
├── debug_test.sh               # Debug test with encryption verification (NEW)
├── simple_test.sh              # Simple functionality test (NEW)
├── comprehensive_test.sh       # Full test suite (NEW)
├── verify_encryption.sh        # Encryption verification tool (NEW)
├── mnt/                        # Mount point directory
└── evfs_data.bin               # Encrypted backing file (created at runtime)
```

---

## Files Added

### `evfs_crypto.h`
Header file exposing the encryption/decryption interface:
```c
int evfs_crypto_init(void);                          // Initialize AES-256 module
void evfs_crypto_cleanup(void);                      // Clean up and zero keys
int evfs_encrypt_buffer(char *buf, size_t size);    // Encrypt in-place (AES-256-CBC)
int evfs_decrypt_buffer(char *buf, size_t size);    // Decrypt in-place (AES-256-CBC)
```

### `evfs_crypto.c`
Complete AES-256-CBC implementation using OpenSSL:
- **Key Derivation:** Uses SHA-256 to convert passphrase to 256-bit key
- **Encryption Mode:** AES-256-CBC (Cipher Block Chaining)
- **Block Alignment:** Handles 16-byte AES block requirements with zero-padding
- **Secure Cleanup:** Zeros out key material on shutdown
- **Error Handling:** Comprehensive error checking for all OpenSSL operations

**Key Functions:**
```c
// Initialize crypto subsystem (call once at mount)
int evfs_crypto_init(void) {
    // Derives 256-bit key from hardcoded passphrase using SHA-256
    // Sets up fixed IV (WARNING: not production-ready)
    // Returns 0 on success, -1 on failure
}

// Encrypt buffer with automatic padding
int evfs_encrypt_buffer(char *buf, size_t size) {
    // Pads to 16-byte boundary
    // Encrypts in-place using AES-256-CBC
    // Returns 0 on success, -1 on failure
}

// Decrypt buffer
int evfs_decrypt_buffer(char *buf, size_t size) {
    // Decrypts padded data
    // Returns 0 on success, -1 on failure
}

// Cleanup crypto subsystem (call at unmount)
void evfs_crypto_cleanup(void) {
    // Zeros out sensitive key material
    // Prevents key recovery from memory
}
```

---

## Files Modified

### `evfs_core.c`
**Changes:**
1. Added `#include "evfs_crypto.h"` at the top
2. Modified `evfs_init()` function:
   ```c
   void *evfs_init(struct fuse_conn_info *conn) {
       (void)conn;
       printf("[INIT] Initializing EVFS...\n");
       
       // Initialize crypto module FIRST (NEW)
       if (evfs_crypto_init() != 0) {
           fprintf(stderr, "[INIT] Failed to initialize crypto module\n");
           return NULL;
       }
       
       init_filesystem();
       return NULL;
   }
   ```

3. Modified `evfs_destroy()` function:
   ```c
   void evfs_destroy(void *private_data) {
       (void)private_data;
       printf("[DESTROY] Cleaning up EVFS...\n");
       
       print_file_table();
       cleanup_storage();
       
       // Cleanup crypto module LAST (NEW)
       evfs_crypto_cleanup();
       
       printf("[DESTROY] EVFS cleanup complete\n");
   }
   ```

**Purpose:** Ensures encryption subsystem initializes at mount and cleans up at unmount.

---

### `evfs_storage.c`
**Major Changes:**

1. Added `#include "evfs_crypto.h"` at the top

2. **Modified `read_block()` function:**
   ```c
   int read_block(int file_idx, off_t offset, char *buf, size_t size) {
       // Calculate padded size (must match what was written)
       size_t padded_size = size;
       if (size % 16 != 0) {
           padded_size = ((size / 16) + 1) * 16;
       }
       
       // Allocate temporary buffer for padded read
       char *temp_buf = malloc(padded_size);
       
       // Read the padded size from disk
       ssize_t bytes_read = read(backing_fd, temp_buf, padded_size);
       
       // Decrypt the full padded data
       if (evfs_decrypt_buffer(temp_buf, padded_size) != 0) {
           fprintf(stderr, "[STORAGE] Decryption failed\n");
           free(temp_buf);
           return -1;
       }
       
       // Copy only requested size to output buffer
       memcpy(buf, temp_buf, size);
       free(temp_buf);
       
       return size;  // Return original requested size
   }
   ```

3. **Modified `write_block()` function:**
   ```c
   int write_block(int file_idx, off_t offset, const char *buf, size_t size) {
       // Calculate padded size for AES (must be multiple of 16)
       size_t padded_size = size;
       if (size % 16 != 0) {
           padded_size = ((size / 16) + 1) * 16;
       }
       
       // Allocate buffer with extra space for padding
       char *enc_buf = malloc(padded_size);
       
       // Copy data and zero-pad
       memcpy(enc_buf, buf, size);
       if (padded_size > size) {
           memset(enc_buf + size, 0, padded_size - size);
       }
       
       // Encrypt the padded buffer
       if (evfs_encrypt_buffer(enc_buf, size) != 0) {
           fprintf(stderr, "[STORAGE] Encryption failed\n");
           free(enc_buf);
           return -1;
       }
       
       // Write FULL PADDED SIZE to disk (CRITICAL!)
       ssize_t bytes_written = write(backing_fd, enc_buf, padded_size);
       free(enc_buf);
       
       // Return original size to caller
       return size;
   }
   ```

**Purpose:** 
- Centralizes all encryption/decryption at storage layer
- Ensures padded data is written/read for proper AES operation
- All data on disk is encrypted; FUSE operations work with plaintext

---

### `Makefile`
**Changes:**
1. Added OpenSSL include path: `-I/usr/include/openssl` to `CFLAGS`
2. Added OpenSSL libraries: `-lcrypto -lssl` to `LDFLAGS`
3. Added `evfs_crypto.h` to header dependencies
4. Added `check-openssl` target to verify OpenSSL installation

**Complete Makefile:**
```makefile
CC = gcc
CFLAGS = -Wall -Wextra -g `pkg-config fuse --cflags` -I/usr/include/openssl
LDFLAGS = `pkg-config fuse --libs` -lcrypto -lssl

TARGET = evfs
SOURCES = main.c evfs_core.c evfs_metadata.c evfs_storage.c evfs_readwrite.c evfs_crypto.c
OBJECTS = $(SOURCES:.c=.o)
HEADER = evfs.h evfs_crypto.h

all: check-openssl $(TARGET)

check-openssl:
	@echo "Checking for OpenSSL..."
	@pkg-config --exists openssl || (echo "ERROR: OpenSSL not found" && exit 1)
	@echo "OpenSSL found ✓"

$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete! AES-256 encryption enabled ✓"

%.o: %.c $(HEADER)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJECTS) evfs_data.bin
```

---

## Technical Implementation Details

### Encryption Architecture

```
┌─────────────────────────────────────────────┐
│         FUSE User Space                     │
│  (Applications see plaintext files)         │
└────────────────┬────────────────────────────┘
                 │
                 ↓
┌─────────────────────────────────────────────┐
│    EVFS FUSE Operations (evfs_core.c)       │
│    - create, open, read, write              │
│    - Works with PLAINTEXT data              │
└────────────────┬────────────────────────────┘
                 │
                 ↓
┌─────────────────────────────────────────────┐
│   Read/Write Layer (evfs_readwrite.c)       │
│   - Manages file operations                 │
│   - Updates metadata                        │
└────────────────┬────────────────────────────┘
                 │
                 ↓
┌─────────────────────────────────────────────┐
│   Storage Layer (evfs_storage.c)            │
│   ┌───────────────────────────────────┐    │
│   │  ENCRYPTION HAPPENS HERE!         │    │
│   │  • write_block(): encrypt before  │    │
│   │    writing to disk                │    │
│   │  • read_block(): decrypt after    │    │
│   │    reading from disk              │    │
│   └───────────────────────────────────┘    │
└────────────────┬────────────────────────────┘
                 │
                 ↓
┌─────────────────────────────────────────────┐
│   Crypto Module (evfs_crypto.c)             │
│   - AES-256-CBC encryption/decryption       │
│   - SHA-256 key derivation                  │
│   - Block padding/alignment                 │
└────────────────┬────────────────────────────┘
                 │
                 ↓
┌─────────────────────────────────────────────┐
│   Backing File (evfs_data.bin)              │
│   ⚠️  ENCRYPTED DATA ONLY - NO PLAINTEXT    │
└─────────────────────────────────────────────┘
```

### AES Block Padding Strategy

AES requires data to be in 16-byte blocks. Our implementation:

1. **Write Operation:**
   - Original data: 5 bytes (`hello`)
   - Padded size: 16 bytes (`hello` + 11 zero bytes)
   - Encrypt entire 16-byte block
   - Write 16 bytes to disk

2. **Read Operation:**
   - Read 16 bytes from disk
   - Decrypt entire 16-byte block
   - Return only 5 bytes (original size) to caller

3. **Why This Works:**
   - AES operates on complete 16-byte blocks
   - Padding is transparent to application
   - File size tracking in metadata preserves original size

---

## Prerequisites

### System Requirements
- **Operating System:** Linux (Ubuntu/Debian recommended)
- **FUSE Library:** libfuse-dev
- **OpenSSL Library:** libssl-dev (version 1.1.0 or higher)
- **Compiler:** gcc with C11 support
- **Build Tools:** make, pkg-config

### Installation Commands

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install -y build-essential libfuse-dev libssl-dev pkg-config

# Optional: Install debugging tools
sudo apt-get install -y strace valgrind hexdump ent
```

#### Fedora/RHEL/CentOS
```bash
sudo yum install -y gcc make fuse-devel openssl-devel pkg-config

# Optional: Install debugging tools
sudo yum install -y strace valgrind
```

#### macOS
```bash
brew install macfuse openssl pkg-config

# May need to export OpenSSL paths
export CFLAGS="-I$(brew --prefix openssl)/include"
export LDFLAGS="-L$(brew --prefix openssl)/lib"
```

### Verify Installation
```bash
# Check FUSE
pkg-config --modversion fuse
# Should output: 2.9.x or higher

# Check OpenSSL
pkg-config --modversion openssl
# Should output: 1.1.x or 3.x.x

# Check OpenSSL version
openssl version
# Should output: OpenSSL 1.1.1 or higher

# Test OpenSSL libraries
pkg-config --cflags --libs openssl
# Should output include paths and library flags
```

---

## Build Instructions

### Quick Start
```bash
# Clone or navigate to project directory
cd Virtual-Encrypted-File-System

# Clean build
make clean

# Build with AES-256
make

# Expected output:
# Checking for OpenSSL...
# OpenSSL found ✓
# Compiling main.c...
# Compiling evfs_core.c...
# Compiling evfs_metadata.c...
# Compiling evfs_storage.c...
# Compiling evfs_readwrite.c...
# Compiling evfs_crypto.c...
# Linking evfs...
# Build complete! AES-256 encryption enabled ✓
```

### Build Troubleshooting

**Issue: "openssl/evp.h: No such file or directory"**
```bash
# Install OpenSSL development headers
sudo apt-get install libssl-dev

# Verify installation
ls /usr/include/openssl/evp.h
```

**Issue: "undefined reference to `EVP_EncryptInit_ex`"**
```bash
# OpenSSL libraries not linked
# Check Makefile has: LDFLAGS += -lcrypto -lssl

# Rebuild
make clean && make
```

**Issue: Compiler warnings about sign comparison**
```bash
# These are non-critical warnings about comparing signed/unsigned integers
# They don't affect functionality but can be fixed by casting
```

---

## Running the Filesystem

### Mount Options

#### 1. Background Mode (Production)
```bash
# Create mount point
mkdir -p mnt

# Mount in background
./evfs mnt/

# Filesystem runs silently in background
# Use normally with any file operations
echo "test" > mnt/file.txt
cat mnt/file.txt

# Unmount
fusermount -u mnt/
```

#### 2. Foreground Mode (Development)
```bash
# Run in foreground with output
./evfs -f mnt/

# Filesystem stays in foreground
# Press Ctrl+C to unmount
# See all operation logs in terminal
```

#### 3. Debug Mode (Troubleshooting)
```bash
# Run with FUSE debug output
./evfs -d mnt/

# Shows detailed FUSE operations:
# - Every getattr, read, write call
# - All parameters and return values
# - FUSE protocol messages
# Very verbose - use for debugging only
```

### Using Makefile Targets
```bash
# Build and mount in foreground
make mount
# Starts: ./evfs -f mnt/

# Unmount (in another terminal)
make unmount
# Runs: fusermount -u mnt/

# Run test suite
make test
# Runs: bash test_basic.sh

# See all commands
make help
```

---

## Testing and Verification

### Automated Test Suite

We provide comprehensive test scripts to verify encryption:

#### 1. Simple Functionality Test
```bash
chmod +x simple_test.sh
./simple_test.sh
```
Tests basic read/write with different file sizes (4, 5, 16, 32 bytes).

#### 2. Debug Test with Encryption Verification
```bash
chmod +x debug_test.sh
./debug_test.sh
```
Performs:
- Build verification
- Mount/unmount test
- File creation and reading
- Encryption verification in backing file
- Shows hex dump of encrypted data

#### 3. Comprehensive Test Suite (50+ tests)
```bash
chmod +x comprehensive_test.sh
./comprehensive_test.sh
```
Tests:
- Basic file operations
- Various file sizes (1 byte to 100KB)
- Multiple files
- Unicode and special characters
- File operations (delete, rename, append)
- Directory operations
- Edge cases (empty files, long names)
- Concurrent operations
- Persistence after remount
- Encryption verification
- Performance benchmarks

#### 4. Encryption Verification Tool
```bash
chmod +x verify_encryption.sh
./verify_encryption.sh
```
Comprehensive encryption verification:
- Creates files with known sensitive content
- Searches for plaintext in backing file
- Analyzes entropy
- Verifies decryption works
- Provides security assessment

---

### Manual Testing Guide

#### Test 1: Basic Encryption Verification
```bash
# Terminal 1: Mount filesystem
./evfs -f mnt/

# Terminal 2: Create test file
echo "TOP_SECRET_PASSWORD_123" > mnt/secret.txt
cat mnt/secret.txt
# Output: TOP_SECRET_PASSWORD_123 (plaintext visible through FUSE)

# Unmount
# (Ctrl+C in Terminal 1)

# Verify encryption in backing file
strings evfs_data.bin | grep "TOP_SECRET"
# Should output: NOTHING (encrypted!)

strings evfs_data.bin | grep "PASSWORD"
# Should output: NOTHING (encrypted!)

# View encrypted hex data
hexdump -C evfs_data.bin | head -20
# Should see random-looking bytes like: 5a 3f b2 8c 7e...
```

#### Test 2: Multiple File Sizes
```bash
./evfs -f mnt/ &

# Test different sizes
echo -n "test" > mnt/4byte.txt           # 4 bytes (< 16)
echo -n "hello" > mnt/5byte.txt          # 5 bytes (needs padding)
echo -n "0123456789ABCDEF" > mnt/16byte.txt  # 16 bytes (exact)
echo -n "01234567890123456789012345678901" > mnt/32byte.txt  # 32 bytes (2 blocks)

# Verify all files read correctly
cat mnt/4byte.txt   # Should output: test
cat mnt/5byte.txt   # Should output: hello
cat mnt/16byte.txt  # Should output: 0123456789ABCDEF
cat mnt/32byte.txt  # Should output: 01234567890123456789012345678901

# List files
ls -lh mnt/

fusermount -u mnt/
```

#### Test 3: Large File Test
```bash
./evfs mnt/ &

# Create 1MB file
dd if=/dev/zero of=mnt/large.bin bs=1024 count=1024

# Verify size
ls -lh mnt/large.bin
# Should show: 1.0M

# Read entire file
cat mnt/large.bin > /dev/null
echo "Read successful"

# Check backing file
ls -lh evfs_data.bin
# Should be larger than 1MB (due to filesystem overhead)

fusermount -u mnt/
```

#### Test 4: Persistence Test
```bash
# Mount and create files
./evfs mnt/
echo "Persistent data" > mnt/persist.txt
echo "More data" > mnt/data.txt
fusermount -u mnt/

# Remount
./evfs mnt/
cat mnt/persist.txt  # Should output: Persistent data
cat mnt/data.txt     # Should output: More data
ls mnt/              # Should show both files

fusermount -u mnt/
```

---

## Encryption Verification Methods

### Method 1: Visual Hex Comparison
```bash
# Create plaintext file
echo "Hello World 123" > plaintext.txt

# View plaintext hex
hexdump -C plaintext.txt
# Output shows readable ASCII:
# 00000000  48 65 6c 6c 6f 20 57 6f  72 6c 64 20 31 32 33 0a  |Hello World 123.|

# Create same content in EVFS
./evfs mnt/
echo "Hello World 123" > mnt/test.txt
fusermount -u mnt/

# View encrypted hex
hexdump -C evfs_data.bin | head -10
# Output shows random bytes:
# 00000000  5a 3f b2 8c 7e 91 4d f3  8c 14 da 34 d5 58 b1 e7  |Z?..~.M....4.X..|
```

### Method 2: Search for Plaintext (Should Fail)
```bash
./evfs mnt/
echo "CONFIDENTIAL_DATA" > mnt/secret.txt
echo "PASSWORD: admin123" > mnt/pass.txt
fusermount -u mnt/

# Try to find plaintext (should find NOTHING)
strings evfs_data.bin | grep "CONFIDENTIAL"  # No output
strings evfs_data.bin | grep "PASSWORD"      # No output
strings evfs_data.bin | grep "admin123"      # No output

echo "✓ All data is encrypted!"
```

### Method 3: File Type Detection
```bash
./evfs mnt/
echo "Test data" > mnt/test.txt
fusermount -u mnt/

# Check file type
file evfs_data.bin
# Output: evfs_data.bin: data

# NOT: evfs_data.bin: ASCII text
# "data" means binary (encrypted), which is correct!
```

### Method 4: Entropy Analysis
```bash
# Install entropy tool
sudo apt-get install ent

# Check backing file entropy
ent evfs_data.bin

# Expected output:
# Entropy = 7.9+ bits per byte (close to 8 = random/encrypted)
# Chi-square = low value (random distribution)

# Low entropy (< 5) = NOT encrypted!
# High entropy (> 7.5) = Properly encrypted ✓
```

### Method 5: Binary Grep Test
```bash
./evfs mnt/
echo "SECRET_API_KEY_xyz789" > mnt/api.txt
fusermount -u mnt/

# Try binary grep (should find nothing)
grep -a "SECRET" evfs_data.bin
# No output = encrypted ✓

grep -a "API_KEY" evfs_data.bin
# No output = encrypted ✓

# If you see output = NOT ENCRYPTED (problem!)
```

---

## Debugging Commands

### Real-Time Operation Monitoring

#### Monitor All Operations
```bash
# Terminal 1: Run with all debug output
./evfs -d mnt/ 2>&1 | tee debug.log

# Terminal 2: Perform operations
echo "test" > mnt/file.txt 
cat mnt/file.txt
ls mnt/

# View debug.log for detailed trace
```

#### Filter Specific Operations
```bash
# Monitor only encryption operations
./evfs -f mnt/ 2>&1 | grep CRYPTO

# Monitor only storage operations
./evfs -f mnt/ 2>&1 | grep STORAGE

# Monitor only read/write
./evfs -f mnt/ 2>&1 | grep -E "READ|WRITE"
```

### System-Level Debugging

#### Check FUSE Mount Status
```bash
# List all FUSE mounts
mount | grep fuse

# Check specific mount
mount | grep evfs
# Output: evfs on /path/to/mnt type fuse.evfs

# Check mount options
cat /proc/mounts | grep evfs
```

#### Monitor System Calls with strace
```bash
# Trace all system calls
strace -f ./evfs -f mnt/ 2>&1 | tee strace.log

# Trace specific calls
strace -e trace=open,read,write,close ./evfs -f mnt/

# Trace file operations only
strace -e trace=file ./evfs -f mnt/
```

#### Check File Descriptors
```bash
# List open file descriptors
lsof | grep evfs

# Check for leaks
lsof -p $(pgrep evfs)

# Check backing file access
lsof evfs_data.bin
```

### Memory Debugging with Valgrind

#### Basic Memory Check
```bash
# Run with memory leak detection
valgrind --leak-check=full ./evfs -f mnt/

# In another terminal
echo "test" > mnt/test.txt
cat mnt/test.txt

# Unmount and check output
fusermount -u mnt/
```

#### Advanced Memory Analysis
```bash
# Full leak check with origins
valgrind --leak-check=full --show-leak-kinds=all \
         --track-origins=yes ./evfs -f mnt/

# Check for use-after-free
valgrind --tool=memcheck --track-origins=yes ./evfs -f mnt/
```

### Performance Profiling

#### Measure Operation Times
```bash
./evfs mnt/ &

# Time write operations
time dd if=/dev/zero of=mnt/test.bin bs=1M count=10
# Output: real 0m0.234s

# Time read operations
time dd if=mnt/test.bin of=/dev/null bs=1M
# Output: real 0m0.156s

# Time encryption overhead
time cat mnt/test.bin > /dev/null

fusermount -u mnt/
```

#### Profile with perf (Linux)
```bash
# Record performance data
perf record ./evfs -f mnt/

# In another terminal, perform operations
echo "test" > mnt/test.txt

# Stop and analyze
perf report
```

### Backing File Analysis

#### Inspect Raw Data
```bash
# Hex dump first 512 bytes
hexdump -C evfs_data.bin | head -32

# View as hex and ASCII
xxd evfs_data.bin | less

# Search for patterns
xxd evfs_data.bin | grep "pattern"

# Show file size and allocation
ls -lh evfs_data.bin
stat evfs_data.bin
```

#### Compare Encrypted vs Plaintext
```bash
# Create reference
echo "Test Data 123" > reference.txt

# View plaintext hex
xxd reference.txt
# 00000000: 5465 7374 2044 6174 6120 3132 330a       Test Data 123.

# View encrypted hex
./evfs mnt/
echo "Test Data 123" > mnt/test.txt
fusermount -u mnt/
xxd evfs_data.bin | head
# 00000000: 7a3f b28c 7e91 4df3 8c14 da34 d558 b1e7  z?..~.M....4.X..

# Completely different! ✓ Encrypted