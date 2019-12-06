#!/bin/bash

mnt_file=$1
mnt_path=$2

if [[ ./mnt_mp3_img.sh > 1 ]]; then
  echo "Fail to mount MP3 img"
  exit
fi

cp student-distrib/bootimg "${mnt_path}"
cp student-distrib/filesys_img "${mnt_path}"