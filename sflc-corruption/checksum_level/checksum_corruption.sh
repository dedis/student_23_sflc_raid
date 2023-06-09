#!/bin/bash
#set -x # to echo all commands when they are entered

echo "loading kernel driver, may cause error if it was already done"

driver_path="../dm-sflc"

sudo insmod ${driver_path}/bin/dm-sflc.ko

sflc_path="../shufflecake-userland/build/shufflecake"
device_path="/dev/sda1"
volume_path="/dev/mapper/sflc-"
mount_path="/media/sflc/" # mount folders already exist

n_hidden_files=100
size_hidden_files=1

n_decoy_files=20
size_decoy_files=1

n_rounds=40

normal_fs="fsck from util-linux 2.37.2
Pass 1: Checking inodes, blocks, and sizes
Pass 2: Checking directory structure
Pass 3: Checking directory connectivity
Pass 4: Checking reference counts
Pass 5: Checking group summary information"

n_tests=30

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

for k in `seq 28 $n_tests`
do
	echo "test $k"
	
	# remove results file if it exists

	if [ -f "results$k.txt" ] ; then
	    rm results$k.txt
	fi

	if [ -f "fsck$k.txt" ] ; then
	    rm fsck$k.txt
	fi

	if [ -f "checksums.txt" ] ; then
	    rm checksums.txt
	fi

	# initialize

	sudo ./${sflc_path} create_vols --no-randfill ${device_path} pass1 pass2

	sudo ./${sflc_path} open_vols ${device_path} test1 test2 pass2

	sudo mkfs.ext4 ${volume_path}test1
	sudo mkfs.ext4 ${volume_path}test2

	# prepare hidden files

	sudo mount ${volume_path}test2 ${mount_path}test2

	for i in `seq 1 $n_hidden_files`
	do
		head -c ${size_hidden_files}M < /dev/urandom | sudo tee ${mount_path}test2/$i > /dev/null
		cksum ${mount_path}test2/$i >> checksums.txt
	done

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
		
		sudo fsck -fy ${volume_path}test2 >> temp.txt
		
		fs_output=$(head -6 temp.txt)
		
		if [ "$fs_output" != "$normal_fs" ]
		then
			echo $i >> fsck$k.txt
			cat temp.txt >> fsck$k.txt
		fi
		
		rm temp.txt

		# verify hidden files

		sudo mount ${volume_path}test2 ${mount_path}test2

		for j in `seq 1 $n_hidden_files`
		do
			cksum ${mount_path}test2/$j >> verify.txt 2> /dev/null
		done

		count=$(diff -y --suppress-common-lines checksums.txt verify.txt | wc -l )
		
		echo $count >> results$k.txt

		rm verify.txt
		
		sumount ${mount_path}test2
		
		sudo ./${sflc_path} close_vols ${device_path}
	done
	
	rm checksums.txt
done
