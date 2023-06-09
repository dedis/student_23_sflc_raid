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
#include "utils/log.h"


/*****************************************************
 *                     CONSTANTS                     *
 *****************************************************/

/* The device is randomised in chunks of 1024 blocks (arbitrary number) */
#define SFLC_BLOCKS_IN_RAND_CHUNK	1024
/* That's 4 MiB */
#define SFLC_RAND_CHUNK_SIZE (SFLC_BLOCKS_IN_RAND_CHUNK * SFLC_SECTOR_SIZE)


/*****************************************************
 *          PRIVATE FUNCTIONS PROTOTYPES             *
 *****************************************************/

/* Fill the device with random data */
static int _fillWithRandom(char *bdev_path, uint64_t dev_size);


/*****************************************************
 *          PUBLIC FUNCTIONS DEFINITIONS             *
 *****************************************************/

/**
 *  Create N volumes (only formats the device header, does not open the volumes).
 *  Creates them in order from 0 to N-1, so as to induce a back-linked list on the device.
 *
 *  @param args->bdev_path The path to the underlying block device
 *  @param args->nr_vols The number of volumes to create
 *  @param args->pwds The array of passwords for the various volumes
 *  @param args->pwd_lens The length of each password
 *  @param args->no_randfill A boolean switch indicating that the volume should not
 *   be filled entirely with random data prior to formatting.
 *
 *  @return Error code, 0 on success
 */
int sflc_cmd_initVolumes(sflc_cmd_InitArgs *args)
{
	int64_t dev_size;
	size_t nr_slices;
	int err;

	/* Sanity check */
	if (args->nr_vols > SFLC_DEV_MAX_VOLUMES) {
		sflc_log_error("Cannot create %lu volumes on a single device", args->nr_vols);
		return EINVAL;
	}

	/* Get device info */
	dev_size = sflc_disk_getSize(args->bdev_path);
	if (dev_size < 0) {
		err = -dev_size;
		sflc_log_error("Could not get device size for %s; error %d", args->bdev_path, err);
		return err;
	}
	/* Convert to number of slices */
	nr_slices = sflc_disk_maxSlices(dev_size);
	sflc_log_debug("Device %s has %ld blocks, corresponding to %lu logical slices", args->bdev_path, dev_size, nr_slices);

	/* Fill with random, if needed */
	if (!args->no_randfill) {
		err = _fillWithRandom(args->bdev_path, (uint64_t) dev_size);
		if (err) {
			sflc_log_error("Could not fill device %s with random bytes; error %d", args->bdev_path, err);
			return err;
		}
	}

	/* Common fields for all the volumes */
	sflc_Volume vol;
	vol.bdev_path = args->bdev_path;
	vol.nr_slices = nr_slices;
	/* Create each of the volumes */
	size_t i;
	for (i = 0; i < args->nr_vols; i++) {
		/* This volume's password */
		vol.pwd = args->pwds[i];
		vol.pwd_len = args->pwd_lens[i];
		/* This volume's index */
		vol.vol_idx = i;
		/* Copy VMB key over from previous iteration (reads garbage on first iteration, but it's unused) */
		memcpy(vol.prev_vmb_key, vol.vmb_key, SFLC_CRYPTO_KEYLEN);

		/* Create volume */
		err = sflc_act_createVolume(&vol);
		if (err) {
			sflc_log_error("Error creating volume %lu on device %s; error %d", i, args->bdev_path, err);
			return err;
		}
	}
	printf("Created %lu volumes on device %s\n", args->nr_vols, args->bdev_path);

	return 0;
}

// REDUNDANCY MITIGATION

