# Shufflecake - Kernel module

This repository contains the kernel module implementing the _Shufflecake_ scheme as a device-mapper target for the Linux kernel.

## Installation

This kernel module was developed for the 5.15 Linux kernel, you can run it on Ubuntu 22.04 LTS. __WARNING__: Shufflecake is experimental software, there might still be some runtime bugs (which are under investigation) occasionally compromising the system's stability and requiring a reboot. It is recommended to run it on a VM or spare machine.

The following instructions are given for Debian/Ubuntu and similar derivatives.

### Compiling

You need the kernel headers to compile the source. Use 

`sudo apt install linux-headers-$(uname -r)`.  

After that, just run `make`. All the compilation artifacts will go in the `bin/` directory.

You might encounter a compilation error by `make` saying that the installed GCC version is different (probably newer) from the one used to build the kernel: GCC-specific features are indeed aggressively used by kernel developers, so the GCC versions really need to match if you want to load an out-of-tree module into an already-compiled kernel. If that's the case, you have to downgrade GCC to the indicated version. Work is in progress to fix this.

### Running

Load the module with `sudo insmod bin/dm-sflc.ko`.  
Remove the module with `sudo rmmod dm_sflc`. You can only do this if no _Shufflecake_ volume is open.


## Copyright

Copyright The Shufflecake Project Authors (2022)

Copyright The Shufflecake Project Contributors (2022)

Copyright Contributors to the The Shufflecake Project.

See the AUTHORS file at the top-level directory of this distribution and at <https://www.shufflecake.net/permalinks/shufflecake-userland/AUTHORS>.

The program dm-sflc is part of the Shufflecake Project. Shufflecake is a plausible deniability (hidden storage) layer for Linux. See <https://www.shufflecake.net>.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
    
