/*
 * Copyright The Shufflecake Project Authors (2022)
 * Copyright The Shufflecake Project Contributors (2022)
 * Copyright Contributors to the The Shufflecake Project.
 *
 * See the AUTHORS file at the top-level directory of this distribution and at
 * <https://www.shufflecake.net/permalinks/shufflecake-userland/AUTHORS>
 *
 * This file is part of the program shufflecake-userland, which is part of the
 * Shufflecake Project. Shufflecake is a plausible deniability (hidden storage)
 * layer for Linux. See <https://www.shufflecake.net>.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version. This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details. You should have received a copy of the
 * GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 */

/*****************************************************
 *                 INCLUDE SECTION                  *
 *****************************************************/

#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "utils/disk.h"
#include "utils/crypto.h"
#include "actions/volume.h"
#include "test_actions.h"
#include "minunit.h"
#include "utils/log.h"


/*****************************************************
 *                     CONSTANTS                     *
 *****************************************************/

#define DUMMY_PWD "lolololol"
#define WRONG_PWD "yeyeyeyeye"
#define MAX_BDEV_PATH_LEN 100


/*****************************************************
 *          PUBLIC FUNCTIONS DEFINITIONS            *
 *****************************************************/

char *test_vol_allActions()
{
	sflc_Volume vol;
	char bdev_path[MAX_BDEV_PATH_LEN + 1];
	char label[SFLC_MAX_VOL_NAME_LEN + 1];
	int64_t dev_size;
	size_t nr_slices;
	bool match;
	int err;

	sflc_log_blue("Testing all volume operations (needs sudo privileges and dm-sflc loaded)");

	// Get bdev_path or terminate
	printf("Type path of underlying block device (empty to skip test case): ");
	fgets(bdev_path, MAX_BDEV_PATH_LEN, stdin);
	bdev_path[strlen(bdev_path) - 1] = '\0';	// Discard newline
	if (strlen(bdev_path) == 0) {
		sflc_log_yellow("Skipping test case");
		return NULL;
	}
	// Get vol_idx
	printf("Type index of volume within the device: ");
	scanf("%lu", &vol.vol_idx);

	// Get device info
	dev_size = sflc_disk_getSize(bdev_path);
	mu_assert("Error reading device size", dev_size > 0);
	nr_slices = sflc_disk_maxSlices(dev_size);
	sflc_log_yellow("Device has %ld blocks, corresponding to %lu logical slices", dev_size, nr_slices);

	// Fill input fields in volume
	vol.bdev_path = bdev_path;
	vol.nr_slices = nr_slices;
	vol.pwd = DUMMY_PWD;
	vol.pwd_len = strlen(DUMMY_PWD);
	memset(vol.prev_vmb_key, 0, SFLC_CRYPTO_KEYLEN);

	// Create volume
	err = sflc_act_createVolume(&vol);
	mu_assert("Error creating volume", !err);

	// Check with the wrong password
	vol.pwd = WRONG_PWD;
	vol.pwd_len = strlen(WRONG_PWD);
	err = sflc_act_checkPwd(&vol, &match);
	mu_assert("Error checking wrong volume password", !err);
	mu_assert("Wrong password unexpectedly manages to open volume", !match);

	// Check with the right password
	vol.pwd = DUMMY_PWD;
	vol.pwd_len = strlen(DUMMY_PWD);
	err = sflc_act_checkPwd(&vol, &match);
	mu_assert("Error checking right volume password", !err);
	mu_assert("Correct password doesn't open volume", match);

	// Actually open it (using the pwd)
	err = sflc_act_openVolumeWithPwd(&vol);
	mu_assert("Error opening volume with password", !err);

	// Close it (hack: label is known)
	sprintf(label, "sflc-0-%lu", vol.vol_idx);
	err = sflc_act_closeVolume(label);
	mu_assert("Error closing volume (opened with pwd)", !err);

	// Open it again (using the key)
	err = sflc_act_openVolumeWithKey(&vol);
	mu_assert("Error opening volume with key", !err);

	// Close it again
	err = sflc_act_closeVolume(label);
	mu_assert("Error closing volume (opened with key)", !err);

	sflc_log_green("Test case finished, manually check the results");

	return NULL;
}
