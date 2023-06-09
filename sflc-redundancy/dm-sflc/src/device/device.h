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
 * A device represents the underlying "real" block device, common to all
 * "virtual" volumes that map onto it. It also groups other structures
 * shared between volumes on the same device.
 * Devices are kept in a list. Volumes mapping onto a device are kept
 * in an array within the device, because their index is important (they
 * are stored in increasing degree of "secrecy").
 */

#ifndef _SFLC_DEVICE_DEVICE_H_
#define _SFLC_DEVICE_DEVICE_H_

/*****************************************************
 *             TYPES FORWARD DECLARATIONS            *
 *****************************************************/

/* Necessary since device.h and volume.h include each other */

typedef struct sflc_device_s sflc_Device;
typedef struct sflc_dev_iv_cache_entry_s sflc_dev_IvCacheEntry;

/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/

#include <linux/device-mapper.h>

#include "volume/volume.h"
#include "crypto/symkey/symkey.h"
#include "sysfs/sysfs.h"

/*****************************************************
 *                     CONSTANTS                     *
 *****************************************************/

/* We need 4096-byte sectors to amortise the space overhead of the IVs */
#define SFLC_DEV_SECTOR_SIZE 4096
/* A SFLC sector encompasses 8 kernel sectors */
#define SFLC_DEV_SECTOR_SCALE (SFLC_DEV_SECTOR_SIZE / SECTOR_SIZE)
/* An IV block holds IVs for 256 data blocks */
#define SFLC_DEV_SECTOR_TO_IV_RATIO (SFLC_DEV_SECTOR_SIZE / SFLC_SK_IV_LEN)

/* Max number of volumes linked to a single device */
#define SFLC_DEV_MAX_VOLUMES 15
/* Size of the whole header section of the device */
#define SFLC_DEV_HEADER_SIZE (SFLC_DEV_MAX_VOLUMES * SFLC_VOL_HEADER_SIZE)

/* At most 1M slices, so at most ~1TB */
#define SFLC_DEV_MAX_SLICES (1024 * 1024)
/* A physical slice contains the 256 encrypted data blocks and the IV block */
#define SFLC_DEV_PHYS_SLICE_SIZE (SFLC_VOL_LOG_SLICE_SIZE + (SFLC_VOL_LOG_SLICE_SIZE / SFLC_DEV_SECTOR_TO_IV_RATIO))

/* Value marking a PSI as unassigned */
#define SFLC_DEV_RMAP_INVALID_VOL 0xFFU

/*****************************************************
 *                       TYPES                       *
 *****************************************************/

struct sflc_dev_iv_cache_entry_s
{
	/* The PSI it refers to */
	u32			psi;
	/* The actual data, containing the 4-kB IV block */
	struct page	      * iv_page;

	/* How many processes are holding it (can only be flushed when it's 0) */
	u16			refcnt;
	/* How many changes have been performed since the last flush */
	u16			dirtyness;

	/* Position in the LRU list */
	struct list_head	lru_node;
};

struct sflc_device_s
{
	/* Underlying block device */
	struct dm_dev                 * real_dev;
	char                          * real_dev_path;

	/* All volumes linked to this device */
	sflc_Volume                    * vol[SFLC_DEV_MAX_VOLUMES];
	int                             vol_cnt;

	/* Reverse slice map, associating PSIs to volume indices */
	struct mutex			rmap_lock;
	u8			      	* rmap;
	u32				tot_slices;
	u32				free_slices;

	/* LRU cache of IV blocks */
	struct mutex			iv_cache_lock;
	wait_queue_head_t		iv_cache_waitqueue;
	sflc_dev_IvCacheEntry	     ** iv_cache;
	u32				iv_cache_nr_entries;
	struct list_head		iv_lru_list;

	/* Sysfs stuff */
	sflc_sysfs_DeviceKobject	      * kobj;

	/* We keep all devices in a list */
	struct list_head                list_node;  
};

/*****************************************************
 *           PUBLIC VARIABLES DECLARATIONS           *
 *****************************************************/

/* List of all devices */
extern struct list_head sflc_dev_list;
/* Big, coarse-grained lock for all modifying operations on any device or the device list */
extern struct semaphore sflc_dev_mutex;

/*****************************************************
 *            PUBLIC FUNCTIONS PROTOTYPES            *
 *****************************************************/

/*
 * None of these functions acquire the big device lock: it must be held
 * by the caller.
 */

/* Creates Device and adds it to the list. Returns an ERR_PTR() if unsuccessful. */
sflc_Device * sflc_dev_getDevice(struct dm_target * ti, char * real_dev_path, u32 tot_slices);

/* Returns NULL if not found */
sflc_Device * sflc_dev_lookupByPath(char * real_dev_path);

/* Returns false if still busy (not all volumes have been removed) Frees the Device. */
bool sflc_dev_putDevice(struct dm_target * ti, sflc_Device * dev);


/* Returns false if volume index was already occupied. */
bool sflc_dev_addVolume(sflc_Device * dev, sflc_Volume * vol, int vol_idx);

/* Looks at all volumes in all devices. Returns NULL if not found */
sflc_Volume * sflc_dev_lookupVolumeByName(char * vol_name);

/* Does not put the volume. Returns false if was already NULL. */
bool sflc_dev_removeVolume(sflc_Device * dev, int vol_idx);


/* Synchronously reads/writes one 4096-byte sector from/to the underlying device 
   to/from the provided page */
int sflc_dev_rwSector(sflc_Device * dev, struct page * page, sector_t sector, int rw);


/* The caller needs to hold rmap_lock to call these functions */

/* Sets the PSI as owned by the given volume. Returns < 0 if already taken. */
int sflc_dev_setRmap(sflc_Device * dev, u32 psi, u8 vol_idx);

/* Sets the PSI as free. */
void sflc_dev_unsetRmap(sflc_Device * dev, u32 psi);

/* Returns a random free physical slice, or < 0 if error */
s32 sflc_dev_getRandomFreePsi(sflc_Device * dev);


/* These functions provide concurrent-safe access to the entries of the IV cache.
   The lock iv_cache_lock is acquired by these functions: it must not be held by the caller.
   The individual entries (IV blocks) are not concurrent-safe because they don't need to:
   concurrent consumers of the same block are concerned with different IVs contained in the
   block, because we exclued concurrent I/O to the same logical data block (BIG QUESTION MARK HERE).
   When the refcount reaches 0, the IV block is flushed. */

/* Get a pointer to the specified IV block. Increases the refcount and possibly the dirtyness (if WRITE). */
u8 * sflc_dev_getIvBlockRef(sflc_Device * dev, u32 psi, int rw);

/* Signal end of usage of an IV block. Decreases the refcount. */
int sflc_dev_putIvBlockRef(sflc_Device * dev, u32 psi);

/* Flush all dirty IV blocks */
void sflc_dev_flushIvs(sflc_Device * dev);


#endif /* _SFLC_DEVICE_DEVICE_H_ */
