import subprocess
import time
import os
from contextlib import contextmanager

driver_path = "../../dm-sflc"

subprocess.run(["sudo", "insmod", driver_path + "/bin/dm-sflc.ko"])

sflc_path = "../../shufflecake-userland/build/shufflecake"
device_path = "/dev/sda1"
volume_path = "/dev/mapper/sflc-"
mount_path = "/media/sflc/"  # mount folders already exist

n_hidden_files = 1
size_hidden_files = 1
metric_hidden_files = "K"

n_decoy_files = 20
size_decoy_files = 1
metric_decoy_files = "M"

n_rounds = 40

normal_fs = '''fsck from util-linux 2.37.2
Pass 1: Checking inodes, blocks, and sizes
Pass 2: Checking directory structure
Pass 3: Checking directory connectivity
Pass 4: Checking reference counts
Pass 5: Checking group summary information'''

n_tests = 10


# safe unmount function
def sumount(mount):
    busy = True
    while (busy):
        mountpoint = subprocess.run(["mountpoint", mount], capture_output=True).stdout.decode('utf-8')
        if mountpoint == mount+" is a mountpoint\n":
            umount = subprocess.run(["umount", mount]).returncode
            if umount == 0:
                busy = False
            else:
                time.sleep(5)
        else:
            busy = False


@contextmanager
def opened_w_error(filename, mode="r"):
    try:
        f = open(filename, mode)
    except IOError as err:
        yield None, err
    else:
        try:
            yield f, None
        finally:
            f.close()


urandom = open("/dev/urandom", "rb")

for k in range(1, n_tests + 1):
    print("test " + str(k))

    # reset result files
    if os.path.exists("results" + str(k) + ".txt"):
        os.remove("results" + str(k) + ".txt")
    if os.path.exists("fsck" + str(k) + ".txt"):
        os.remove("fsck" + str(k) + ".txt")

    # prepare volumes
    subprocess.run(["sudo", "./" + sflc_path, "create_vols", "--no-randfill", device_path, "pass1", "pass2"])

    subprocess.run(["sudo", "./" + sflc_path, "open_vols", device_path, "test1", "test2", "pass2"])

    subprocess.run(["sudo", "mkfs.ext4", volume_path + "test1"])
    subprocess.run(["sudo", "mkfs.ext4", volume_path + "test2"])

    subprocess.run(["sudo", "mount", volume_path + "test2", mount_path + "test2"])

    # prepare hidden files
    for i in range(1, n_hidden_files + 1):
        cmd_str = "head -c " + str(size_hidden_files) + "M < /dev/urandom"

        bytefile = subprocess.run(["head", "-c", str(size_hidden_files) + metric_hidden_files], stdin=urandom,
                                  capture_output=True)

        hidden_file = open(mount_path + "test2/" + str(i), "wb")
        hidden_file.write(bytefile.stdout)
        hidden_file.close()

        copyfile = open("byte_test/" + str(i), "wb")
        copyfile.write(bytefile.stdout)
        copyfile.close()

    sumount(mount_path + "test2")

    subprocess.run(["sudo", "./" + sflc_path, "close_vols", device_path])

    resultsfile = open("results" + str(k) + ".txt", "a")

    # start rounds
    for i in range(1, n_rounds + 1):
        print("round " + str(i))

        subprocess.run(["sudo", "./" + sflc_path, "open_vols", device_path, "test1", "pass1"])

        subprocess.run(["sudo", "mount", volume_path + "test1", mount_path + "test1"])

        # create decoy files
        for j in range(1, n_decoy_files + 1):
            filename = str(n_decoy_files * (i - 1) + j)
            bytefile = subprocess.run(["head", "-c", str(size_decoy_files) + metric_decoy_files], stdin=urandom,
                                      capture_output=True)

            decoyfile = open(mount_path + "test1/" + filename, "wb")
            decoyfile.write(bytefile.stdout)
            decoyfile.close()

        sumount(mount_path + "test1")

        subprocess.run(["sudo", "./" + sflc_path, "close_vols", device_path])

        # verify filesystems
        subprocess.run(["sudo", "./" + sflc_path, "open_vols", device_path, "test1", "test2", "pass2"])

        fs_output = subprocess.run(["sudo", "fsck", "-fy", volume_path + "test2"], capture_output=True).stdout
        fs_output = fs_output.decode('utf-8')

        if fs_output[:224] != normal_fs:
            fsckfile = open("fsck" + str(k) + ".txt", "a")
            fsckfile.write(str(i) + '\n')
            fsckfile.write(fs_output)
            fsckfile.close()

        # check hidden files
        subprocess.run(["sudo", "mount", volume_path + "test2", mount_path + "test2"])

        for j in range(1, n_hidden_files + 1):
            with opened_w_error("byte_test/" + str(j), "rb") as (reference, err_reference):
                with opened_w_error(mount_path + "test2/" + str(j), "rb") as (test, err_test):
                    if err_reference:
                        print("CANNOT OPEN REFERENCE FILE, TERMINATING", err_reference)
                        exit(1)
                    elif err_test:
                        print("cannot open test file, considering it lost", err_test)
                        if metric_hidden_files == "K":
                            resultsfile.write('0' * 1024)
                        elif metric_hidden_files == "M":
                            resultsfile.write('0' * (1024 * 1024))
                        else:
                            print("UNKNWON HIDDEN FILE METRIC, TERMINATING")
                            exit(1)
                    else:
                        reference_byte = reference.read(1)
                        test_byte = test.read(1)

                        # need to be changed causing to write too much when file corrupted
                        while reference_byte != b'' or test_byte != b'':
                            if reference_byte != b'' and test_byte != b'' and reference_byte == test_byte:
                                resultsfile.write('1')
                            else:
                                resultsfile.write('0')
                            if reference_byte != b'':
                                reference_byte = reference.read(1)
                            if test_byte != b'':
                                test_byte = test.read(1)
                    resultsfile.write('\n')

        resultsfile.write('\n')

        sumount(mount_path + "test2")

        subprocess.run(["sudo", "./" + sflc_path, "close_vols", device_path])

    resultsfile.close()

urandom.close()
