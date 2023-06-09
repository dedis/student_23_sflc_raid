#!/bin/sh
#set -x # to echo all commands when they are entered

sflc_path="shufflecake-userland/build/shufflecake"
device_path="/dev/sdb1"
volume_path="/dev/mapper/sflc-"
mount_path="/media/sflc/" # mount folders already exist

sumount() {

busy=true
while $busy
do
 if mountpoint -q "$1"
 then
  sudo umount "$1" 2> /dev/null
  if [ $? -eq 0 ]
  then
   busy=false  # umount successful
  else
   echo -n '.'  # output to show that the script is alive
   sleep 3      # 5 seconds for testing, modify to 300 seconds later on
  fi
 else
  busy=false  # not mounted
 fi
done
}

sudo ./${sflc_path} create_vols --no-randfill ${device_path} pass1 pass2 pass3

sudo ./${sflc_path} open_vols --redundant-within ${device_path} test1 test2 test3 pass3

sudo mkfs.ext4 ${volume_path}test1
sudo mkfs.ext4 ${volume_path}test3

sudo mount ${volume_path}test1 ${mount_path}test1
sudo mount ${volume_path}test3 ${mount_path}test3

sleep 3 # to try to stabilize slices

sumount ${mount_path}test1
sumount ${mount_path}test3

sudo ./${sflc_path} close_vols ${device_path}
