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
 * Shufflecake core
 */

#ifndef _SFLC_H_
#define _SFLC_H_

/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/

#include <stdbool.h>

#include "utils.h"

/*****************************************************
 *                    CONSTANTS                      *
 *****************************************************/

#define SFLC_USR_PWD_MAX_LEN 40

/* Must match the definition in the kernel module */
#define SFLC_TARGET_NAME "shufflecake"

/* Must match the definitions in the kernel module */
#define SFLC_SECTOR_SIZE 4096
#define KERNEL_SECTOR_SIZE 512
#define SFLC_SECTOR_SCALE (SFLC_SECTOR_SIZE / KERNEL_SECTOR_SIZE)

/* Must match the definitions in the kernel module */
#define SFLC_KERN_IV_LEN 16  // We use AES-CTR in dm_sflc
#define SFLC_SECTOR_TO_IV_RATIO (SFLC_SECTOR_SIZE / SFLC_KERN_IV_LEN)   // An IV block has IVs for 256 data blocks
#define SFLC_VOL_MAX_SLICES (1024 * 1024)    // So max size of a volume is 1 TB
#define SFLC_POS_MAP_ENTRY_LEN 4   // At most 1M slices, so we need 20 bits to index them, rounded to 32
/* One sector reserved for the userland tool, 1024 for the position map, 4 for the relative IVs.
   A volume header occupies (4 MB + 20 kB) of space. */
#define SFLC_VOL_HEADER_SIZE (1 + 1024 + 4)   // In 4096-byte sectors
/* Max 15 volumes */
#define SFLC_DEV_MAX_VOLUMES 15
/* Around 60 MB of device header */
#define SFLC_DEV_HEADER_SIZE (SFLC_DEV_MAX_VOLUMES * SFLC_VOL_HEADER_SIZE)
/* The logical storage space is segmented into 1 MB slices */
#define SFLC_LOG_SLICE_SIZE 256
/* One sector for the IVs, 256 of data */
#define SFLC_PHYS_SLICE_SIZE (1 + SFLC_LOG_SLICE_SIZE)

/*****************************************************
 *            PUBLIC FUNCTIONS PROTOTYPES            *
 *****************************************************/

void sflc_create_vols(char * real_dev_path, char ** pwd, int nr_pwd);
void sflc_open_vols(char * real_dev_path, char ** vol_names, int nr_vols, char * last_pwd, bool vol_creation);
void sflc_close_vols(char * real_dev_path);

#endif /* _SFLC_H_ */
