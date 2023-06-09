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
 * This file only implements the volume-related device management functions.
 */

/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/

#include "device.h"
#include "log/log.h"

/*****************************************************
 *                     CONSTANTS                     *
 *****************************************************/

/*****************************************************
 *           PUBLIC FUNCTIONS DEFINITIONS            *
 *****************************************************/

/* Returns false if volume index was already occupied. */
bool sflc_dev_addVolume(sflc_Device * dev, sflc_Volume * vol, int vol_idx)
{
	int err;

	if (dev->vol[vol_idx]) {
		pr_err("Something's wrong, asked to set volume number %d, already occupied\n", vol_idx);
		return false;
	}

	/* Update sysfs */
	err = sflc_sysfs_addVolumeToDevice(dev->kobj, vol->kdev);
	if (err) {
		pr_err("Could not add volume symlink in sysfs device subdir; error %d\n", err);
		return false;
	}

	/* Update fields */
	dev->vol[vol_idx] = vol;
	dev->vol_cnt += 1;

	return true;
}

/* Looks at all volumes in all devices. Returns NULL if not found */
sflc_Volume * sflc_dev_lookupVolumeByName(char * vol_name)
{
	sflc_Device * dev;
	sflc_Volume * vol;

	/* Sweep all devices */
	list_for_each_entry(dev, &sflc_dev_list, list_node) {
		/* Sweep all volumes */
		int i;
		for (i = 0; i < SFLC_DEV_MAX_VOLUMES; ++i) {
			vol = dev->vol[i];
			if (vol && (strcmp(vol_name, vol->vol_name) == 0)) {
				return vol;
			}
		}
	}

	return NULL;
}

/* Does not put the volume. Returns false if was already NULL. */
bool sflc_dev_removeVolume(sflc_Device * dev, int vol_idx)
{
	if (!dev->vol[vol_idx]) {
		pr_err("Something's wrong, asked to unset volume number %d, already NULL\n", vol_idx);
		return false;
	}
	
	/* Remove sysfs entry */
	if (dev->vol[vol_idx]->kdev) {
		sflc_sysfs_removeVolumeFromDevice(dev->kobj, dev->vol[vol_idx]->kdev);
	}

	/* Update fields */
	dev->vol[vol_idx] = NULL;
	dev->vol_cnt -= 1;
	return true;
}
