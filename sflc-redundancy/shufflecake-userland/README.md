# Shufflecake - Userland tool

This repository contains the userland tool used to correctly manage the creation, opening, and closing of _Shufflecake_ volumes.

The following instructions are given for Debian/Ubuntu and similar derivatives.


## Compiling

You need the device-mapper userspace library and libsodium to compile the source. Use  
`sudo apt install libdevmapper-dev`  
`sudo apt install libsodium-dev`


After that, just run `make`. All the compilation artifacts will go in the `build/` directory.

## Usage

To run this tool, the _Shufflecake_ kernel module must be compiled and loaded. This is a stateless, non-interactive tool. Run `./build/shufflecake` with no arguments to display a usage prompt. 

In the context of _Shufflecake_, a _device_ is the underlying raw block device that is formatted to contain hidden data, while a _volume_ is the logical, encrypted and hidden partition within a device.

The device can be any block device: a whole USB stick (or disk), a partition, a file-backed loop device, etc. (you likely find it under `/dev/`).

Volumes are password-protected, and embedded in the underlying device as data which is indistinguishable from random noise without the proper password. Up to 15 volumes can be _nested_, i.e., hidden within each other. Providing the password for the _highest_ hierarchy volume (i.e., "most hidden") will automatically unlock also all the volumes in lower hierarchy. For security and consistency reasons, you cannot create/open/close nested volumes within the same device one at a time; the tool only allows to perform these operations on all the volumes in a device at the same time.

### Creating volumes

Creates __N__ nested volumes with the given passwords within the given device, by properly formatting and encrypting the first __N__ slots (out of 15 total) in the device header, and filling the remaining ones with random bytes. Unless a `--no-randfill` option is provided, the entirety of the data section (the rest of the device) is also initialised with random bytes; only use this option for quick testing, or if the device already was already initialised in the past (e.g. it has pre-existing volumes): the complete overwrite of the header guarantees that pre-existing volumes will be wiped by crypto-shredding.

If the device is not empty, you will lose all your data. Also, you will lose all pre-existing _Shufflecake_ volumes on this device, by performing this operation. This is legitimate, and intended: this way, there's no need to re-initialise a device if you want to reuse it differently. Also, you cannot create additional volumes beside the pre-existing ones.

`sudo ./build/shufflecake create_vols [--no-randfill] <device> <pwd1> ... <pwdN>`

The command directly takes the __N__ passwords. It does not open the volumes, i.e. create the virtual block devices under `/dev/mapper/`, it only formats the header.

All the non-header space of the underlying device will be filled with random data. On large devices, this will take quite long. It can be skipped for testing and development purposes, or if the device was already initialised, by passing the option `--no-randfill`.

### Opening volumes

Opens the first __M__ volumes from the device. Only takes the __M__-th password as input, then recursively opens the previous ones (lower hierarchy) using the (encrypted) `prev_pwd` field in each volume's header.

`sudo ./build/shufflecake open_vols <device> <volName1> ... <volNameM> <pwdM>`

Besides the last password, this command also takes __M__ volume labels as input. A volume named `<volName>` will appear as a virtual block device under `/dev/mapper/sflc-<volName>`. For this reason, labels must at all times be unique not just across volumes in a device, but across all devices managed by _Shufflecake_. In the future, label generation and management will be done automatically.

Notice: this command only creates the virtual block devices, you will still need to create and mount your own file systems on them.

### Closing volumes

Closes all the volumes currently open on a device.

`sudo ./build/shufflecake close_vols <device>`


## Maintainers

Elia Anzuoni <elianzuoniATgmail.com>

Tommaso Gagliardoni <tommasoATgagliardoni.net>


## Copyright

Copyright The Shufflecake Project Authors (2022)

Copyright The Shufflecake Project Contributors (2022)

Copyright Contributors to the The Shufflecake Project.

See the AUTHORS file at the top-level directory of this distribution and at <https://www.shufflecake.net/permalinks/shufflecake-userland/AUTHORS>.

The program shufflecake-userland is part of the Shufflecake Project. Shufflecake is a plausible deniability (hidden storage) layer for Linux. See <https://www.shufflecake.net>.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
    

