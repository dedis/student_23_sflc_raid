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
 * Sysfs functions
 */

#ifndef _SFLC_SYSFS_SYSFS_H_
#define _SFLC_SYSFS_SYSFS_H_

/*****************************************************
 *             TYPES FORWARD DECLARATIONS            *
 *****************************************************/

/* Necessary since device.h and sysfs.h include each other */

typedef struct sflc_sysfs_device_kobject_s sflc_sysfs_DeviceKobject;
typedef struct sflc_sysfs_volume_device_s sflc_sysfs_VolumeDevice;

/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/

#include <linux/sysfs.h>
#include <linux/device.h>

#include "device/device.h"
#include "volume/volume.h"

/*****************************************************
 *                       TYPES                       *
 *****************************************************/

/* This embeds a struct kobject */
struct sflc_sysfs_device_kobject_s
{
	/* The Device it represents */
	sflc_Device    * dev;
        /* The name of this device's subdirectoy under realdevs */
        char          * dirname;

	/* The embedded kobject */
        struct kobject  kobj;
};

/* This embeds a struct device */
struct sflc_sysfs_volume_device_s
{
        /* The Volume it represents */
        sflc_Volume   * vol;

        /* The embedded device */
        struct device   dev;
};

/*****************************************************
 *           PUBLIC VARIABLES DECLARATIONS           *
 *****************************************************/

/* Kobject associated to the /sys/module/dm_sflc/realdevs entry */
extern struct kobject * sflc_sysfs_realdevsKobj;

/* Root device that will be every volume's parent.
   A bit overkill to have a sflc root device, but it does the trick. */
extern struct device * sflc_sysfs_rootDevice;

/*****************************************************
 *            PUBLIC FUNCTIONS PROTOTYPES            *
 *****************************************************/

/* Called on module load/unload */
int sflc_sysfs_init(void);
void sflc_sysfs_exit(void);


/* Device-related functions */

/* Creates and registers a DeviceKobject instance. Returns ERR_PTR() on error. */
sflc_sysfs_DeviceKobject * sflc_sysfs_devKobjCreateAndAdd(sflc_Device * dev);

/* Releases a reference to a DeviceKobject instance */
void sflc_sysfs_putDevKobj(sflc_sysfs_DeviceKobject * dev_kobj);

/* Creates a symlink inside the device's subdirectory pointing to the volume's subdirectory */
int sflc_sysfs_addVolumeToDevice(sflc_sysfs_DeviceKobject * dev_kobj, sflc_sysfs_VolumeDevice * kdev);

/* Removes the symlink created before */
void sflc_sysfs_removeVolumeFromDevice(sflc_sysfs_DeviceKobject * dev_kobj, sflc_sysfs_VolumeDevice * kdev);


/* Volume-related functions */

/* Creates and registers a VolumeDevice instance. Returns ERR_PTR() on error. */
sflc_sysfs_VolumeDevice * sflc_sysfs_volDevCreateAndAdd(sflc_Volume * vol);

/* Releases a reference to a DeviceKobject instance */
void sflc_sysfs_putVolDev(sflc_sysfs_VolumeDevice * kdev);


#endif /* _SFLC_SYSFS_SYSFS_H_ */
