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

/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/

#include "sysfs.h"
#include "utils/string.h"
#include "log/log.h"

/*****************************************************
 *                    CONSTANTS                      *
 *****************************************************/

#define SFLC_SYSFS_VOL_NR_SLICES_ATTR_NAME mapped_slices

/*****************************************************
 *           PRIVATE FUNCTIONS PROTOTYPES            *
 *****************************************************/

static void sflc_sysfs_volDevRelease(struct device * dev);
static ssize_t sflc_sysfs_showVolNrSlices(struct device * dev, struct device_attribute * attr, char * buf);

/*****************************************************
 *           PRIVATE VARIABLES DEFINITIONS           *
 *****************************************************/

/* Attribute showing the number of slices utilised by the volume */
static const struct device_attribute sflc_sysfs_volNrSlicesAttr = __ATTR(
	SFLC_SYSFS_VOL_NR_SLICES_ATTR_NAME,
	0444,
	sflc_sysfs_showVolNrSlices,
	NULL
);

/*****************************************************
 *           PUBLIC FUNCTIONS DEFINITIONS            *
 *****************************************************/

/* Creates and registers a VolumeDevice instance. Returns ERR_PTR() on error. */
sflc_sysfs_VolumeDevice * sflc_sysfs_volDevCreateAndAdd(sflc_Volume * vol)
{
	sflc_sysfs_VolumeDevice * kdev;
	int err;

	/* Allocate device */
	kdev = kzalloc(sizeof(sflc_sysfs_VolumeDevice), GFP_KERNEL);
	if (!kdev) {
		err = -ENOMEM;
		pr_err("Could not allocate VolumeDevice\n");
		goto err_kdev_alloc;
	}

	/* Set volume */
	kdev->vol = vol;

	/* Set name */
	err = dev_set_name(&kdev->dev, "%s", vol->vol_name);
	if (err) {
		pr_err("Could not set device name %s; error %d\n", vol->vol_name, err);
		goto err_dev_set_name;
	}

	/* Set destructor */
	kdev->dev.release = sflc_sysfs_volDevRelease;
	/* Set parent */
	kdev->dev.parent = sflc_sysfs_rootDevice;

	/* Register */
	err = device_register(&kdev->dev);
	if (err) {
		pr_err("Could not register volume %s; error %d\n", vol->vol_name, err);
		goto err_dev_register;
	}

	/* Add mapped_slices attribute */
	err = device_create_file(&kdev->dev, &sflc_sysfs_volNrSlicesAttr);
	if (err) {
		pr_err("Could not create mapped_slices device file; error %d\n", err);
		goto err_dev_create_file;
	}

	return kdev;


err_dev_create_file:
	device_unregister(&kdev->dev);
err_dev_register:
	put_device(&kdev->dev);
err_dev_set_name:
	kfree(kdev);
err_kdev_alloc:
	return ERR_PTR(err);
}

/* Releases a reference to a DeviceKobject instance */
void sflc_sysfs_putVolDev(sflc_sysfs_VolumeDevice * kdev)
{
	device_unregister(&kdev->dev);
}

/*****************************************************
 *           PRIVATE FUNCTIONS PROTOTYPES            *
 *****************************************************/

static ssize_t sflc_sysfs_showVolNrSlices(struct device * dev, struct device_attribute * attr, char * buf)
{
	sflc_sysfs_VolumeDevice * kdev = container_of(dev, sflc_sysfs_VolumeDevice, dev);
	sflc_Volume * vol = kdev->vol;

	return sprintf(buf, "%u\n", vol->mapped_slices);
}

static void sflc_sysfs_volDevRelease(struct device * dev)
{
	sflc_sysfs_VolumeDevice * kdev;

	/* Cast */
	kdev = container_of(dev, sflc_sysfs_VolumeDevice, dev);

	/* Just free */
	kfree(kdev);

	return;
}
