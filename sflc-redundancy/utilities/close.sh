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

sumount ${mount_path}test1
sumount ${mount_path}test3

sudo ./${sflc_path} close_vols ${device_path}
