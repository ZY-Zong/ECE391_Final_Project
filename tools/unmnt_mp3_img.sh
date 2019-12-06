#!/bin/bash

mnt_path=$1

mnt_info=$(mount | grep "${mnt_path}")
if [[ -n "${mnt_info}" ]]; then
  disk=$(echo $mnt_info | awk '{print $1}')
  echo "Detaching ${disk}..."
  umount "${mnt_path}"
  hdiutil detach $disk
  rm -rf "${mnt_path}"
else
  echo "MP3 img has not yet mounted."
  exit 1
fi