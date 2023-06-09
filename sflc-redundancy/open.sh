#!/bin/sh
#set -x # to echo all commands when they are entered

sflc_path="shufflecake-userland/build/shufflecake"
device_path="/dev/sdb1"
volume_path="/dev/mapper/sflc-"
mount_path="/media/sflc/" # mount folders already exist

sudo ./${sflc_path} open_vols --redundant-within ${device_path} test1 test2 test3 pass3

sudo mount ${volume_path}test1 ${mount_path}test1
sudo mount ${volume_path}test3 ${mount_path}test3
