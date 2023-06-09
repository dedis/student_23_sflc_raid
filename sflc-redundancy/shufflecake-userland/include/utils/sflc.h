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
 * Miscellaneous constants that must match with the definitions in the Kernel module
 */

#ifndef _UTILS_SFLC_H_
#define _UTILS_SFLC_H_


/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/


/*****************************************************
 *                    CONSTANTS                      *
 *****************************************************/

/* Name of the DM target in the kernel */
#define SFLC_DM_TARGET_NAME "shufflecake"

/* Disk constants */
#define SFLC_SECTOR_SIZE 	4096	/* bytes */
#define KERNEL_SECTOR_SIZE 	512		/* bytes */
#define SFLC_SECTOR_SCALE 	(SFLC_SECTOR_SIZE / KERNEL_SECTOR_SIZE)

/* Max number of volumes in a device */
#define SFLC_DEV_MAX_VOLUMES	15

/* Max total number of open devices at any given time */
#define SFLC_TOT_MAX_DEVICES	1024
/* A volume name is sflc-<devID>-<volIdx> */
#define SFLC_MAX_VOL_NAME_LEN	15

/* A slice index is represented over 32 bits */
#define SFLC_SLICE_IDX_WIDTH	4	/* bytes */
/* A position map block contains 1024 slice indices */
#define SFLC_SLICE_IDX_PER_BLOCK	(SFLC_SECTOR_SIZE / SFLC_SLICE_IDX_WIDTH)

// IV length for AES-CTR
#define SFLC_AESCTR_IVLEN 		16	/* bytes */

/* An IV block can encrypt 256 data blocks */
#define SFLC_DATA_BLOCKS_PER_IV_BLOCK	(SFLC_SECTOR_SIZE / SFLC_AESCTR_IVLEN)
/* A logical slice spans 256 blocks of data (1 MiB) */
#define SFLC_BLOCKS_PER_LOG_SLICE	SFLC_DATA_BLOCKS_PER_IV_BLOCK
/* A physical slice also includes the IV block */
#define SFLC_BLOCKS_PER_PHYS_SLICE 	(1 + SFLC_BLOCKS_PER_LOG_SLICE)

/* A PSI of 0xFFFFFFFF indicates an unassigned LSI */
#define SFLC_EPM_FILLER	0xFF

/* The sysfs file containing the next available device ID */
#define SFLC_SYSFS_NEXTDEVID	"/sys/devices/sflc/next_dev_id"
/* The sysfs directory containing a subdir for each (underlying) block device */
#define SFLC_SYSFS_BDEVS_DIR	"/sys/module/dm_sflc/bdevs"
/* Within each bdev's subdir, this file lists its open volumes */
#define SFLC_SYSFS_OPENVOLUMES_FILENAME	"volumes"

/* TODO: reasonable? */
#define SFLC_BDEV_PATH_MAX_LEN	1024

/* For when you can't be bothered to upper-bound a buffer size */
#define SFLC_BIGBUFSIZE	4096


#endif /* _UTILS_SFLC_H_ */
