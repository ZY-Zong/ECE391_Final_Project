#!/bin/bash

mnt_file=$1
mnt_path=$2

mnt_info=$(mount | grep "${mnt_path}")
if [[ -n "${mnt_info}" ]]; then
  disk=$(echo $mnt_info | awk '{print $1}')
  echo "MP3 img has already attached to ${disk} and mounted to ${mnt_path}"
  exit 1
else
  echo "Mounting MP3 img to ${mnt_path}..."

  if [[ -d "${mnt_path}" ]]; then
    rm -rf "${mnt_path}"
  fi
  mkdir "${mnt_path}"
  
  disk=$(hdiutil attach -imagekey diskimage-class=CRawDiskImage -nomount "${mnt_file}")
  disk=$(echo $disk | awk '{print $1}')

  if [[ -n "${disk}" ]]; then
    echo "Attach image to ${disk}"
    mount -t fuse-ext2 -o loop,offset=32256 "${disk}s1" "${mnt_path}"
    if [[ $? -eq 0 ]]; then
      echo "Success"
      exit 0
    else
      echo "Failed to mount"
      exit 3
    fi
  else
    echo "Failed to attach image using hdiutil"
    exit 2
  fi
fi