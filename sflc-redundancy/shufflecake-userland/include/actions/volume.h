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
 * Defines the Volume
 */

#ifndef _ACTION_VOLUME_H_
#define _ACTION_VOLUME_H_


/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/

#include <stdint.h>
#include <stddef.h>

#include "utils/disk.h"
#include "utils/crypto.h"
#include "utils/math.h"


/*****************************************************
 *                    CONSTANTS                      *
 *****************************************************/


/*****************************************************
 *                     STRUCTS                       *
 *****************************************************/

/**
 * Struct representing a volume.
 * The functions in this module take a partially-filled struct in input
 * and fill some more fields in it as part of their output.
 */
typedef struct {
	// Password
	char *pwd;
	size_t pwd_len;

	// Keys
	char vmb_key[SFLC_CRYPTO_KEYLEN];
	char volume_key[SFLC_CRYPTO_KEYLEN];
	char prev_vmb_key[SFLC_CRYPTO_KEYLEN];

	// Number of slices
	size_t nr_slices;

	// ID of the underlying block device
	size_t dev_id;
	// Index of this volume in the underlying block device
	size_t vol_idx;

	// Underlying block device
	char *bdev_path;
	// Volume name under /dev/mapper/
	char label[SFLC_MAX_VOL_NAME_LEN + 1];

} sflc_Volume;


/*****************************************************
 *                 INLINE FUNCTIONS                  *
 *****************************************************/

// Size, in 4096-byte blocks, of a whole volume header (VMB+PM)
static inline size_t sflc_volHeaderSize(size_t nr_slices)
{
	// Each PosMapBlock holds up to 1024 PSIs (Physical Slice Index)
	size_t nr_pmbs = ceil(nr_slices, SFLC_SLICE_IDX_PER_BLOCK);
	// Each array holds up to 256 PosMapBlocks
	size_t nr_arrays = ceil(nr_pmbs, SFLC_BLOCKS_PER_LOG_SLICE);

	// 1 VMB, the PMBs, and the IV blocks
	return 1 + nr_pmbs + nr_arrays;
}

// Position of the VMB for the given volume
static inline uint64_t sflc_vmbPosition(sflc_Volume *vol)
{
	return ((uint64_t) vol->vol_idx) * ((uint64_t) sflc_volHeaderSize(vol->nr_slices));
}


/*****************************************************
 *           PUBLIC FUNCTIONS PROTOTYPES             *
 *****************************************************/

/* Writes a volume header (VMB+PM) on-disk */
int sflc_act_createVolume(sflc_Volume *vol);

/* Check if the password is correct for this volume */
int sflc_act_checkPwd(sflc_Volume *vol, bool *match);
/* Read a VMB from disk and issue a DM ioctl to create the appropriate virtual device */
int sflc_act_openVolumeWithPwd(sflc_Volume *vol);
int sflc_act_openVolumeWithKey(sflc_Volume *vol);

/* Close the volume via the appropriate ioctl to DM */
int sflc_act_closeVolume(char *label);

#endif /* _ACTION_VOLUME_H_ */
