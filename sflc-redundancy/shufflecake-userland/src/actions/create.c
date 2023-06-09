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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "header/position_map.h"
#include "header/volume_master_block.h"
#include "actions/volume.h"
#include "utils/sflc.h"
#include "utils/crypto.h"
#include "utils/log.h"


/*****************************************************
 *          PUBLIC FUNCTIONS DEFINITIONS             *
 *****************************************************/

/**
 * Writes a volume header (VMB+PM) on-disk. Also fills the vmb_key and volume_key.
 * Some of the fields in the struct are input, some are output.
 *
 * @param pwd The volume password
 * @param pwd_len The password length
 * @param prev_vmb_key The previous volume's VMB key
 * @param nr_slices The number of logical slices of the volume(s)
 * @param vol_idx The index of the volume within the device
 * @param bdev_path The underlying block device to write the volume header
 *
 * @output vmb_key This volume's VMB key
 * @output volume_key This volume's data section key
 *
 * @unused label
 *
 * @return Error code, 0 on success
 */
int sflc_act_createVolume(sflc_Volume *vol)
{
	sflc_VolumeMasterBlock vmb;
	char enc_vmb[SFLC_SECTOR_SIZE];
	sflc_EncPosMap epm;
	uint64_t sector;
	int err;

	// Sample keys
	sflc_rand_getWeakBytes(vol->vmb_key, SFLC_CRYPTO_KEYLEN);
	sflc_rand_getWeakBytes(vol->volume_key, SFLC_CRYPTO_KEYLEN);

	// Fill VMB
	memcpy(vmb.vmb_key, vol->vmb_key,  SFLC_CRYPTO_KEYLEN);
	memcpy(vmb.volume_key, vol->volume_key, SFLC_CRYPTO_KEYLEN);
	memcpy(vmb.prev_vmb_key, vol->prev_vmb_key, SFLC_CRYPTO_KEYLEN);
	vmb.nr_slices = vol->nr_slices;

	// Encrypt it
	err = sflc_vmb_seal(&vmb, vol->pwd, vol->pwd_len, enc_vmb);
	if (err) {
		sflc_log_error("Could not seal VMB; error %d", err);
		goto out;
	}

	// Write it to disk
	sector = sflc_vmbPosition(vol);
	err = sflc_disk_writeSector(vol->bdev_path, sector, enc_vmb);
	if (err) {
		sflc_log_error("Could not write VMB to disk; error %d", err);
		goto out;
	}
	sector += 1;

	// Create encrypted empty position map
	err = sflc_epm_create(vol->nr_slices, vol->volume_key, &epm);
	if (err) {
		sflc_log_error("Could not create encrypted empty position map; error %d", err);
		goto out;
	}

	// Loop over PMB arrays to write it to disk
	int i;
	for (i = 0; i < epm.nr_arrays; i++) {
		char *iv_block = epm.iv_blocks[i];
		char *pmb_array = epm.pmb_arrays[i];
		size_t nr_pmbs = ((i == epm.nr_arrays-1) ? epm.nr_last_pmbs : SFLC_BLOCKS_PER_LOG_SLICE);

		// First write the IV block
		err = sflc_disk_writeSector(vol->bdev_path, sector, iv_block);
		if (err) {
			sflc_log_error("Could not write IV block to disk; error %d", err);
			goto out;
		}
		sector += 1;

		// Then the whole PMB array
		err = sflc_disk_writeManySectors(vol->bdev_path, sector, pmb_array, nr_pmbs);
		if (err) {
			sflc_log_error("Could not write PMB array to disk; error %d", err);
			goto out;
		}
		sector += nr_pmbs;

		// Free them both
		free(iv_block);
		free(pmb_array);
	}

	// Free containers
	free(epm.iv_blocks);
	free(epm.pmb_arrays);


out:
	return err;
}
