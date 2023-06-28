#!/bin/sh
set -x # to echo all commands when they are entered

sflc_path="shufflecake-userland/build/shufflecake"
device_path="/dev/sdb1"
volume_path="/dev/mapper/sflc-"
mount_path="/media/sflc/" # mount folders already exist

sudo ./${sflc_path} open_vols ${device_path} test1 test2 test3 pass2

sudo rm ${mount_path}test1/*
sudo rm ${mount_path}test2/*
sudo rm ${mount_path}test3/*

sudo rm ${mount_path}test1/*
sudo rm ${mount_path}test2/*
sudo rm ${mount_path}test3/*

sudo umount ${mount_path}test1
sudo umount ${mount_path}test2
sudo umount ${mount_path}test3

sudo ./${sflc_path} close_vols ${device_path}
