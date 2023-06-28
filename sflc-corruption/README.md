# README

This folder contains the different tests used in the report in Chapter 5 : Data Corruption and Chapter 7 : Results. All these tests were run on a machine running Ubuntu 22.04 with the 5.15 Linux kernel. The parameters used in each test are explained in the report. The library required for the Python virtual environment are listed in `requirements.txt`.



`checksum_level` corresponds to Section 5.3 : Checksum analysis. Run the script `checksum_corruption.sh` to run the test. All tests parameters can be changed at the beginning of the script. It produces two files in each test round, `fsck.txt` that contains the filesystem events that happened and `results.txt` that contains the checksum corruption results.

The test results used in the report are saved in `checksum_results` along with the Python scripts used to make graphs out of them. The filesystem events files were parsed manually into the format that is used Python scripts. The test parameters have to be specified at the beginning of each Python script and the graphs are saved locally in their own folder.



`byte_level` corresponds to Section 5.4 : Byte-level analysis. Run the script `byte_corrution.sh` to run the test. All tests parameters can be changed at the beginning of the script. It saves the bytefiles used for the tests in a separate folder called `byte_test`. It produces two files in each test round, `fsck.txt` that contains the filesystem events that happened and `results.txt` that contains the checksum corruption results.

The test results used in the report are saved in `byte_results` along with the Python scripts used to make graphs out of them. The filesystem events files were parsed manually into the format that is used Python scripts. The test parameters have to be specified at the beginning of each Python script and the graphs are saved locally in their own folder.

Additionnal Python scripts are available in the folder `visual_corruption` to showcase the corruption on a byte-level graphically



`slice_level` corresponds to Section 5.5 : Slice analysis. The folder `slice_corruption` contains Pythons scripts that were used to showcase the corruption on a slice-level graphically. The folder `slice_rate` contains the tests used to measure the rate corruption of the slices. Run the `test.py` Python script, the tests parameters can be changed at the beginning of the script. It produces two files in each test round, `fsck.txt` that contains the filesystem events that happened and `results.txt` that contains the checksum corruption results.

The `rate_graph.py` Python script can be used to generate the graphs from the results. There is also a second Python script `mitigated_rate_graph.py` to generate the corruption rate graphs when the mitigations were applied. The results for those tests were obtained manually, by sending the commands and looking at the logs after each round, because of the instability of the system, as it is explained in the report.



The `test_files` folder contains sample files of different formats that can be used for testing. They come from the website https://sample-videos.com/ Finally, the `utilities` folder contain simple scripts that were developped to make the use of Shufflecake faster.



All tests contain the original results that were used to generate the graphs in the report and the graphs themselves.