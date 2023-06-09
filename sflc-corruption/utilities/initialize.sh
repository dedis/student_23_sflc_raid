#!/bin/sh
set -x # to echo all commands when they are entered

klib_path="../dm-sflc"
sflc_path="../shufflecake-userland/build/shufflecake"
device_path="/dev/sda1"
volume_path="/dev/mapper/sflc-"
mount_path="/media/sflc/" # mount folders already exist

sudo insmod ${klib_path}/bin/dm-sflc.ko

sudo ./${sflc_path} create_vols --no-randfill ${device_path} pass1 pass2

sudo ./${sflc_path} open_vols ${device_path} test1 test2 pass2

sudo mkfs.ext4 ${volume_path}test1
sudo mkfs.ext4 ${volume_path}test2

sudo mount ${volume_path}test1 ${mount_path}test1
sudo mount ${volume_path}test2 ${mount_path}test2

sudo umount ${mount_path}test1
sudo umount ${mount_path}test2

sudo ./${sflc_path} close_vols ${device_path}
