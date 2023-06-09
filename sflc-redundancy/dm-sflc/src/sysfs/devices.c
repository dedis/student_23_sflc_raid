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

#define SFLC_SYSFS_DEV_VOLUMES_ATTR_NAME "volumes"
#define SFLC_SYSFS_DEV_TOT_SLICES_ATTR_NAME "tot_slices"
#define SFLC_SYSFS_DEV_FREE_SLICES_ATTR_NAME "free_slices"

/*****************************************************
 *           PRIVATE FUNCTIONS PROTOTYPES            *
 *****************************************************/

/* Functions that will go in the sysfs_ops */
static ssize_t sflc_sysfs_devShow(struct kobject * kobj, struct attribute * attr, char * buf);
static ssize_t sflc_sysfs_devStore(struct kobject * kobj, struct attribute * attr, const char * buf, size_t len);

/* Concrete file showers */
static ssize_t sflc_sysfs_showDeviceVolumes(struct kobject * kobj, struct attribute * attr, char * buf);
static ssize_t sflc_sysfs_showDeviceTotSlices(struct kobject * kobj, struct attribute * attr, char * buf);
static ssize_t sflc_sysfs_showDeviceFreeSlices(struct kobject * kobj, struct attribute * attr, char * buf);

/* Release function for the DeviceKobject */
static void sflc_sysfs_releaseDevKobj(struct kobject * kobj);

/*****************************************************
 *           PRIVATE VARIABLES DEFINITIONS           *
 *****************************************************/

/* The attribute representing the volumes file */
static const struct attribute sflc_sysfs_devVolumesAttr = {
	.name = SFLC_SYSFS_DEV_VOLUMES_ATTR_NAME,
	.mode = 0444
};

/* The attribute representing the tot_slices file */
static const struct attribute sflc_sysfs_devTotSlicesAttr = {
	.name = SFLC_SYSFS_DEV_TOT_SLICES_ATTR_NAME,
	.mode = 0444
};

/* The attribute representing the free_slices file */
static const struct attribute sflc_sysfs_devFreeSlicesAttr = {
	.name = SFLC_SYSFS_DEV_FREE_SLICES_ATTR_NAME,
	.mode = 0444
};

/* The sysfs_ops struct encapsulating the access methods */
static const struct sysfs_ops sflc_sysfs_devKobjSysfsOps = {
	.show = sflc_sysfs_devShow,
	.store = sflc_sysfs_devStore
};

/* The type for our DeviceKobject */
static struct kobj_type sflc_sysfs_devKobjType = {
	.release = sflc_sysfs_releaseDevKobj,
	.sysfs_ops = &sflc_sysfs_devKobjSysfsOps,
};

/*****************************************************
 *           PUBLIC FUNCTIONS DEFINITIONS            *
 *****************************************************/

/* Creates and registers a DeviceKobject instance. Return ERR_PTR() on error. */
sflc_sysfs_DeviceKobject * sflc_sysfs_devKobjCreateAndAdd(sflc_Device * dev)
{
	sflc_sysfs_DeviceKobject * dev_kobj;
	int err;

	/* Allocate outer structure */
	dev_kobj = kzalloc(sizeof(sflc_sysfs_DeviceKobject), GFP_KERNEL);
	if (!dev_kobj) {
		err = -ENOMEM;
		pr_err("Could not allocate DeviceKobject\n");
		goto err_dev_kobj_alloc;
	}

	/* Set device */
	dev_kobj->dev = dev;

	/* Allocate inner string */
	dev_kobj->dirname = kzalloc(strlen(dev->real_dev_path) + 1, GFP_KERNEL);
	if (!dev_kobj->dirname) {
		err = -ENOMEM;
		pr_err("Could not allocate dirname\n");
		goto err_dirname_alloc;
	}

	/* Copy it and substitute slashes with underscores */
	strcpy(dev_kobj->dirname, dev->real_dev_path);
	sflc_str_replaceAll(dev_kobj->dirname, '/', '_');

	/* Init and add kobject */
	err = kobject_init_and_add(&dev_kobj->kobj, &sflc_sysfs_devKobjType, 
					sflc_sysfs_realdevsKobj,	dev_kobj->dirname);
	if (err) {
		pr_err("Could not init and add internal kobject; error %d\n", err);
		goto err_kobj_init_add;
	}

	/* Create the volumes file */
	err = sysfs_create_file(&dev_kobj->kobj, &sflc_sysfs_devVolumesAttr);
	if (err) {
		pr_err("Could not add volumes file; error %d\n", err);
		goto err_vol_file;
	}
	/* Create the tot_slices file */
	err = sysfs_create_file(&dev_kobj->kobj, &sflc_sysfs_devTotSlicesAttr);
	if (err) {
		pr_err("Could not add tot_slices file; error %d\n", err);
		goto err_tot_slices_file;
	}
	/* Create the volumes file */
	err = sysfs_create_file(&dev_kobj->kobj, &sflc_sysfs_devFreeSlicesAttr);
	if (err) {
		pr_err("Could not add free_slices file; error %d\n", err);
		goto err_free_slices_file;
	}

	return dev_kobj;


err_free_slices_file:
err_tot_slices_file:
err_vol_file:
	kobject_put(&dev_kobj->kobj);
err_kobj_init_add:
	kfree(dev_kobj->dirname);
err_dirname_alloc:
	kfree(dev_kobj);
err_dev_kobj_alloc:
	return ERR_PTR(err);
}

