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

#ifndef _UTILS_DISK_H_
#define _UTILS_DISK_H_

/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/

#include <stdint.h>

#include "utils/sflc.h"


/*****************************************************
 *                      MACROS                       *
 *****************************************************/

/**
 * Max slices for given disk size (in 4096-byte blocks).
 *
 * The bigger a disk is, the more slices it can host. However, the more slices we format it with,
 * the bigger the position map needed to index them: the header size grows with the number of slices,
 * taking up part of the space that's supposed to host those slices.
 * To settle the matter, let us derive an upper bound on the header size, yielding a "safe" value
 * for the number of slices (given a disk size).
 *
 * To index s slices, we need pm := ceil(s/1024) <= s/1024 + 1 PosMap blocks, since each PosMap
 * block (4096 bytes) can host 1024 slice indices (4 bytes each).
 *
 * To encrypt those PosMap blocks, we need iv := ceil(pm/256) <= pm IV blocks, since each IV block
 * (4096 bytes) can encrypt 256 data blocks (IVs are 16 bytes).
 *
 * Therefore, a position map indexing s slices occupies pm+iv <= 2*pm <= 2*s/1024 + 2 blocks.
 *
 * A single volume's header contains the Volume Master Block and the position map, therefore it
 * occupies 1+pm+iv <= 2*s/1024 + 3 blocks.
 *
 * The entire device's header simply contains 15 volume headers of the same size, therefore it
 * occupies h := 15 * (1+pm+iv) <= 15*2*s/1024 + 3*15 <= s + 3*15 blocks.
 * The last inequality follows from 15*2/1024 <= 1 (we need to enforce this on the symbolic values).
 *
 * To actually host the s slices, the data section needs 257*s blocks (256 data blocks + 1 IV block
 * per slice).
 *
 * Therefore, in order to format a disk with s slices, we need at most (s + 3*15) + 257*s =
 * = (1 + 257)*s + 3*15 blocks.
 *
 * If we are given d blocks on the disk, a safe value for s is one that satisfies
 * (1 + 257)*s + 3*15 <= d    <==>    s <= (d - 3*15) / (1 + 257)
 */
#define sflc_disk_maxSlices(size)	(size - 3*SFLC_DEV_MAX_VOLUMES) / (1 + SFLC_BLOCKS_PER_PHYS_SLICE)


/* Let us enforce, on the symbolic values, the inequality used in the previous bound */
#if SFLC_DEV_MAX_VOLUMES * 2 > SFLC_SLICE_IDX_PER_BLOCK
#error "Invalid combination of parameters, probably SFLC_DEV_MAX_VOLUMES is too big"
#endif


/*****************************************************
 *            PUBLIC FUNCTIONS PROTOTYPES            *
 *****************************************************/

/* Returns the size in 4096-byte sectors (or < 0 if error) */
int64_t sflc_disk_getSize(char * bdev_path);

/* Reads a single 4096-byte sector from the disk */
int sflc_disk_readSector(char * bdev_path, uint64_t sector, char * buf);

/* Writes a single 4096-byte sector to the disk */
int sflc_disk_writeSector(char * bdev_path, uint64_t sector, char * buf);

/* Writes many 4096-byte sectors to the disk */
int sflc_disk_writeManySectors(char * bdev_path, uint64_t sector, char * buf, size_t num_sectors);


#endif /* _UTILS_DISK_H_ */
