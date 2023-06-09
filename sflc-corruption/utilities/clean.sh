#!/bin/sh
set -x # to echo all commands when they are entered

echo "kernel module should be loaded before this script" # sudo insmod bin/dm-sflc.ko

sflc_path="../shufflecake-userland/build/shufflecake"
device_path="/dev/sda1"
volume_path="/dev/mapper/sflc-"
mount_path="/media/sflc/" # mount folders already exist

sudo ./${sflc_path} open_vols ${device_path} test1 test2 pass2

sudo mount ${volume_path}test1 ${mount_path}test1
sudo mount ${volume_path}test2 ${mount_path}test2

sudo rm ${mount_path}test1/*
sudo rm ${mount_path}test2/*

sudo rm checksums.txt

sudo umount ${mount_path}test1
sudo umount ${mount_path}test2

sudo ./${sflc_path} close_vols ${device_path}