int sflc_cmd_redinitVolumes(sflc_cmd_InitArgs *args)
{
	int64_t dev_size;
	size_t nr_slices;
	int err;

	/* Sanity check */
	if (args->nr_vols-1 > (SFLC_DEV_MAX_VOLUMES-1)/2) {
		sflc_log_error("Cannot create %lu volumes on a single device", args->nr_vols);
		return EINVAL;
	}

	/* Get device info */
	dev_size = sflc_disk_getSize(args->bdev_path);
	if (dev_size < 0) {
		err = -dev_size;
		sflc_log_error("Could not get device size for %s; error %d", args->bdev_path, err);
		return err;
	}
	/* Convert to number of slices */
	nr_slices = sflc_disk_maxSlices(dev_size);
	sflc_log_debug("Device %s has %ld blocks, corresponding to %lu logical slices", args->bdev_path, dev_size, nr_slices);

	/* Fill with random, if needed */
	if (!args->no_randfill) {
		err = _fillWithRandom(args->bdev_path, (uint64_t) dev_size);
		if (err) {
			sflc_log_error("Could not fill device %s with random bytes; error %d", args->bdev_path, err);
			return err;
		}
	}

	/* Common fields for all the volumes */
	sflc_Volume vol;
	vol.bdev_path = args->bdev_path;
	vol.nr_slices = nr_slices;
	/* Creates the single decoy volume */
	/* This volume's password */
	vol.pwd = args->pwds[0];
	vol.pwd_len = args->pwd_lens[0];
	/* This volume's index */
	vol.vol_idx = 0;
	/* Copy VMB key over from previous iteration (reads garbage on first iteration, but it's unused) */
	memcpy(vol.prev_vmb_key, vol.vmb_key, SFLC_CRYPTO_KEYLEN);
	/* Create volume */
	err = sflc_act_createVolume(&vol);
	if (err) {
		sflc_log_error("Error creating decoy volume on device %s; error %d", args->bdev_path, err);
		return err;
	}
	/* Create each of the redundant hidden volumes */
	size_t i;
	size_t j;
	for (i = 1; i < args->nr_vols; i++) {
		for (j=0; j<2; j++) {
			/* This volume's password */
			vol.pwd = args->pwds[i];
			vol.pwd_len = args->pwd_lens[i];
			/* This volume's index */
			vol.vol_idx = i+j;
			/* Copy VMB key over from previous iteration (reads garbage on first iteration, but it's unused) */
			memcpy(vol.prev_vmb_key, vol.vmb_key, SFLC_CRYPTO_KEYLEN);
			/* Create volume */
			err = sflc_act_createVolume(&vol);
			if (err) {
				sflc_log_error("Error creating redundant hidden volume %lu on device %s; error %d", i, args->bdev_path, err);
				return err;
			}
		}
	}
	printf("Created %lu volumes on device %s\n", args->nr_vols, args->bdev_path);

	return 0;
}

/*****************************************************
 *          PRIVATE FUNCTIONS PROTOTYPES             *
 *****************************************************/

/* Fill the device with random data */
static int _fillWithRandom(char *bdev_path, uint64_t dev_size)
{
	char *rand_chunk;
	int err;

	/* Allocate chunk */
	rand_chunk = malloc(SFLC_RAND_CHUNK_SIZE);
	if (!rand_chunk) {
		sflc_log_error("Could not allocate %d bytes for chunk of random data", SFLC_RAND_CHUNK_SIZE);
		return ENOMEM;
	}

	/* Loop to write random data in chunks */
	uint64_t blocks_remaining = dev_size;
	uint64_t sector = 0;
	while (blocks_remaining > 0) {
		uint64_t blocks_to_write =
				(blocks_remaining > SFLC_BLOCKS_IN_RAND_CHUNK) ? SFLC_BLOCKS_IN_RAND_CHUNK : blocks_remaining;
		uint64_t bytes_to_write = blocks_to_write * SFLC_SECTOR_SIZE;

		/* Sample random bytes */
		err = sflc_rand_getWeakBytes(rand_chunk, bytes_to_write);
		if (err) {
			sflc_log_error("Could not sample %lu random bytes to write on disk; error %d", bytes_to_write, err);
			goto out;
		}

		/* Write on disk */
		err = sflc_disk_writeManySectors(bdev_path, sector, rand_chunk, blocks_to_write);
		if (err) {
			sflc_log_error("Could not write random bytes on disk; error %d", err);
			goto out;
		}

		/* Advance loop */
		sector += blocks_to_write;
		blocks_remaining -= blocks_to_write;
	}

	/* No prob */
	err = 0;


out:
	free(rand_chunk);
	return err;
}

