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

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <unistd.h>

#include "utils/disk.h"
#include "utils/log.h"


/*****************************************************
 *           PUBLIC FUNCTIONS DEFINITIONS            *
 *****************************************************/

/**
 * Returns the size in 4096-byte sectors (or < 0 if error).
 *
 * @param bdev_path The path of the block device
 *
 * @return The size (in 4096-byte sectors) of the disk, or -errno if error
 */
int64_t sflc_disk_getSize(char * bdev_path)
{
    int fd;
    uint64_t size_bytes;
    int64_t ret;
    
    /* Open file */
    fd = open(bdev_path, O_RDONLY);
    if (fd < 0) {
    	sflc_log_error("Could not open file %s", bdev_path);
        perror("Cause: ");
        ret = -errno;
        goto bad_open;
    }
    sflc_log_debug("Opened file %s", bdev_path);

    /* Get size in bytes */
    if (ioctl(fd, BLKGETSIZE64, &size_bytes) < 0) {
    	sflc_log_error("Could not ioctl file %s", bdev_path);
        perror("Cause: ");
        ret = -errno;
        goto bad_ioctl;
    }
    sflc_log_debug("Size of %s is %lu bytes", bdev_path, size_bytes);

    /* Compute size in SFLC sectors */
    ret = (size_bytes / SFLC_SECTOR_SIZE);

bad_ioctl:
    close(fd);
bad_open:
    return ret;
}

/**
 * Reads a single 4096-byte sector from the disk.
 *
 * @param bdev_path The path of the block device
 * @param sector The index of the desired sector
 * @param The caller-allocated buffer (must hold 4096 bytes) where the data
 *  from the disk will go
 *
 * @return The error code (0 on success)
 */
int sflc_disk_readSector(char * bdev_path, uint64_t sector, char * buf)
{
    int fd;
    int err;
    
    /* Open file */
    fd = open(bdev_path, O_RDONLY);
    if (fd < 0) {
        sflc_log_error("Could not open file %s", bdev_path);
        perror("Cause: ");
        err = errno;
        goto bad_open;
    }
    sflc_log_debug("Opened file %s", bdev_path);

    /* Set offset in bytes */
    if (lseek(fd, sector * SFLC_SECTOR_SIZE, SEEK_SET) < 0) {
        sflc_log_error("Could not lseek file %s to sector %lu", bdev_path, sector);
        perror("Cause: ");
        err = errno;
        goto bad_lseek;
    }
    sflc_log_debug("Successful lseek on file %s to sector %lu", bdev_path, sector);

    /* Read in a loop */
    size_t bytes_to_read = SFLC_SECTOR_SIZE;
    while (bytes_to_read > 0) {
    	/* Read syscall */
		ssize_t bytes_read = read(fd, buf, bytes_to_read);
		if (bytes_read < 0) {
			sflc_log_error("Could not read file %s at sector %lu", bdev_path, sector);
			perror("Cause: ");
			err = errno;
			goto bad_read;
		}

		/* Partial read? No problem just log */
		if (bytes_read < bytes_to_read) {
			sflc_log_debug("Partial read from file %s at sector %lu: %ld bytes instead of %ld",
					bdev_path, sector, bytes_read, bytes_to_read);
		}

		/* Advance loop */
		bytes_to_read -= bytes_read;
		buf += bytes_read;
    }

    // No prob
    err = 0;

bad_read:
bad_lseek:
    close(fd);
bad_open:
    return err;
}


/**
 * Writes a single 4096-byte sector to the disk.
 *
 * @param bdev_path The path of the block device
 * @param sector The index of the desired sector
 * @param The caller-allocated buffer (must hold 4096 bytes) where the data
 *  comes from
 *
 * @return The error code (0 on success)
 */
int sflc_disk_writeSector(char * bdev_path, uint64_t sector, char * buf)
{
	return sflc_disk_writeManySectors(bdev_path, sector, buf, 1);
}


/**
 * Writes many 4096-byte sectors to the disk.
 *
 * @param bdev_path The path of the block device
 * @param sector The index of the starting sector
 * @param The caller-allocated buffer where the data
 *  comes from
 * @param num_sectors The number of sectors to write
 *
 * @return The error code (0 on success)
 */
int sflc_disk_writeManySectors(char * bdev_path, uint64_t sector, char * buf, size_t num_sectors)
{
    int fd;
    int err;
    
    /* Open file */
    fd = open(bdev_path, O_WRONLY);
    if (fd < 0) {
        sflc_log_error("Could not open file %s", bdev_path);
        perror("Cause: ");
        err = errno;
        goto bad_open;
    }
    sflc_log_debug("Opened file %s", bdev_path);

    /* Set offset in bytes */
    if (lseek(fd, sector * SFLC_SECTOR_SIZE, SEEK_SET) < 0) {
        sflc_log_error("Could not lseek file %s to sector %lu", bdev_path, sector);
        perror("Cause: ");
        err = errno;
        goto bad_lseek;
    }
    sflc_log_debug("Successful lseek on file %s to sector %lu", bdev_path, sector);

    /* Write in a loop */
    size_t bytes_to_write = SFLC_SECTOR_SIZE * num_sectors;
    while (bytes_to_write > 0) {
    	/* Write syscall */
		ssize_t bytes_written = write(fd, buf, bytes_to_write);
		if (bytes_written < 0) {
			sflc_log_red("Could not write file %s at sector %lu", bdev_path, sector);
			perror("Cause: ");
			err = errno;
			goto bad_write;
		}

		/* Partial write? No problem just log */
		if (bytes_written < bytes_to_write) {
			sflc_log_debug("Partial write to file %s at sector %lu: %ld bytes instead of %ld",
					bdev_path, sector, bytes_written, bytes_to_write);
		}

		/* Advance loop */
		bytes_to_write -= bytes_written;
		buf += bytes_written;
    }

    // No prob
    err = 0;

bad_write:
bad_lseek:
    close(fd);
bad_open:
    return err;
}
