/*
 *  Copyright The Shufflecake Project Authors (2022)
 *  Copyright The Shufflecake Project Contributors (2022)
 *  Copyright Contributors to the The Shufflecake Project.
 *  
 *  See the AUTHORS file at the top-level directory of this distribution and at
 *  <https://www.shufflecake.net/permalinks/shufflecake-userland/AUTHORS>
 *  
 *  This file is part of the program shufflecake-userland, which is part of the 
 *  Shufflecake Project. Shufflecake is a plausible deniability (hidden storage) 
 *  layer for Linux. See <https://www.shufflecake.net>.
 *  
 *  This program is free software: you can redistribute it and/or modify it 
 *  under the terms of the GNU General Public License as published by the Free 
 *  Software Foundation, either version 3 of the License, or (at your option) 
 *  any later version. This program is distributed in the hope that it will be 
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General 
 *  Public License for more details. You should have received a copy of the 
 *  GNU General Public License along with this program. 
 *  If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * Disk helper functions
 */

/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <linux/fs.h>

#include "sflc.h"
#include "disk.h"

/*****************************************************
 *           PUBLIC FUNCTIONS DEFINITIONS            *
 *****************************************************/

/* Returns the size in 4096-byte sectors (or < 0 if error) */
int64_t disk_getSize(char * real_dev_path)
{
    int fd;
    uint64_t size;
    
    /* Open file */
    fd = open(real_dev_path, O_RDONLY);
    if (fd < 0) {
        perror("ERR: Could not open file:");
        return -1;
    }

    /* Get size in bytes */
    if (ioctl(fd, BLKGETSIZE64, &size) < 0) {
        perror("ERR: Could not ioctl file:");
        close(fd);
        return -1;
    }

    close(fd);
    return (size / SFLC_SECTOR_SIZE);
}

/* Reads a single 4096-byte sector from the disk */
int disk_readSector(char * real_dev_path, uint64_t sector, char * buf)
{
    int fd;
    ssize_t bytes_read;
    
    /* Open file */
    fd = open(real_dev_path, O_RDONLY);
    if (fd < 0) {
        perror("ERR: Could not open file:");
        return -1;
    }

    /* Set offset in bytes */
    if (lseek(fd, sector * SFLC_SECTOR_SIZE, SEEK_SET) < 0) {
        perror("ERR: Could not lseek:");
        close(fd);
        return -1;
    }

    /* Read */
    bytes_read = read(fd, buf, SFLC_SECTOR_SIZE);
    if (bytes_read < 0) {
        perror("ERR: Could not read:");
        close(fd);
        return -1;
    }
    if (bytes_read != SFLC_SECTOR_SIZE) {
        print_red("ERR: Only read %ld bytes!\n", bytes_read);
        close(fd);
        return -2;
    }

    close(fd);
    return 0;
}

/* Writes a single 4096-byte sector to the disk */
int disk_writeSector(char * real_dev_path, uint64_t sector, char * buf)
{
    int fd;
    ssize_t bytes_written;
    
    /* Open file */
    fd = open(real_dev_path, O_WRONLY);
    if (fd < 0) {
        perror("ERR: Could not open file:");
        return -1;
    }

    /* Set offset in bytes */
    if (lseek(fd, sector * SFLC_SECTOR_SIZE, SEEK_SET) < 0) {
        perror("ERR: Could not lseek:");
        close(fd);
        return -1;
    }

    /* Write */
    bytes_written = write(fd, buf, SFLC_SECTOR_SIZE);
    if (bytes_written < 0) {
        perror("ERR: Could not write:");
        close(fd);
        return -1;
    }
    if (bytes_written != SFLC_SECTOR_SIZE) {
        print_red("ERR: Only wrote %ld bytes!\n", bytes_written);
        close(fd);
        return -2;
    }

    close(fd);
    return 0;
}