/* Releases a reference to a DeviceKobject instance */
void sflc_sysfs_putDevKobj(sflc_sysfs_DeviceKobject * dev_kobj)
{
	kobject_put(&dev_kobj->kobj);
}

/* Creates a symlink inside the device's subdirectory pointing to the volume's subdirectory */
int sflc_sysfs_addVolumeToDevice(sflc_sysfs_DeviceKobject * dev_kobj, sflc_sysfs_VolumeDevice * kdev)
{
	return sysfs_create_link(&dev_kobj->kobj, &kdev->dev.kobj, kdev->dev.kobj.name);
}

/* Removes the symlink created before */
void sflc_sysfs_removeVolumeFromDevice(sflc_sysfs_DeviceKobject * dev_kobj, sflc_sysfs_VolumeDevice * kdev)
{
	sysfs_remove_link(&dev_kobj->kobj, kdev->dev.kobj.name);
}

/*****************************************************
 *           PRIVATE FUNCTIONS PROTOTYPES            *
 *****************************************************/

/* Dispatch to the right shower */
static ssize_t sflc_sysfs_devShow(struct kobject * kobj, struct attribute * attr, char * buf)
{
	/* Dispatch based on name */
	if (strcmp(attr->name, SFLC_SYSFS_DEV_VOLUMES_ATTR_NAME) == 0) {
		return sflc_sysfs_showDeviceVolumes(kobj, attr, buf);
	}
	if (strcmp(attr->name, SFLC_SYSFS_DEV_TOT_SLICES_ATTR_NAME) == 0) {
		return sflc_sysfs_showDeviceTotSlices(kobj, attr, buf);
	}
	if (strcmp(attr->name, SFLC_SYSFS_DEV_FREE_SLICES_ATTR_NAME) == 0) {
		return sflc_sysfs_showDeviceFreeSlices(kobj, attr, buf);
	}
	
	/* Else, error */
	pr_err("Error, unknown attribute %s\n", attr->name);
	return -EIO;
}

/* Do nothing */
static ssize_t sflc_sysfs_devStore(struct kobject * kobj, struct attribute * attr, const char * buf, size_t len)
{
	return -EIO;
}

/* Show the list of mounted volumes */
static ssize_t sflc_sysfs_showDeviceVolumes(struct kobject * kobj, struct attribute * attr, char * buf)
{
	sflc_sysfs_DeviceKobject * dev_kobj;
	sflc_Device * dev;
	ssize_t ret;

	/* Cast to a DeviceKobject */
	dev_kobj = container_of(kobj, sflc_sysfs_DeviceKobject, kobj);
	/* Get the device */
	dev = dev_kobj->dev;

	/* File contents */
	ret = 0;
	ssize_t written;
	/* Write the volume count */
	written = sprintf(buf, "%d", dev->vol_cnt);
	ret += written;
	buf += written;
	/* And all the volume names, space-separated */
	int i;
	for (i = 0; i < dev->vol_cnt; i++) {
		written = sprintf(buf, " %s", dev->vol[i]->vol_name);
		ret += written;
		buf += written;
	}
	/* Add newline */
	written = sprintf(buf, "\n");
	ret += written;
	buf += written;

	return ret;
}

/* Show the number of slices */
static ssize_t sflc_sysfs_showDeviceTotSlices(struct kobject * kobj, struct attribute * attr, char * buf)
{
	sflc_sysfs_DeviceKobject * dev_kobj;
	sflc_Device * dev;
	ssize_t ret;

	/* Cast to a DeviceKobject */
	dev_kobj = container_of(kobj, sflc_sysfs_DeviceKobject, kobj);
	/* Get the device */
	dev = dev_kobj->dev;

	/* Write the tot_slices */
	ret = sprintf(buf, "%u\n", dev->tot_slices);

	return ret;
}

/* Show the number of free slices */
static ssize_t sflc_sysfs_showDeviceFreeSlices(struct kobject * kobj, struct attribute * attr, char * buf)
{
	sflc_sysfs_DeviceKobject * dev_kobj;
	sflc_Device * dev;
	ssize_t ret;

	/* Cast to a DeviceKobject */
	dev_kobj = container_of(kobj, sflc_sysfs_DeviceKobject, kobj);
	/* Get the device */
	dev = dev_kobj->dev;

	/* Write the free_slices */
	ret = sprintf(buf, "%u\n", dev->free_slices);

	return ret;
}

/* Release function for the DeviceKobject */
static void sflc_sysfs_releaseDevKobj(struct kobject * kobj)
{
	sflc_sysfs_DeviceKobject * dev_kobj;

	/* Cast to a DeviceKobject */
	dev_kobj = container_of(kobj, sflc_sysfs_DeviceKobject, kobj);

	/* Free everything */
	kfree(dev_kobj->dirname);
	kfree(dev_kobj);

	return;
}
