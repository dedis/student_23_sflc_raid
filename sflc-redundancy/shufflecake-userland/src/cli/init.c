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

#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "cli/cli.h"
#include "commands/commands.h"
#include "utils/sflc.h"
#include "utils/input.h"
#include "utils/log.h"


/*****************************************************
 *          PUBLIC FUNCTIONS DEFINITIONS             *
 *****************************************************/

/**
 * Gather user inputs interactively to create volumes
 *
 * @return Error code, 0 on success
 */
int sflc_cli_init(void)
{
	sflc_cmd_InitArgs args;
	char bdev_path[SFLC_BDEV_PATH_MAX_LEN + 2];
	char str_nrvols[SFLC_BIGBUFSIZE];
	char *pwds[SFLC_DEV_MAX_VOLUMES];
	size_t pwd_lens[SFLC_DEV_MAX_VOLUMES];
	char str_randfill[SFLC_BIGBUFSIZE];
	int err;

	/* Gather (absolute) path to underlying block device */
	printf("Enter the absolute path to the underlying block device you want to format with Shufflecake: ");
	err = sflc_safeReadLine(bdev_path, SFLC_BDEV_PATH_MAX_LEN + 2);
	if (err) {
		sflc_log_error("Could not read path to underlying block device; error %d", err);
		return err;
	}
	/* Check that it is absolute */
	if (bdev_path[0] != '/') {
		printf("The path to the block device must be absolute");
		return EINVAL;
	}
	/* Assign it */
	args.bdev_path = bdev_path;

	/* Gather number of volumes */
	printf("\nHow many volumes do you want to create (maximum is %d)? ", SFLC_DEV_MAX_VOLUMES);
	err = sflc_safeReadLine(str_nrvols, SFLC_BIGBUFSIZE);
	if (err) {
		sflc_log_error("Could not read number of volumes; error %d", err);
		return err;
	}
	/* Parse string */
	if (sscanf(str_nrvols, "%lu\n", &args.nr_vols) != 1) {
		sflc_log_error("Could not parse number of volumes");
		return EINVAL;
	}
	/* Bounds check */
	if (args.nr_vols > SFLC_DEV_MAX_VOLUMES) {
		printf("Number of volumes too high, Shufflecake supports up to %d volumes on a single device", SFLC_DEV_MAX_VOLUMES);
		return EINVAL;
	}

	/* Gather the passwords */
	printf("\nNow you will be asked to insert the passwords for all the volumes you want to create, \nfrom "
			"volume 0 (the least secret) to volume %lu (the most secret).\n\n", args.nr_vols - 1);
	size_t i;
	for (i = 0; i < args.nr_vols; i++) {
		/* Allocate pwd */
		pwds[i] = malloc(SFLC_BIGBUFSIZE);

		/* Read it */
		printf("Choose password for volume %lu: ", i);
		err = sflc_safeReadLine(pwds[i], SFLC_BIGBUFSIZE);
		if (err) {
			sflc_log_error("Could not read password for volume %lu; error %d", i, err);
			return err;
		}

		/* You can trust the length of strings input this way */
		pwd_lens[i] = strlen(pwds[i]);
	}
	/* Assign them */
	args.pwds = pwds;
	args.pwd_lens = pwd_lens;

	/* Ask whether we should randfill or not */
	printf("\nShould we fill the device with random data before formatting it?\n"
			"It's recommended to do it, you should only skip it if you know what you're doing,\n"
			"or for testing purposes. Type n or N to skip, anything else to rand-fill the device: ");
	err = sflc_safeReadLine(str_randfill, SFLC_BIGBUFSIZE);
	if (err) {
		sflc_log_error("Could not read rand-fill switch; error %d", err);
		return err;
	}
	/* Only decide based on the first character */
	if (str_randfill[0] == 'n' || str_randfill[0] == 'N') {
		printf("Skipping the rand-fill operation\n");
		args.no_randfill = true;
	}
	else {
		printf("Going to perform the rand-fill operation\n");
		args.no_randfill = false;
	}

	/* Actually perform the command */
	return sflc_cmd_initVolumes(&args);
}

