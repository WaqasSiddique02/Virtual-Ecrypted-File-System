#!/bin/bash

# Simple test to debug encryption issues

echo "=== EVFS Simple Test ==="

# Cleanup
fusermount -u mnt/ 2>/dev/null || true
rm -f evfs_data.bin

# Build
make clean
make

# Create mount point
mkdir -p mnt

# Run in background
./evfs -f mnt/ > test.log 2>&1 &
PID=$!
echo "Started EVFS with PID $PID"

# Wait for mount
sleep 2

# Test 1: Write small file
echo "=== Test 1: Write 'test' (4 bytes) ==="
echo -n "test" > mnt/file1.txt
sleep 1

echo "File size:"
ls -l mnt/file1.txt

echo "Read back:"
cat mnt/file1.txt
echo ""

# Test 2: Write 5 bytes (not multiple of 16)
echo "=== Test 2: Write 'hello' (5 bytes) ==="
echo -n "hello" > mnt/file2.txt
sleep 1

echo "File size:"
ls -l mnt/file2.txt

echo "Read back:"
cat mnt/file2.txt
echo ""

# Test 3: Write 16 bytes (exact multiple)
echo "=== Test 3: Write '0123456789ABCDEF' (16 bytes) ==="
echo -n "0123456789ABCDEF" > mnt/file3.txt
sleep 1

echo "File size:"
ls -l mnt/file3.txt

echo "Read back:"
cat mnt/file3.txt
echo ""

# Test 4: Write 32 bytes (exact multiple)
echo "=== Test 4: Write 32 bytes ==="
echo -n "01234567890123456789012345678901" > mnt/file4.txt
sleep 1

echo "File size:"
ls -l mnt/file4.txt

echo "Read back:"
cat mnt/file4.txt
echo ""

# Unmount
echo "=== Unmounting ==="
fusermount -u mnt/
wait $PID

# Check backing file
echo "=== Backing file check ==="
echo "Size: $(stat -c%s evfs_data.bin) bytes"
echo "Hex dump (first 128 bytes):"
hexdump -C evfs_data.bin | head -8

echo ""
echo "=== Log output (last 50 lines) ==="
tail -50 test.log