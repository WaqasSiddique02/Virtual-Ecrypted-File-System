#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo "========================================"
echo "  EVFS Read/Write Operations Tests"
echo "========================================"

test_result() {
    if [ "$1" -eq 0 ]; then
        echo -e "${GREEN}PASS${NC}: $2"
    else
        echo -e "${RED}FAIL${NC}: $2"
        exit 1
    fi
}

cleanup() {
    echo ""
    echo "Cleaning up..."
    fusermount -u mnt 2>/dev/null || umount mnt 2>/dev/null
    sleep 1
    echo "Cleanup complete."
}

trap cleanup EXIT

echo -e "${YELLOW}Building EVFS...${NC}"
make clean && make
test_result $? "Build successful"

mkdir -p mnt

echo -e "${YELLOW}Mounting EVFS...${NC}"
./evfs -f mnt &
EVFS_PID=$!
sleep 2

mountpoint -q mnt || test_result 1 "Mount failed"

echo "Hello, EVFS!" > mnt/test.txt
CONTENT=$(cat mnt/test.txt)
[ "$CONTENT" = "Hello, EVFS!" ] || test_result 1 "Read/write failed"

echo "Second line" >> mnt/test.txt

SIZE=$(stat -c%s mnt/test.txt)
[ "$SIZE" -gt 0 ] || test_result 1 "File size invalid"

dd if=/dev/urandom of=mnt/large.dat bs=1K count=100 status=none
[ "$(stat -c%s mnt/large.dat)" -eq 102400 ] || test_result 1 "Large file invalid"

mkdir mnt/testdir
echo "Dir file" > mnt/testdir/a.txt

rm mnt/test.txt
[ ! -f mnt/test.txt ] || test_result 1 "Delete failed"

mv mnt/large.dat mnt/renamed.dat
[ -f mnt/renamed.dat ] || test_result 1 "Rename failed"

echo "All tests passed"
wait "$EVFS_PID"