// REDUNDANCY MITIGATION

int sflc_cli_redinit(void)
{
	sflc_cmd_InitArgs args;
	char bdev_path[SFLC_BDEV_PATH_MAX_LEN + 2];
	char str_nrvols[SFLC_BIGBUFSIZE];
	char *pwds[SFLC_DEV_MAX_VOLUMES];
	size_t pwd_lens[SFLC_DEV_MAX_VOLUMES];
	char str_randfill[SFLC_BIGBUFSIZE];
	int err;

	/* Gather (absolute) path to underlying block device */
	printf("Enter the absolute path to the underlying block device you want to format with Shufflecake: ");
	err = sflc_safeReadLine(bdev_path, SFLC_BDEV_PATH_MAX_LEN + 2);
	if (err) {
		sflc_log_error("Could not read path to underlying block device; error %d", err);
		return err;
	}
	/* Check that it is absolute */
	if (bdev_path[0] != '/') {
		printf("The path to the block device must be absolute");
		return EINVAL;
	}
	/* Assign it */
	args.bdev_path = bdev_path;

	/* Gather number of volumes */
	printf("\nThe first volume will not be redundant. It serves as a general decoy.");
	printf("\nHow many additionnal redundant volumes do you want to create (maximum is %d)? ", (SFLC_DEV_MAX_VOLUMES-1)/2);
	err = sflc_safeReadLine(str_nrvols, SFLC_BIGBUFSIZE);
	if (err) {
		sflc_log_error("Could not read number of volumes; error %d", err);
		return err;
	}
	/* Parse string */
	if (sscanf(str_nrvols, "%lu\n", &args.nr_vols) != 1) {
		sflc_log_error("Could not parse number of volumes");
		return EINVAL;
	}
	/* Bounds check */
	if (args.nr_vols > SFLC_DEV_MAX_VOLUMES) {
		printf("Number of volumes too high, Shufflecake in re supports up to %d volumes on a single device", SFLC_DEV_MAX_VOLUMES);
		return EINVAL;
	}

	/* For the decoy volume */
	args.nr_vols += 1;

	/* Gather the passwords */
	printf("\nNow you will be asked to insert the passwords for all the volumes you want to create, \nfrom "
			"volume 0 (the least secret) to volume %lu (the most secret).\n\n", args.nr_vols - 1);
	size_t i;
	for (i = 0; i < args.nr_vols; i++) {
		/* Allocate pwd */
		pwds[i] = malloc(SFLC_BIGBUFSIZE);

		/* Read it */
		printf("Choose password for volume %lu: ", i);
		err = sflc_safeReadLine(pwds[i], SFLC_BIGBUFSIZE);
		if (err) {
			sflc_log_error("Could not read password for volume %lu; error %d", i, err);
			return err;
		}

		/* You can trust the length of strings input this way */
		pwd_lens[i] = strlen(pwds[i]);
	}
	/* Assign them */
	args.pwds = pwds;
	args.pwd_lens = pwd_lens;

	/* Ask whether we should randfill or not */
	printf("\nShould we fill the device with random data before formatting it?\n"
			"It's recommended to do it, you should only skip it if you know what you're doing,\n"
			"or for testing purposes. Type n or N to skip, anything else to rand-fill the device: ");
	err = sflc_safeReadLine(str_randfill, SFLC_BIGBUFSIZE);
	if (err) {
		sflc_log_error("Could not read rand-fill switch; error %d", err);
		return err;
	}
	/* Only decide based on the first character */
	if (str_randfill[0] == 'n' || str_randfill[0] == 'N') {
		printf("Skipping the rand-fill operation\n");
		args.no_randfill = true;
	}
	else {
		printf("Going to perform the rand-fill operation\n");
		args.no_randfill = false;
	}

	/* Actually perform the command */
	return sflc_cmd_redinitVolumes(&args);
}