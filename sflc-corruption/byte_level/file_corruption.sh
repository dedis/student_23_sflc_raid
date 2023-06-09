#!/bin/sh
set -x # to echo all commands when they are entered

echo "kernel module should be loaded before this script" # sudo insmod bin/dm-sflc.ko

sflc_path="../shufflecake-userland/build/shufflecake"
device_path="/dev/sda1"
volume_path="/dev/mapper/sflc-"
mount_path="/media/sflc/" # mount folders already exist

test_files="test_files"

n_decoy_files=20
size_decoy_files=4

n_rounds=10

# remove results file if it exists

if [ -f "results.txt" ] ; then
    rm results.txt
fi

if [ -f "fsck.txt" ] ; then
    rm fsck.txt
fi

# safe umount function (waits until device not busy anymore)

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
   sleep 5      # 5 seconds for testing, modify to 300 seconds later on
  fi
 else
  busy=false  # not mounted
 fi
done
}

# initialize

sudo ./${sflc_path} create_vols --no-randfill ${device_path} pass1 pass2

sudo ./${sflc_path} open_vols ${device_path} test1 test2 pass2

sudo mkfs.ext4 ${volume_path}test1
sudo mkfs.ext4 ${volume_path}test2

# prepare hidden files

sudo mount ${volume_path}test2 ${mount_path}test2

sudo cp -r ${test_files}/. ${mount_path}test2

sumount ${mount_path}test2

sudo ./${sflc_path} close_vols ${device_path}

# do rounds

for i in `seq 1 $n_rounds`
do
	echo "round $i"
	# fill decoy

	sudo ./${sflc_path} open_vols ${device_path} test1 pass1

	sudo mount ${volume_path}test1 ${mount_path}test1

	for j in `seq 1 $n_decoy_files`
	do
		filename=$((n_rounds*(i-1)+j))
		head -c ${size_decoy_files}M < /dev/urandom | sudo tee ${mount_path}test1/$filename > /dev/null
	done

	sumount ${mount_path}test1

	sudo ./${sflc_path} close_vols ${device_path}

	# check filesystem hidden

	sudo ./${sflc_path} open_vols ${device_path} test1 test2 pass2

	sudo fsck -fy ${volume_path}test2 >> fsck.txt
	
	# try opening all formats
	
	echo "$i" >> results.txt
	
	mp3_s=$((ffmpeg -v error -i SampleAudio_0.4mb.mp3 -f null test | wc -l))
