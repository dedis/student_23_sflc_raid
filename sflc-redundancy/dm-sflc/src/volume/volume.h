/*
 *  Copyright The Shufflecake Project Authors (2022)
 *  Copyright The Shufflecake Project Contributors (2022)
 *  Copyright Contributors to the The Shufflecake Project.
 *  
 *  See the AUTHORS file at the top-level directory of this distribution and at
 *  <https://www.shufflecake.net/permalinks/shufflecake-userland/AUTHORS>
 *  
 *  This file is part of the program dm-sflc, which is part of the Shufflecake 
 *  Project. Shufflecake is a plausible deniability (hidden storage) layer for 
 *  Linux. See <https://www.shufflecake.net>.
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
 
/*
 * A volume represents a single "virtual" block device, mapping onto
 * a "real" device represented by a device.
 */

#ifndef _SFLC_VOLUME_VOLUME_H_
#define _SFLC_VOLUME_VOLUME_H_

/*****************************************************
 *             TYPES FORWARD DECLARATIONS            *
 *****************************************************/

/* Necessary since device.h and volume.h include each other */

typedef struct sflc_vol_write_work_s sflc_vol_WriteWork;
typedef struct sflc_vol_decrypt_work_s sflc_vol_DecryptWork;
typedef struct sflc_volume_s sflc_Volume;

/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/

#include <linux/blk_types.h>

#include "device/device.h"
#include "crypto/symkey/symkey.h"
#include "sysfs/sysfs.h"

/*****************************************************
 *                     CONSTANTS                     *
 *****************************************************/

/* One sector for the userland tool, 1024 for the position (forward) map, 4 for the relative IVs */
#define SFLC_VOL_HEADER_IV_BLOCKS 4
#define SFLC_VOL_HEADER_DATA_BLOCKS (SFLC_VOL_HEADER_IV_BLOCKS * SFLC_DEV_SECTOR_TO_IV_RATIO)
#define SFLC_VOL_HEADER_SIZE (1 + SFLC_VOL_HEADER_DATA_BLOCKS + SFLC_VOL_HEADER_IV_BLOCKS)	// In 4096-byte sectors

/* A single header data block contains 1024 fmap mappings */
#define SFLC_VOL_HEADER_MAPPINGS_PER_BLOCK (SFLC_DEV_SECTOR_SIZE / sizeof(u32))

/* We split the volume's logical addressing space into 1 MB slices */
#define SFLC_VOL_LOG_SLICE_SIZE 256	// In 4096-byte sectors

/* Value marking an LSI as unassigned */
#define SFLC_VOL_FMAP_INVALID_PSI 0xFFFFFFFFU

/*****************************************************
 *                       TYPES                       *
 *****************************************************/

struct sflc_vol_write_work_s
{
	/* Essential information */
        sflc_Volume            * vol;
        struct bio            * orig_bio;

	/* Write requests need to allocate own page */
	struct page	      * page;

	/* Will be submitted to workqueue */
        struct work_struct      work;
};

struct sflc_vol_decrypt_work_s
{
	/* Essential information */
        sflc_Volume            * vol;
        struct bio            * orig_bio;
	struct bio	      * phys_bio;

	/* IV retrieval information */
	u32 			psi;
	u32			off_in_slice;

	/* Will be submitted to workqueue */
        struct work_struct      work;
};

struct sflc_volume_s
{
	/* Shufflecake-unique name for this instance */
	char           		      *	vol_name;

	/* Backing device */
	sflc_Device    		      * dev;
	/* Index of this volume within the device's volume array */
	int				vol_idx;

	/* Forward position map */
	struct mutex			fmap_lock;
	u32	        	      *	fmap;
	/* Stats on the fmap */
	u32				mapped_slices;

	/* Sysfs stuff */
	sflc_sysfs_VolumeDevice	      * kdev;

	/* Crypto */
	sflc_sk_Context  	      *	skctx;
};

/*****************************************************
 *           PUBLIC VARIABLES DECLARATIONS           *
 *****************************************************/

/*****************************************************
 *            PUBLIC FUNCTIONS PROTOTYPES            *
 *****************************************************/

/* Creates volume and adds it to the device. Returns an ERR_PTR() if unsuccessful */
sflc_Volume * sflc_vol_getVolume(struct dm_target * ti, char * vol_name, sflc_Device* dev, 
				int vol_idx, u8 * enckey, bool vol_creation);
/* Removes the volume from the device and frees it. */
void sflc_vol_putVolume(struct dm_target * ti, sflc_Volume * vol);

/* Remaps the underlying block device and the sector number */
int sflc_vol_remapBioFast(sflc_Volume * vol, struct bio * bio);
/* Processes the bio in the normal indirection+crypto way */
int sflc_vol_processBio(sflc_Volume * vol, struct bio * bio);

/* Executed in top half */
void sflc_vol_doRead(sflc_Volume * vol, struct bio * bio);
/* Executed in bottom half */
void sflc_vol_doWrite(struct work_struct * work);

/* Maps a logical 512-byte sector to a physical 512-byte sector. Returns < 0 if error.
 * Specifically, if op == READ, and the logical slice is unmapped, -ENXIO is returned. */
s64 sflc_vol_remapSector(sflc_Volume * vol, sector_t log_sector, int op, u32 * psi_out, u32 * off_in_slice_out);
/* Loads (and decrypts) the position map from the volume's header */
int sflc_vol_loadFmap(sflc_Volume * vol);
/* Stores (and encrypts) the position map to the volume's header */
int sflc_vol_storeFmap(sflc_Volume * vol);

s32 sflc_vol_mapSlice(sflc_Volume * vol, u32 lsi, int op); // From private to public
int sflc_vol_processBioRedundantlyAmong(sflc_Volume * vol, sflc_Volume * copy_vol, struct bio * bio);
int sflc_vol_processBioRedundantlyWithin(sflc_Volume * vol, struct bio * bio);


#endif /* _SFLC_VOLUME_VOLUME_H_ */
