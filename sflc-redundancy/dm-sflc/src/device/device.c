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
 * This file only implements the device-related device management functions.
 */

/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/

#include "device.h"
#include "sysfs/sysfs.h"
#include "log/log.h"

/*****************************************************
 *                     CONSTANTS                     *
 *****************************************************/

/*****************************************************
 *           PUBLIC VARIABLES DEFINITIONS            *
 *****************************************************/

LIST_HEAD(sflc_dev_list);
DEFINE_SEMAPHORE(sflc_dev_mutex);

/*****************************************************
 *           PUBLIC FUNCTIONS DEFINITIONS            *
 *****************************************************/

/* Creates Device and adds it to the list. Returns an ERR_PTR() if unsuccessful. */
sflc_Device * sflc_dev_getDevice(struct dm_target * ti, char * real_dev_path, u32 tot_slices)
{
	sflc_Device * dev;
	int err;
	int i;

	pr_debug("Called to create sflc_Device on %s\n", real_dev_path);

	/* Allocate device */
	dev = kzalloc(sizeof(sflc_Device), GFP_KERNEL);
	if (!dev) {
		pr_err("Could not allocate %lu bytes for sflc_Device\n", sizeof(sflc_Device));
		err = -ENOMEM;
		goto err_alloc_dev;
	}

	/* Init list node here, so it's always safe to list_del() */
	INIT_LIST_HEAD(&dev->list_node);

	/* Set backing real device */
	err = dm_get_device(ti, real_dev_path, dm_table_get_mode(ti->table), &dev->real_dev);
	if (err) {
		pr_err("Could not dm_get_device: error %d\n", err);
		goto err_dm_get_dev;
	}
	/* And its path */
	dev->real_dev_path = kmalloc(strlen(real_dev_path) + 1, GFP_KERNEL);
	if (!dev->real_dev_path) {
		pr_err("Could not allocate %lu bytes for dev->real_dev_path\n", strlen(real_dev_path) + 1);
		err = -ENOMEM;
		goto err_alloc_real_dev_path;
	}
	strcpy(dev->real_dev_path, real_dev_path);

	/* Init volumes */
	for (i = 0; i < SFLC_DEV_MAX_VOLUMES; ++i) {
		dev->vol[i] = NULL;
	}
	dev->vol_cnt = 0;

	/* Init rmap lock */
	mutex_init(&dev->rmap_lock);
	/* Allocate reverse slice map */
	dev->tot_slices = tot_slices;
	dev->rmap = vmalloc(dev->tot_slices * sizeof(u8));
	if (!dev->rmap) {
		pr_err("Could not allocate reverse slice map\n");
		err = -ENOMEM;
		goto err_alloc_rmap;
	}
	/* Initialise it */
	memset(dev->rmap, SFLC_DEV_RMAP_INVALID_VOL, dev->tot_slices * sizeof(u8));
	dev->free_slices = dev->tot_slices;

	/* Init IV cache lock */
	mutex_init(&dev->iv_cache_lock);
	/* Init IV cache waitqueue */
	init_waitqueue_head(&dev->iv_cache_waitqueue);
	/* Allocate IV cache */
	dev->iv_cache = kzalloc(dev->tot_slices * sizeof(sflc_dev_IvCacheEntry *), GFP_KERNEL);
	if (!dev->iv_cache) {
		pr_err("Could not allocate IV cache\n");
		err = -ENOMEM;
		goto err_alloc_iv_cache;
	}
	/* Set it empty */
	dev->iv_cache_nr_entries = 0;
	/* Init list head */
	INIT_LIST_HEAD(&dev->iv_lru_list);

	/* Create kobject */
	dev->kobj = sflc_sysfs_devKobjCreateAndAdd(dev);
	if (IS_ERR(dev->kobj)) {
		err = PTR_ERR(dev->kobj);
		pr_err("Could not create kobject; error %d\n", err);
		goto err_sysfs;
	}

	/* Add to device list */
	list_add_tail(&dev->list_node, &sflc_dev_list);

	return dev;


err_sysfs:
	kfree(dev->iv_cache);
err_alloc_iv_cache:
	vfree(dev->rmap);
err_alloc_rmap:
	kfree(dev->real_dev_path);
err_alloc_real_dev_path:
	dm_put_device(ti, dev->real_dev);
err_dm_get_dev:
	kfree(dev);
err_alloc_dev:
	return ERR_PTR(err);
}

/* Returns NULL if not found */
sflc_Device * sflc_dev_lookupByPath(char * real_dev_path)
{
	sflc_Device * dev;

	/* Sweep the list of devices */
	list_for_each_entry(dev, &sflc_dev_list, list_node) {
		if (strcmp(real_dev_path, dev->real_dev_path) == 0) {
			return dev;
		}
	}

	return NULL;
}

/* Returns false if still busy (not all volumes have been removed). Frees the Device. */
bool sflc_dev_putDevice(struct dm_target * ti, sflc_Device * dev) 
{
	/* Check if we actually have to put this device */
	if (!dev) {
		return false;
	}
	if (dev->vol_cnt > 0) {
		pr_warn("Called while still holding %d volumes\n", dev->vol_cnt);
		return false;
	}

	/* Flush all IVs */
	sflc_dev_flushIvs(dev);

	/* List */
	list_del(&dev->list_node);

	/* Sysfs */
	sflc_sysfs_putDevKobj(dev->kobj);

	/* IV cache */
	kfree(dev->iv_cache);

	/* Reverse slice map */
	vfree(dev->rmap);

	/* Backing device */
	dm_put_device(ti, dev->real_dev);
	kfree(dev->real_dev_path);

	/* Nothing to do with the volumes */

	/* Free the device itself */
	kfree(dev);

	return true;
}
