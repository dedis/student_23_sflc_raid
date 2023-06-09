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

#include "commands/commands.h"
#include "actions/volume.h"
#include "utils/sflc.h"
#include "utils/crypto.h"
#include "utils/string.h"
#include "utils/file.h"
#include "utils/log.h"


/*****************************************************
 *          PRIVATE FUNCTIONS PROTOTYPES             *
 *****************************************************/

/* Reads the list of volumes from sysfs */
static int _readVolumesList(char *bdev_path, char **labels, size_t *nr_vols);

/* Close them all */
static int _closeVolumes(char **labels, size_t nr_vols);


/*****************************************************
 *          PUBLIC FUNCTIONS DEFINITIONS             *
 *****************************************************/

/**
 * Close all volumes on the device (reads the list from sysfs)
 *
 * @param bdev_path The path to the underlying block device
 *
 * @return Error code, 0 on success
 */
int sflc_cmd_closeVolumes(char *bdev_path)
{
	char *labels[SFLC_DEV_MAX_VOLUMES];
	size_t nr_vols;
	int err;

	/* Allocate labels */
	size_t i;
	for (i = 0; i < SFLC_DEV_MAX_VOLUMES; i++) {
		labels[i] = malloc(SFLC_MAX_VOL_NAME_LEN + 1);
		if (!labels[1]) {
			sflc_log_error("Could not allocate volume label %lu", i);
			return ENOMEM;	// Do not free the ones already allocated
		}
	}

	/* Read them */
	err = _readVolumesList(bdev_path, labels, &nr_vols);
	if (err) {
		sflc_log_error("Could not read volume list from sysfs; error %d", err);
		goto out;
	}

	/* Close the volumes */
	err = _closeVolumes(labels, nr_vols);
	if (err) {
		sflc_log_error("Could not close volumes; error %d", err);
		goto out;
	}

	/* No prob */
	err = 0;


out:
	for (i = 0; i < SFLC_DEV_MAX_VOLUMES; i++) {
		free(labels[i]);
	}
	return err;
}


/*****************************************************
 *          PRIVATE FUNCTIONS PROTOTYPES             *
 *****************************************************/

/* Reads the list of volumes from sysfs */
static int _readVolumesList(char *bdev_path, char **labels, size_t *nr_vols)
{
	char bdev_path_noslash[SFLC_BDEV_PATH_MAX_LEN + 1];
	char openvolumes_path[SFLC_BIGBUFSIZE];
	char *str_openvolumes;

	/* Remove the slashes from the bdev_path (replace with underscores) */
	strcpy(bdev_path_noslash, bdev_path);
	sflc_str_replaceAll(bdev_path_noslash, '/', '_');
	/* Build path to sysfsy file containing open volumes list */
	sprintf(openvolumes_path, "%s/%s/%s", SFLC_SYSFS_BDEVS_DIR, bdev_path_noslash, SFLC_SYSFS_OPENVOLUMES_FILENAME);

	/* Read the sysfs file */
	str_openvolumes = sflc_readFile(openvolumes_path);
	if (!str_openvolumes) {
		sflc_log_error("Could not read file %s", openvolumes_path);
		return EBADF;
	}

	/* Parse the number of volumes */
	char *endptr;
	*nr_vols = strtoul(str_openvolumes, &endptr, 10);
	/* Skip past the number of volumes (lands on a whitespace before the first label) */
	str_openvolumes = endptr;

	/* Just to be sure */
	if (*nr_vols > SFLC_DEV_MAX_VOLUMES) {
		sflc_log_error("Something is seriously wrong, sysfs file says there are %lu open volumes, that's too many!", *nr_vols);
		return EBADF;
	}

	/* Read labels */
	size_t i;
	for (i = 0; i < *nr_vols; i++) {
		/* Trust the content of the sysfs file */
		if (sscanf(str_openvolumes, " %s", labels[i]) != 1) {
			sflc_log_error("Could not read volume label %lu. Sysfs content:\n%s", i, str_openvolumes);
			return EBADF;
		}
		sflc_log_debug("Label %lu to close: %s", i, labels[i]);

		/* Skip past the whitespace and the label */
		str_openvolumes += 1 + strlen(labels[i]);
	}

	return 0;
}


/* Close them all */
static int _closeVolumes(char **labels, size_t nr_vols)
{
	int err;

	/* Eazy peazy */
	size_t i;
	for (i = 0; i < nr_vols; i++) {
		err = sflc_act_closeVolume(labels[i]);
		if (err) {
			sflc_log_error("Could not close volume %s; error %d", labels[i], err);
			return err;
		}
		sflc_log_debug("Closed volume %s", labels[i]);
		printf("Closed volume /dev/mapper/%s\n", labels[i]);
	}

	return 0;
}

