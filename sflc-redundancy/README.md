# README

This folder contains the implementation in the Shufflecake software of the different mitigations presented in the report in Chapter 6 : Mitigations. All these tests were run on a machine running Ubuntu 22.04 with the 5.15 Linux kernel and the version of Shufflecake used is the 0.1



The `dm-sflc` and `shufflecake-userland` folders are, respectively, the modified kernel driver and userland application. The added and/or modified code compared to the original Shufflecake software is indicated in tags (slfc-raid START and END). The main parts of the modified code are in `dm-sflc/src/target/target.c`, `dm-sflc/src/volume/io.c`, `shufflecake-userland/src/main.c` and `shufflecake-userland/src/sflc.c`.



To exchange information with the driver, the kernel ring buffer was used extensively, as it is explained in the report. Finally, the `utilities` folder contain simple scripts that were developped to make the use of Shufflecake faster.