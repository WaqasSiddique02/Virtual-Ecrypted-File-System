#!/bin/bash

# Comprehensive EVFS AES Encryption Test Suite

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

PASS=0
FAIL=0

# Test result tracking
test_result() {
    if [ $1 -eq 0 ]; then
        echo -e "${GREEN}✓ PASS${NC}: $2"
        ((PASS++))
    else
        echo -e "${RED}✗ FAIL${NC}: $2"
        ((FAIL++))
    fi
}

# Cleanup function
cleanup() {
    echo -e "\n${YELLOW}Cleaning up...${NC}"
    fusermount -u mnt/ 2>/dev/null || true
    sleep 1
}

trap cleanup EXIT

echo "========================================"
echo "  EVFS AES-256 Comprehensive Test Suite"
echo "========================================"

# Build
echo -e "\n${BLUE}Building EVFS...${NC}"
make clean > /dev/null 2>&1
rm -f evfs_data.bin
make > /dev/null 2>&1
test_result $? "Build system"

# Create mount point
mkdir -p mnt

# Mount
echo -e "\n${BLUE}Mounting filesystem...${NC}"
./evfs mnt/ > /dev/null 2>&1 &
EVFS_PID=$!
sleep 2

if kill -0 $EVFS_PID 2>/dev/null; then
    test_result 0 "Mount filesystem"
else
    test_result 1 "Mount filesystem"
    exit 1
fi

echo -e "\n${BLUE}=== Test 1: Basic File Operations ===${NC}"

# Test 1.1: Create and write file
echo "Hello EVFS with AES-256!" > mnt/test1.txt
test_result $? "Create and write file"

# Test 1.2: Read file back
CONTENT=$(cat mnt/test1.txt 2>/dev/null)
if [ "$CONTENT" = "Hello EVFS with AES-256!" ]; then
    test_result 0 "Read file correctly"
else
    test_result 1 "Read file correctly (got: '$CONTENT')"
fi

# Test 1.3: File size
SIZE=$(stat -c%s mnt/test1.txt)
if [ "$SIZE" -eq 26 ]; then
    test_result 0 "File size correct ($SIZE bytes)"
else
    test_result 1 "File size incorrect (expected 26, got $SIZE)"
fi

echo -e "\n${BLUE}=== Test 2: Various File Sizes ===${NC}"

# Test 2.1: Small file (4 bytes - less than AES block)
echo -n "test" > mnt/small.txt
SMALL=$(cat mnt/small.txt)
[ "$SMALL" = "test" ]
test_result $? "Small file (4 bytes)"

# Test 2.2: 5 bytes (not multiple of 16)
echo -n "hello" > mnt/five.txt
FIVE=$(cat mnt/five.txt)
[ "$FIVE" = "hello" ]
test_result $? "5-byte file (padding required)"

# Test 2.3: Exactly 16 bytes (one AES block)
echo -n "0123456789ABCDEF" > mnt/block16.txt
BLOCK16=$(cat mnt/block16.txt)
[ "$BLOCK16" = "0123456789ABCDEF" ]
test_result $? "16-byte file (exact AES block)"

# Test 2.4: 32 bytes (two AES blocks)
echo -n "01234567890123456789012345678901" > mnt/block32.txt
BLOCK32=$(cat mnt/block32.txt)
[ "$BLOCK32" = "01234567890123456789012345678901" ]
test_result $? "32-byte file (two AES blocks)"

# Test 2.5: 17 bytes (requires padding)
echo -n "12345678901234567" > mnt/odd17.txt
ODD17=$(cat mnt/odd17.txt)
[ "$ODD17" = "12345678901234567" ]
test_result $? "17-byte file (requires padding)"

echo -e "\n${BLUE}=== Test 3: Multiple Files ===${NC}"

# Test 3.1: Create multiple files
for i in {1..5}; do
    echo "File number $i" > mnt/multi_$i.txt
done
test_result $? "Create 5 files"

# Test 3.2: Read all files
ALL_OK=true
for i in {1..5}; do
    CONTENT=$(cat mnt/multi_$i.txt)
    if [ "$CONTENT" != "File number $i" ]; then
        ALL_OK=false
    fi
