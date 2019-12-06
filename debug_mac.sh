#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

mnt_path="cmake-build-debug/mp3_mnt"

if [ -d "${mnt_path}" ]; then
    rm -rf "${mnt_path}"
fi

mkdir "${mnt_path}"

echo "[1/4] Attaching MP3 img..."
disk=$(hdiutil attach -imagekey diskimage-class=CRawDiskImage -nomount student-distrib/mp3.img)
if [[ $? -ne 0 ]]; then
  echo "Failed"
  exit 1
fi
disk=$(echo $disk | awk '{print $1}')

echo "[2/4] Mounting MP3 img..."
tools/mount_fuse-ext2 -o loop,offset=32256 "${disk}s1" "${mnt_path}"
if [[ $? -ne 0 ]]; then
  echo "Failed"
  exit 1
fi

echo "[3/4] Copying files..."
cp student-distrib/bootimg "${mnt_path}"
cp student-distrib/filesys_img "${mnt_path}"
sync

echo "[4/4] Unmount and detatch..."
umount "${mnt_path}"
hdiutil detach $disk > /dev/null

rm -rf "${mnt_path}"

echo "Done"