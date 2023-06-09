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
#include <stdint.h>

#include "commands/commands.h"
#include "actions/volume.h"
#include "utils/sflc.h"
#include "utils/crypto.h"
#include "utils/file.h"
#include "utils/log.h"


/*****************************************************
 *          PRIVATE FUNCTIONS PROTOTYPES             *
 *****************************************************/

/* Finds the volume opened by the given pwd (counters time-based attacks) */
static int _findVolumeByPwd(sflc_cmd_OpenArgs *args, size_t nr_slices, size_t *vol_idx);

/* Opens the indicated volume with the pwd and the previous ones with the VMB key */
static int _openVolumes(sflc_cmd_OpenArgs *args, size_t nr_slices, size_t vol_idx);

/* Read the next device ID in sysfs */
static int _getNextDevId(size_t *next_dev_id);


/*****************************************************
 *          PUBLIC FUNCTIONS DEFINITIONS             *
 *****************************************************/

/**
 * Open M volumes, from the one whose pwd is provided back up to the first one.
 * Scans the device to find the volume that can be opened with the provided
 * pwd, opens it, then opens the previous ones with their VMB key.
 *
 * @param args->bdev_path The underlying block device
 * @param pwd The password
 * @param pwd_len The password length
 *
 * @return Error code (also if no volume could be opened), 0 on success
 */
int sflc_cmd_openVolumes(sflc_cmd_OpenArgs *args)
{
	int64_t dev_size;
	size_t nr_slices;
	size_t vol_idx;
	int err;

	/* Get number of slices */
	dev_size = sflc_disk_getSize(args->bdev_path);
	if (dev_size < 0) {
		err = -dev_size;
		sflc_log_error("Could not read device size for %s; error %d", args->bdev_path, err);
		return err;
	}
	nr_slices = sflc_disk_maxSlices(dev_size);

	/* Find volume opened by the pwd */
	err = _findVolumeByPwd(args, nr_slices, &vol_idx);
	if (err) {
		sflc_log_error("Could not find volume opened by given password; error %d", err);
		return err;
	}

	/* Was there one? */
	if (vol_idx >= SFLC_DEV_MAX_VOLUMES) {
		sflc_log_error("The provided password opens no volume on the device");
		return EINVAL;
	}

	/* Open volumes */
	return _openVolumes(args, nr_slices, vol_idx);
}


/*****************************************************
 *          PRIVATE FUNCTIONS PROTOTYPES             *
 *****************************************************/

/* Finds the volume opened by the given pwd (counters time-based attacks) */
int _findVolumeByPwd(sflc_cmd_OpenArgs *args, size_t nr_slices, size_t *vol_idx)
{
	sflc_Volume vol;
	bool match;
	int err;

	/* This output value, if preserved, will signal that no volume has been found */
	*vol_idx = SFLC_DEV_MAX_VOLUMES;

	/* Input fields for the checkPwd() function */
	vol.bdev_path = args->bdev_path;
	vol.nr_slices = nr_slices;
	vol.pwd = args->pwd;
	vol.pwd_len = args->pwd_len;

	/* Scan all volume slots, don't stop at the first match (avoid time-based attacks) */
	size_t idx;
	for (idx = 0; idx < SFLC_DEV_MAX_VOLUMES; idx++) {
		/* Check this volume slot */
		vol.vol_idx = idx;
		err = sflc_act_checkPwd(&vol, &match);
		if (err) {
			sflc_log_error("Could not check pwd for volume slot %lu; error %d", idx, err);
			return err;
		}

		/* Mark if match */
		if (match) {
			*vol_idx = idx;
		}
	}

	return 0;
}


/* Opens the indicated volume with the pwd and the previous ones with the VMB key */
int _openVolumes(sflc_cmd_OpenArgs *args, size_t nr_slices, size_t vol_idx)
{
	sflc_Volume vol;
	int err;

	/* Input fields to open a volume */
	vol.bdev_path = args->bdev_path;
	vol.nr_slices = nr_slices;
	/* Get the ID that will be assigned to the block device */
	err = _getNextDevId(&vol.dev_id);
	if (err) {
		sflc_log_error("Could not get next device ID; error %d", err);
		return err;
	}
	sflc_log_debug("Next device ID is %lu", vol.dev_id);

	/* Open the last one with the pwd */
	vol.vol_idx = vol_idx;
	vol.pwd = args->pwd;
	vol.pwd_len = args->pwd_len;
	err = sflc_act_openVolumeWithPwd(&vol);
	if (err) {
		sflc_log_error("Could not open volume %lu with pwd; error %d", vol_idx, err);
		return err;
	}
	sflc_log_debug("Successfully opened most secret volume (%lu) with password", vol_idx);
	printf("Opened volume /dev/mapper/sflc-%lu-%lu\n", vol.dev_id, vol_idx);

	/* Loop backwards to open the previous ones */
	int i;
	for (i = vol_idx-1; i >= 0; i--) {
		size_t idx = (size_t) i;	// Looping directly with idx would overflow at the "last" iteration

		/* This volume's VMB key was an output of the last openVolumeWith*() */
		memcpy(vol.vmb_key, vol.prev_vmb_key, SFLC_CRYPTO_KEYLEN);

		/* Open this volume with VMB key */
		vol.vol_idx = idx;
		err = sflc_act_openVolumeWithKey(&vol);
		if (err) {
			sflc_log_error("Could not open volume %lu with VMB key; error %d. "
					"Some volumes on the device have already been opened, it's recommended you close them", idx, err);
			return err;
		}
		sflc_log_debug("Successfully opened volume %lu with VMB key", idx);
		printf("Opened volume /dev/mapper/sflc-%lu-%lu\n", vol.dev_id, idx);
	}

	return 0;
}


/* Read the next device ID in sysfs */
static int _getNextDevId(size_t *next_dev_id)
{
	char *str_nextdevid;
	int err;

	/* Read sysfs entry */
	str_nextdevid = sflc_readFile(SFLC_SYSFS_NEXTDEVID);
	if (!str_nextdevid) {
		sflc_log_error("Could not read sysfs entry %s", SFLC_SYSFS_NEXTDEVID);
		return EINVAL;
	}

	/* Parse integer */
	if (sscanf(str_nextdevid, "%lu", next_dev_id) != 1) {
		sflc_log_error("Error parsing content of file %s", SFLC_SYSFS_NEXTDEVID);
		err = EINVAL;
		goto err_devid;
	}
	/* Sanity check */
	if (*next_dev_id >= SFLC_TOT_MAX_DEVICES) {
		sflc_log_error("There are already %d open devices, this is the maximum allowed", SFLC_TOT_MAX_DEVICES);
		err = E2BIG;
		goto err_devid;
	}

	/* All good */
	err = 0;


err_devid:
	free(str_nextdevid);
	return err;
}

