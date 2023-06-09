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

/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "header/position_map.h"
#include "utils/sflc.h"
#include "utils/crypto.h"
#include "utils/math.h"
#include "utils/log.h"


/*****************************************************
 *          PUBLIC FUNCTIONS DEFINITIONS             *
 *****************************************************/

/* Create an encrypted empty position map for the given number of slices.
 * Allocates the internal pointers of the EPM structure.
 * On failure, does not free the allocated memory.
 *
 * @param nr_slices The number of slices the device will be composed of.
 * @param volume_key The volume's data section encryption key, used to encrypt the
 *  position map as well.
 * @param epm The EncPosMap struct to be initialised.
 *
 * @return Error code, 0 on success */
int sflc_epm_create(size_t nr_slices, char *volume_key, sflc_EncPosMap *epm)
{
	size_t nr_pmbs;
	size_t nr_arrays;
	int err;

	// Each PosMapBlock holds up to 1024 PSIs (Physical Slice Index)
	nr_pmbs = ceil(nr_slices, SFLC_SLICE_IDX_PER_BLOCK);
	// Each array holds up to 256 PosMapBlocks
	nr_arrays = ceil(nr_pmbs, SFLC_BLOCKS_PER_LOG_SLICE);

	// Fill the EPM numeric fields
	epm->nr_arrays = nr_arrays;
	// All arrays are full except the last one
	epm->nr_last_pmbs = nr_pmbs - (SFLC_BLOCKS_PER_LOG_SLICE * (nr_arrays - 1));

	// Allocate array of IV blocks
	epm->iv_blocks = malloc(nr_arrays * sizeof(char *));
	if (!epm->iv_blocks) {
		sflc_log_error("Could not malloc array of IV blocks");
		err = ENOMEM;
		goto out;
	}
	// Allocate array of PosMapBlock arrays
	epm->pmb_arrays = malloc(nr_arrays * sizeof(char *));
	if (!epm->pmb_arrays) {
		sflc_log_error("Could not malloc array of PosMapBlock arrays");
		err = ENOMEM;
		goto out;
	}

	// Loop to allocate and encrypt each array
	int i;
	for (i = 0; i < nr_arrays; i++) {
		// The last PMB array might be smaller
		size_t nr_pmbs_here = ((i == nr_arrays - 1) ? epm->nr_last_pmbs : SFLC_BLOCKS_PER_LOG_SLICE);
		size_t pmb_array_size = SFLC_SECTOR_SIZE * nr_pmbs_here;
		char *iv_block;
		char *pmb_array;

		// Allocate IV block
		epm->iv_blocks[i] = malloc(SFLC_SECTOR_SIZE);
		if (!epm->iv_blocks[i]) {
			sflc_log_error("Could not allocate IV block number %d", i);
			err = ENOMEM;
			goto out;
		}
		// Allocate PosMapBlock array
		epm->pmb_arrays[i] = malloc(pmb_array_size);
		if (!epm->pmb_arrays[i]) {
			sflc_log_error("Could not allocate PMB array number %d", i);
			err = ENOMEM;
			goto out;
		}
		// Shorthand
		iv_block = epm->iv_blocks[i];
		pmb_array = epm->pmb_arrays[i];

		// Fill the IV block with random data (can ignore return value)
		sflc_rand_getWeakBytes(iv_block, SFLC_SECTOR_SIZE);
		// Fill the PMB array with 0xFF
		memset(pmb_array, SFLC_EPM_FILLER, pmb_array_size);

		// Loop to encrypt each PMB separately with its IV
		int j;
		for (j = 0; j < nr_pmbs_here; j++) {
			char *iv = iv_block + (j * SFLC_AESCTR_IVLEN);
			char *pmb = pmb_array + (j * SFLC_SECTOR_SIZE);

			// Encrypt in-place
			err = sflc_aes256ctr_encrypt(volume_key, pmb, SFLC_SECTOR_SIZE, iv, NULL);
			if (err) {
				sflc_log_error("Could not encrypt PMB %d of array %d", j, i);
				goto out;
			}
		}
	}


out:
	return err;
}
