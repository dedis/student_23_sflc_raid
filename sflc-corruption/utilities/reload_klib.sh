#!/bin/sh
set -x # to echo all commands when they are entered

klib_path="../dm-sflc"

./close.sh

sudo rmmod dm-sflc.ko

make -C ${klib_path}

sudo insmod ${klib_path}/bin/dm-sflc.ko
