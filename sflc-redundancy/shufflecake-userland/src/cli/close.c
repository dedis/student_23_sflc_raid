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
 * Gather user inputs interactively to close volumes
 *
 * @return Error code, 0 on success
 */
int sflc_cli_close(void)
{
	char bdev_path[SFLC_BDEV_PATH_MAX_LEN + 2];
	int err;

	/* Gather (absolute) path to underlying block device */
	printf("Enter the absolute path to the underlying block device containing the Shufflecake volumes to close: ");
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

	/* Actually perform the command */
	return sflc_cmd_closeVolumes(bdev_path);
}
