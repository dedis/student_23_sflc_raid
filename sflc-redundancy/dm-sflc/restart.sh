#!/bin/sh
set -x # to echo all commands when they are entered

make clean

sudo rmmod dm_sflc

make

sudo make install

sudo insmod bin/dm-sflc.ko
