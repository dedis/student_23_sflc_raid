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

n_hidden_files = 100
size_hidden_files = 1
metric_hidden_files = "M"

n_decoy_files = 25
size_decoy_files = 300
metric_decoy_files = "K"

n_rounds = 100

normal_fs = '''fsck from util-linux 2.37.2
Pass 1: Checking inodes, blocks, and sizes
Pass 2: Checking directory structure
Pass 3: Checking directory connectivity
Pass 4: Checking reference counts
Pass 5: Checking group summary information'''

n_tests = 30


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

def recover_mapper():
    dmapper = subprocess.Popen(["sudo", "dmesg"], stdout=subprocess.PIPE, text=True)
    grepped = subprocess.Popen(["grep", "VOLUME"], stdin=dmapper.stdout, stdout=subprocess.PIPE, text=True)

    output, error = grepped.communicate()

    print(output)
    print(error)

    if error == "None":
        print("ERROR DURING MAPPER MESSAGES RECOVERY")
        return [],[]
    else:
        formatted = [i.split("VOLUME")[1][1:] for i in output.splitlines()]

        test1_length = int(formatted[-1].split(':')[1].split('/')[0])

        test1 = formatted[-1 - test1_length:-1]

        mapped1 = [int(i.split(':')[1].split("->")[1]) for i in test1]
        mapped1.sort()

        test2_length = int(formatted[-1 - test1_length - 1].split(':')[1].split('/')[0])

        test2 = formatted[-1 - test1_length - 1 - test2_length:-1 - test1_length - 1]

        mapped2 = [int(i.split(':')[1].split("->")[1]) for i in test2]
        mapped2.sort()

        return mapped1, mapped2


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

    subprocess.run(["sudo", "mount", volume_path + "test1", mount_path + "test1"])
    subprocess.run(["sudo", "mount", volume_path + "test2", mount_path + "test2"])

    sumount(mount_path + "test1")
    sumount(mount_path + "test2")

    subprocess.run(["sudo", "./" + sflc_path, "close_vols", device_path])

    resultsfile = open("results" + str(k) + ".txt", "w")

    subprocess.run(["sudo", "./" + sflc_path, "open_vols", device_path, "test1", "test2", "pass2"])

    mapped1, mapped2 = recover_mapper() # baseline

    resultsfile.write(str(mapped1)+'\n')
    resultsfile.write(str(mapped2)+'\n')
    resultsfile.write('\n')

    # prepare hidden files

    subprocess.run(["sudo", "mount", volume_path + "test2", mount_path + "test2"])

    for i in range(1, n_hidden_files + 1):
        cmd_str = "head -c " + str(size_hidden_files) + "M < /dev/urandom"

        bytefile = subprocess.run(["head", "-c", str(size_hidden_files) + metric_hidden_files], stdin=urandom,
                                  capture_output=True)

        hidden_file = open(mount_path + "test2/" + str(i), "wb")
        hidden_file.write(bytefile.stdout)
        hidden_file.close()

    sumount(mount_path + "test2")

    subprocess.run(["sudo", "./" + sflc_path, "close_vols", device_path])

    subprocess.run(["sudo", "./" + sflc_path, "open_vols", device_path, "test1", "test2", "pass2"])

    mapped1, mapped2 = recover_mapper()  # after hidden files

    resultsfile.write(str(mapped1) + '\n')
    resultsfile.write(str(mapped2) + '\n')
    resultsfile.write('\n')

    subprocess.run(["sudo", "./" + sflc_path, "close_vols", device_path])

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

        subprocess.run(["sudo", "./" + sflc_path, "open_vols", device_path, "test1", "test2", "pass2"])

        mapped1, mapped2 = recover_mapper()  # after decoy round

        resultsfile.write(str(mapped1) + '\n')
        resultsfile.write(str(mapped2) + '\n')
        resultsfile.write('\n')

        # verify filesystems
        fs_output = subprocess.run(["sudo", "fsck", "-fy", volume_path + "test2"], capture_output=True).stdout
        fs_output = fs_output.decode('utf-8',errors='ignore')

        if fs_output[:224] != normal_fs:
            fsckfile = open("fsck" + str(k) + ".txt", "a")
            fsckfile.write(str(i) + '\n')
            fsckfile.write(fs_output)
            fsckfile.close()

        subprocess.run(["sudo", "./" + sflc_path, "close_vols", device_path])

    resultsfile.close()

urandom.close()
