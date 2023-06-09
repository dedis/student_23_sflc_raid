#!/bin/sh
set -x # to echo all commands when they are entered

echo "kernel module should be loaded before this script" # sudo insmod bin/dm-sflc.ko

sflc_path="../shufflecake-userland/build/shufflecake"
device_path="/dev/sda1"
volume_path="/dev/mapper/sflc-"
mount_path="/media/sflc/" # mount folders already exist

n_files=10
size_files=1

sudo ./${sflc_path} open_vols ${device_path} test1 pass1

sudo mount ${volume_path}test1 ${mount_path}test1

for i in `seq 1 $n_files`
do
	head -c ${size_files}M < /dev/urandom | sudo tee ${mount_path}test1/$i > /dev/null
done

sudo umount ${mount_path}test1

sudo ./${sflc_path} close_vols ${device_path}