done
$ALL_OK
test_result $? "Read all 5 files correctly"

# Test 3.3: List files
FILE_COUNT=$(ls mnt/*.txt 2>/dev/null | wc -l)
[ $FILE_COUNT -ge 10 ]
test_result $? "File listing (found $FILE_COUNT files)"

echo -e "\n${BLUE}=== Test 4: Special Characters ===${NC}"

# Test 4.1: Unicode characters
echo "Hello 世界 🔒" > mnt/unicode.txt
UNICODE=$(cat mnt/unicode.txt)
[ "$UNICODE" = "Hello 世界 🔒" ]
test_result $? "Unicode characters"

# Test 4.2: Newlines and special chars
echo -e "Line1\nLine2\nLine3" > mnt/newlines.txt
LINE_COUNT=$(cat mnt/newlines.txt | wc -l)
[ $LINE_COUNT -eq 3 ]
test_result $? "Newlines in file"

# Test 4.3: Binary-like content
echo -e "\x00\x01\x02\x03\xFF" > mnt/binary.txt
test_result $? "Binary content"

echo -e "\n${BLUE}=== Test 5: File Operations ===${NC}"

# Test 5.1: Overwrite file
echo "Original" > mnt/overwrite.txt
echo "Modified" > mnt/overwrite.txt
MODIFIED=$(cat mnt/overwrite.txt)
[ "$MODIFIED" = "Modified" ]
test_result $? "Overwrite file"

# Test 5.2: Append (truncate then write)
echo "First" > mnt/append.txt
echo "Second" >> mnt/append.txt
LINES=$(cat mnt/append.txt | wc -l)
[ $LINES -eq 2 ]
test_result $? "Append to file"

# Test 5.3: Delete file
echo "Delete me" > mnt/delete.txt
rm mnt/delete.txt
[ ! -f mnt/delete.txt ]
test_result $? "Delete file"

echo -e "\n${BLUE}=== Test 6: Large File ===${NC}"

# Test 6.1: Write 1KB file
dd if=/dev/urandom of=mnt/large1k.bin bs=1024 count=1 2>/dev/null
SIZE=$(stat -c%s mnt/large1k.bin)
[ $SIZE -eq 1024 ]
test_result $? "Write 1KB file"

# Test 6.2: Write 10KB file
dd if=/dev/zero of=mnt/large10k.bin bs=1024 count=10 2>/dev/null
SIZE=$(stat -c%s mnt/large10k.bin)
[ $SIZE -eq 10240 ]
test_result $? "Write 10KB file"

# Test 6.3: Read large file
cat mnt/large10k.bin > /dev/null 2>&1
test_result $? "Read large file"

echo -e "\n${BLUE}=== Test 7: Directory Operations ===${NC}"

# Test 7.1: Create directory
mkdir mnt/testdir 2>/dev/null
test_result $? "Create directory"

# Test 7.2: Remove empty directory
rmdir mnt/testdir 2>/dev/null
test_result $? "Remove empty directory"

echo -e "\n${BLUE}=== Test 8: Rename Operations ===${NC}"

# Test 8.1: Rename file
echo "Rename test" > mnt/oldname.txt
mv mnt/oldname.txt mnt/newname.txt 2>/dev/null
[ -f mnt/newname.txt ] && [ ! -f mnt/oldname.txt ]
test_result $? "Rename file"

# Test 8.2: Read renamed file
RENAMED=$(cat mnt/newname.txt)
[ "$RENAMED" = "Rename test" ]
test_result $? "Read renamed file"

echo -e "\n${BLUE}=== Test 9: Edge Cases ===${NC}"

# Test 9.1: Empty file
touch mnt/empty.txt
SIZE=$(stat -c%s mnt/empty.txt)
[ $SIZE -eq 0 ]
test_result $? "Empty file"

# Test 9.2: Single character
echo -n "X" > mnt/single.txt
SINGLE=$(cat mnt/single.txt)
[ "$SINGLE" = "X" ]
test_result $? "Single character file"

# Test 9.3: Long filename
LONG_NAME="this_is_a_very_long_filename_to_test_limits.txt"
echo "Long name" > mnt/$LONG_NAME
[ -f mnt/$LONG_NAME ]
test_result $? "Long filename"

echo -e "\n${BLUE}=== Test 10: Concurrent Operations ===${NC}"

# Test 10.1: Multiple simultaneous writes
for i in {1..3}; do
    echo "Concurrent $i" > mnt/concurrent_$i.txt &
done
wait
ALL_OK=true
for i in {1..3}; do
    if [ ! -f mnt/concurrent_$i.txt ]; then
        ALL_OK=false
    fi
done
$ALL_OK
test_result $? "Concurrent writes"

echo -e "\n${BLUE}=== Test 11: Unmount and Remount ===${NC}"

# Unmount
fusermount -u mnt/
wait $EVFS_PID 2>/dev/null || true
sleep 1

test_result $? "Unmount filesystem"

# Verify files exist on disk
[ -f evfs_data.bin ]
test_result $? "Backing file exists"

# Remount
./evfs mnt/ > /dev/null 2>&1 &
EVFS_PID=$!
sleep 2

if kill -0 $EVFS_PID 2>/dev/null; then
    test_result 0 "Remount filesystem"
else
    test_result 1 "Remount filesystem"
    exit 1
fi

# Check if files still exist
[ -f mnt/test1.txt ]
test_result $? "Files persist after remount"

# Read file after remount
CONTENT=$(cat mnt/test1.txt 2>/dev/null)
[ "$CONTENT" = "Hello EVFS with AES-256!" ]
test_result $? "Read file after remount"

echo -e "\n${BLUE}=== Test 12: Encryption Verification ===${NC}"

# Unmount for backing file inspection
fusermount -u mnt/
wait $EVFS_PID 2>/dev/null || true
sleep 1

# Test 12.1: No plaintext in backing file
if strings evfs_data.bin | grep -q "Hello EVFS"; then
    test_result 1 "Plaintext not found in backing file (SECURITY ISSUE!)"
else
    test_result 0 "Plaintext not found in backing file"
fi

# Test 12.2: No test strings in backing file
if strings evfs_data.bin | grep -q "test"; then
    test_result 1 "Test string not in backing file"
else
    test_result 0 "Test string encrypted"
fi

# Test 12.3: Backing file is not empty
SIZE=$(stat -c%s evfs_data.bin)
if [ $SIZE -gt 0 ]; then
    test_result 0 "Backing file contains data ($SIZE bytes)"
else
    test_result 1 "Backing file is empty"
fi

echo -e "\n${BLUE}=== Test 13: Performance Test ===${NC}"

# Remount for performance test
./evfs mnt/ > /dev/null 2>&1 &
EVFS_PID=$!
sleep 2

# Test 13.1: Write speed
START=$(date +%s%N)
dd if=/dev/zero of=mnt/perf.bin bs=1024 count=100 2>/dev/null
END=$(date +%s%N)
DURATION=$(( (END - START) / 1000000 ))
echo "  Write speed: 100KB in ${DURATION}ms"
[ $DURATION -lt 5000 ]
test_result $? "Write performance (< 5s for 100KB)"

# Test 13.2: Read speed
START=$(date +%s%N)
cat mnt/perf.bin > /dev/null
END=$(date +%s%N)
DURATION=$(( (END - START) / 1000000 ))
echo "  Read speed: 100KB in ${DURATION}ms"
[ $DURATION -lt 5000 ]
test_result $? "Read performance (< 5s for 100KB)"

# Final cleanup
fusermount -u mnt/
wait $EVFS_PID 2>/dev/null || true

# Summary
echo -e "\n========================================"
echo -e "${GREEN}Tests Passed: $PASS${NC}"
echo -e "${RED}Tests Failed: $FAIL${NC}"
echo "========================================"

if [ $FAIL -eq 0 ]; then
    echo -e "${GREEN}All tests passed! ✓${NC}"
    echo -e "\n${BLUE}Your EVFS with AES-256 encryption is working correctly!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
fi