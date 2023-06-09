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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "header/volume_master_block.h"
#include "actions/volume.h"
#include "utils/sflc.h"
#include "utils/crypto.h"
#include "utils/file.h"
#include "utils/string.h"
#include "utils/dm.h"
#include "utils/log.h"


/*****************************************************
 *          PRIVATE FUNCTIONS PROTOTYPES             *
 *****************************************************/

/* Open the volume through the appropriate ioctl */
static int _openVolume(sflc_Volume *vol);


/*****************************************************
 *          PUBLIC FUNCTIONS DEFINITIONS             *
 *****************************************************/

/**
 * Check if the password is correct for this volume.
 *
 * @param pwd The password
 * @param pwd_len The password length
 * @param vol_idx The index of the volume in the device
 * @param bdev_path The path to the underlying device
 * @param nr_slices The number of logical slices for this device / volumes
 *
 * @output match A boolean containing the answer
 *
 * @return Error code, 0 on success
 */
int sflc_act_checkPwd(sflc_Volume *vol, bool *match)
{
	char enc_vmb[SFLC_SECTOR_SIZE];
	uint64_t sector;
	int err;

	/* Read encrypted VMB from disk */
	sector = sflc_vmbPosition(vol);
	err = sflc_disk_readSector(vol->bdev_path, sector, enc_vmb);
	if (err) {
		sflc_log_error("Could not read VMB from disk; error %d", err);
		return err;
	}

	/* Try unsealing it */
	return sflc_vmb_tryUnsealWithPwd(enc_vmb, vol->pwd, vol->pwd_len, match);
}


/**
 * Read a VMB from disk and issue a DM ioctl to create the appropriate virtual device
 *
 * @param pwd The password
 * @param pwd_len The password length
 * @param dev_id The ID of the underlying block device
 * @param vol_idx The index of the volume within the device
 * @param bdev_path The path to the underlying device
 * @param nr_slices The number of logical slices for this device / volumes
 *
 * @output vmb_key This volume's VMB key
 * @output volume_key This volume's data section key
 * @output prev_vmb_key The previous volume's VMB key
 * @output label The volume's name under /dev/mapper/
 *
 * @return Error code, 0 on success
 */
int sflc_act_openVolumeWithPwd(sflc_Volume *vol)
{
	sflc_VolumeMasterBlock vmb;
	char enc_vmb[SFLC_SECTOR_SIZE];
	uint64_t sector;
	int err;

	/* Read encrypted VMB from disk */
	sector = sflc_vmbPosition(vol);
	err = sflc_disk_readSector(vol->bdev_path, sector, enc_vmb);
	if (err) {
		sflc_log_error("Could not read VMB from disk; error %d", err);
		return err;
	}

	/* Unseal it */
	err = sflc_vmb_unsealWithPwd(enc_vmb, vol->pwd, vol->pwd_len, &vmb);
	if (err) {
		sflc_log_error("Could not unseal VMB; error %d", err);
		return err;
	}

	/* Compare the number of slices */
	if (vol->nr_slices != vmb.nr_slices) {
		sflc_log_error("Incompatible header size: the device size was different when the volumes"
				"were created. Did you resize the device %s since last time?", vol->bdev_path);
		return EINVAL;
	}
	/* Copy the keys over to the Volume struct */
	memcpy(vol->vmb_key, vmb.vmb_key, SFLC_CRYPTO_KEYLEN);
	memcpy(vol->volume_key, vmb.volume_key, SFLC_CRYPTO_KEYLEN);
	memcpy(vol->prev_vmb_key, vmb.prev_vmb_key, SFLC_CRYPTO_KEYLEN);
	/* Build volume label */
	sprintf(vol->label, "sflc-%lu-%lu", vol->dev_id, vol->vol_idx);

	/* Actually open the volume */
	err = _openVolume(vol);
	if (err) {
		sflc_log_error("Could not open volume; error %d", err);
		return err;
	}

	return 0;
}


/**
 * Read a VMB from disk and issue a DM ioctl to create the appropriate virtual device
 *
 * @param vmb_key The key encrypting this volume's VMB payload
 * @param dev_id The ID of the underlying block device
 * @param vol_idx The index of the volume within the device
 * @param bdev_path The path to the underlying device
 * @param nr_slices The number of logical slices for this device / volumes
 *
 * @output volume_key This volume's data section key
 * @output prev_vmb_key The previous volume's VMB key
 * @output vol_id The volume's unique numeric ID
 * @output label The volume's name under /dev/mapper/
 *
 * @return Error code, 0 on success
 */
int sflc_act_openVolumeWithKey(sflc_Volume *vol)
{
	sflc_VolumeMasterBlock vmb;
	char enc_vmb[SFLC_SECTOR_SIZE];
	uint64_t sector;
	int err;

	/* Read encrypted VMB from disk */
	sector = sflc_vmbPosition(vol);
	err = sflc_disk_readSector(vol->bdev_path, sector, enc_vmb);
	if (err) {
		sflc_log_error("Could not read VMB from disk; error %d", err);
		return err;
	}

	/* Unseal it */
	err = sflc_vmb_unsealWithKey(enc_vmb, vol->vmb_key, &vmb);
	if (err) {
		sflc_log_error("Could not unseal VMB; error %d", err);
		return err;
	}

	/* Compare the number of slices */
	if (vol->nr_slices != vmb.nr_slices) {
		sflc_log_error("Incompatible header size: the device size was different when the volumes"
				"were created. Did you resize the device %s since last time?", vol->bdev_path);
		return EINVAL;
	}
	/* Copy the keys over to the Volume struct */
	memcpy(vol->vmb_key, vmb.vmb_key, SFLC_CRYPTO_KEYLEN);
	memcpy(vol->volume_key, vmb.volume_key, SFLC_CRYPTO_KEYLEN);
	memcpy(vol->prev_vmb_key, vmb.prev_vmb_key, SFLC_CRYPTO_KEYLEN);
	/* Build volume label */
	sprintf(vol->label, "sflc-%lu-%lu", vol->dev_id, vol->vol_idx);

	/* Actually open the volume */
	err = _openVolume(vol);
	if (err) {
		sflc_log_error("Could not open volume; error %d", err);
		return err;
	}

	return 0;
}


/*****************************************************
 *          PRIVATE FUNCTIONS DEFINITIONS            *
 *****************************************************/

/* Open the volume through the appropriate ioctl */
static int _openVolume(sflc_Volume *vol)
{
	char *hex_key;
	char params[SFLC_BIGBUFSIZE];
	uint64_t num_sectors;
	int err;

	/* Get the hex version of the volume's data section key */
	hex_key = sflc_toHex(vol->volume_key, SFLC_CRYPTO_KEYLEN);
	if (!hex_key) {
		sflc_log_error("Could not encode volume key to hexadecimal");
		err = ENOMEM;
		goto err_hexkey;
	}

	/* Get the number of logical 512-byte sectors composing the volume */
	num_sectors = ((uint64_t) vol->nr_slices) * SFLC_BLOCKS_PER_LOG_SLICE * SFLC_SECTOR_SCALE;

	/* Build param list */
	sprintf(params, "%s %lu %lu %s", vol->bdev_path, vol->vol_idx, vol->nr_slices, hex_key);

	/* Issue ioctl */
	err = sflc_dm_create(vol->label, num_sectors, params);
	if (err) {
		sflc_log_error("Could not issue ioctl CREATE command to device mapper; error %d", err);
		goto err_dmcreate;
	}
	err = 0;


err_dmcreate:
	free(hex_key);
err_hexkey:
	return err;
}
