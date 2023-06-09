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

#include <linux/module.h>

#include "sysfs.h"
#include "log/log.h"

/*****************************************************
 *                    CONSTANTS                      *
 *****************************************************/

#define SFLC_SYSFS_REAL_DEVS_ENTRY_NAME "realdevs"
#define SFLC_SYSFS_ROOT_DEVICE_NAME "sflc"

/*****************************************************
 *            PUBLIC VARIABLES DEFINITIONS           *
 *****************************************************/

/* Kobject associated to the /sys/module/dm_sflc/realdevs entry */
struct kobject * sflc_sysfs_realdevsKobj;

/* Root device that will be every volume's parent */
struct device * sflc_sysfs_rootDevice;

/*****************************************************
 *           PRIVATE FUNCTIONS PROTOTYPES            *
 *****************************************************/

/*****************************************************
 *           PUBLIC FUNCTIONS DEFINITIONS            *
 *****************************************************/

/* Called on module load */
int sflc_sysfs_init(void)
{
	int err;

	/* Create the realdevs entry under /sys/module/dm_sflc */
	sflc_sysfs_realdevsKobj = kobject_create_and_add(SFLC_SYSFS_REAL_DEVS_ENTRY_NAME, &THIS_MODULE->mkobj.kobj);
	if (!sflc_sysfs_realdevsKobj) {
		err = -ENOMEM;
		pr_err("Could not create realdevs kobject\n");
		goto err_realdevs;
	}

	/* Create the root sflc device */
	sflc_sysfs_rootDevice = root_device_register(SFLC_SYSFS_ROOT_DEVICE_NAME);
	if (IS_ERR(sflc_sysfs_rootDevice)) {
		err = PTR_ERR(sflc_sysfs_rootDevice);
		pr_err("Could not register sflc root device; error %d\n", err);
		goto err_rootdev;
	}

	return 0;

err_rootdev:
	kobject_put(sflc_sysfs_realdevsKobj);
err_realdevs:
	return err;
}

/* Called on module unload */
void sflc_sysfs_exit(void)
{
	root_device_unregister(sflc_sysfs_rootDevice);
	kobject_put(sflc_sysfs_realdevsKobj);
}

/*****************************************************
 *           PRIVATE FUNCTIONS DEFINITIONS           *
 *****************************************************/
