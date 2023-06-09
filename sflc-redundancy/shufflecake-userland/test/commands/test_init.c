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
#include "commands/commands.h"
#include "test_commands.h"
#include "minunit.h"
#include "utils/input.h"
#include "utils/log.h"


/*****************************************************
 *                     CONSTANTS                     *
 *****************************************************/

#define MAX_BDEV_PATH_LEN 100
#define MAX_PWD_LEN	40


/*****************************************************
 *          PUBLIC FUNCTIONS DEFINITIONS            *
 *****************************************************/

char *test_cmd_init()
{
	sflc_cmd_InitArgs args;
	char bdev_path[MAX_BDEV_PATH_LEN + 2];
	size_t nr_vols;
	char pwds[SFLC_DEV_MAX_VOLUMES][MAX_PWD_LEN + 2];
	size_t pwd_lens[SFLC_DEV_MAX_VOLUMES];
	int err;

	sflc_log_blue("Testing volume creation");

	/* Get bdev_path or terminate */
	printf("Type path of underlying block device (empty to skip test case): ");
	err = sflc_safeReadLine(bdev_path, sizeof(bdev_path));
	mu_assert("Could not read path to underlying device", !err);
	/* Terminate if empty input */
	if (strlen(bdev_path) == 0) {
		sflc_log_yellow("Skipping test case");
		return NULL;
	}
	args.bdev_path = bdev_path;

	/* Get number of volumes */
	printf("How many volumes do you want to create?");
	mu_assert("Could not read number of volumes", scanf("%u", &nr_vols) == 1);
	mu_assert("Number of volumes out of bounds", nr_vols <= SFLC_DEV_MAX_VOLUMES);

	/* Get passwords */
	size_t i;
	for (i = 0; )

	sflc_log_green("Test case finished, manually check the results");

	return NULL;
}
