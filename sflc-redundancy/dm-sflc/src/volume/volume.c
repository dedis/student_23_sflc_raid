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
 * This file only implements the volume creation and destruction functions.
 */

/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/

#include "volume.h"
#include "log/log.h"

/*****************************************************
 *                     CONSTANTS                     *
 *****************************************************/

/*****************************************************
 *           PUBLIC FUNCTIONS DEFINITIONS            *
 *****************************************************/

/* Creates volume and adds it to the device. Returns an ERR_PTR() if unsuccessful */
sflc_Volume * sflc_vol_getVolume(struct dm_target * ti, char * vol_name, sflc_Device* dev, 
				int vol_idx, u8 * enckey, bool vol_creation)
{
	sflc_Volume * vol;
	int err;

	pr_debug("Called to create sflc_Volume named \"%s\"\n", vol_name);

	/* Allocate volume */
	vol = kzalloc(sizeof(sflc_Volume), GFP_KERNEL);
	if (!vol) {
		pr_err("Could not allocate %lu bytes for sflc_Volume\n", sizeof(sflc_Volume));
		err = -ENOMEM;
		goto err_alloc_vol;
	}

	/* Set volume name */
	vol->vol_name = kmalloc(strlen(vol_name) + 1, GFP_KERNEL);
	if (!vol->vol_name) {
		pr_err("Could not allocate %lu bytes for volume name\n", strlen(vol_name) + 1);
		err = -ENOMEM;
		goto err_alloc_vol_name;
	}
	strcpy(vol->vol_name, vol_name);

	/* Sysfs stuff */
	vol->kdev = sflc_sysfs_volDevCreateAndAdd(vol);
	if (IS_ERR(vol->kdev)) {
		err = PTR_ERR(vol->kdev);
		pr_err("Could not create sysfs entry; error %d\n", err);
		goto err_sysfs;
	}

	/* Backing device */
	if (!sflc_dev_addVolume(dev, vol, vol_idx)) {
		pr_err("Could not add volume to device\n");
		err = -EINVAL;
		goto err_add_to_dev;
	}
	vol->dev = dev;
	vol->vol_idx = vol_idx;

	/* Crypto */
	vol->skctx = sflc_sk_createContext(enckey);
	if (IS_ERR(vol->skctx)) {
		err = PTR_ERR(vol->skctx);
		pr_err("Could not create crypto context\n");
		goto err_create_skctx;
	}

	/* Initialise fmap_lock */
	mutex_init(&vol->fmap_lock);
	/* Allocate forward map */
	vol->fmap = vmalloc(dev->tot_slices * sizeof(u32));
	if (!vol->fmap) {
		pr_err("Could not allocate forward map\n");
		err = -ENOMEM;
		goto err_alloc_fmap;
	}
	/* And init the stats */
	vol->mapped_slices = 0;

	/* Initialise fmap */
	if (vol_creation) {
		pr_notice("Volume creation for volume %s: starting with empty fmap\n", vol->vol_name);
		u32 i;
		for (i = 0; i < dev->tot_slices; i++) {
			vol->fmap[i] = SFLC_VOL_FMAP_INVALID_PSI;
		}
	} else {
		pr_notice("Volume opening for volume %s: loading fmap from header\n", vol->vol_name);
		err = sflc_vol_loadFmap(vol);
		if (err) {
			pr_err("Could not load position map; error %d\n", err);
			goto err_load_fmap;
		}
		pr_debug("Successfully loaded position map for volume %s\n", vol->vol_name);
	}

	return vol;


err_load_fmap:
	vfree(vol->fmap);
err_alloc_fmap:
	sflc_sk_destroyContext(vol->skctx);
err_create_skctx:
	sflc_dev_removeVolume(vol->dev, vol->vol_idx);
err_add_to_dev:
	sflc_sysfs_putVolDev(vol->kdev);
err_sysfs:
	kfree(vol->vol_name);
err_alloc_vol_name:
	kfree(vol);
err_alloc_vol:
	return ERR_PTR(err);
}

/* Removes the volume from the device and frees it. */
void sflc_vol_putVolume(struct dm_target * ti, sflc_Volume * vol)
{
	int err;

	/* Store fmap */
	pr_notice("Going to store position map of volume %s\n", vol->vol_name);
	err = sflc_vol_storeFmap(vol);
	if (err) {
		pr_err("Could not store position map; error %d\n", err);
	}
	pr_debug("Successfully stored position map of volume %s\n", vol->vol_name);
	/* Free it */
	vfree(vol->fmap);

	/* Skctx */
	sflc_sk_destroyContext(vol->skctx);

	/* Remove from device */
	sflc_dev_removeVolume(vol->dev, vol->vol_idx);

	/* Destroy sysfs entries */
	sflc_sysfs_putVolDev(vol->kdev);

	/* Free name string */
	kfree(vol->vol_name);

	/* Free volume structure */
	kfree(vol);

	return;
}
