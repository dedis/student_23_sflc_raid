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


/**
 * Interface to the device mapper. The only example I could find is the
 * cryptsetup source, at
 * https://kernel.googlesource.com/pub/scm/utils/cryptsetup/cryptsetup/+/v1_6_3/lib/libdevmapper.c
 */

/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/

#include <libdevmapper.h>

#include "utils/dm.h"
#include "utils/log.h"


/*****************************************************
 *           PUBLIC FUNCTIONS DEFINITIONS            *
 *****************************************************/

/**
 * Create a new Shufflecake virtual device (volume) under /dev/mapper.
 *
 * @param virt_dev_name The name of the new virtual device, as it will appear
 *  under /dev/mapper
 * @param num_sectors The size of the virtual device, in 512-byte sectors
 * @param params The string containing the space-separated paramters that will
 *  be passed to the Shufflecake constructor in the kernel module
 *
 * @return The error code (0 on success)
 */
int sflc_dm_create(char * virt_dev_name, uint64_t num_sectors, char * params)
{
    struct dm_task *dmt;
    uint32_t cookie = 0;
    uint16_t udev_flags = 0;
    int err;

    /* Just to be sure, let's get them on the heap */
    char * dup_virt_dev_name = strdup(virt_dev_name);
    char * dup_params = strdup(params);

    sflc_log_debug("Creating /dev/mapper/%s", dup_virt_dev_name);

    /* Instantiate the DM task (with the CREATE ioctl command) */
    if ((dmt = dm_task_create(DM_DEVICE_CREATE)) == NULL) {
        sflc_log_error("Cannot create dm_task");
        err = 1;
        goto dup_free;
    }
    sflc_log_debug("Successfully created dm_task");

    /* Set the name of the target device (to be created) */
    if (!dm_task_set_name(dmt, dup_virt_dev_name)) {
        sflc_log_error("Cannot set device name");
        err = 2;
        goto out;
    }
    sflc_log_debug("Successfully set device name");

    /* State that it is a Shufflecake device, pass the start and size, and the
     * constructor parameters */
    if (!dm_task_add_target(dmt, 0, num_sectors, SFLC_DM_TARGET_NAME, dup_params)) {
    	sflc_log_error("Cannot add DM target and parameters");
    	err = 3;
        goto out;
    }
    sflc_log_debug("Successfully added DM target and parameters");

    /* Say that we want a new node under /dev/mapper */
    if (!dm_task_set_add_node(dmt, DM_ADD_NODE_ON_CREATE)) {
        sflc_log_error("Cannot add /dev/mapper node");
        err = 4;
        goto out;
    }
    sflc_log_debug("Successfully set the ADD_NODE flag");

    /* Get a cookie (request ID, basically) to wait for task completion */
    if (!dm_task_set_cookie(dmt, &cookie, udev_flags)) {
        sflc_log_error("Cannot get cookie");
        err = 5;
        goto out;
    }
    sflc_log_debug("Successfully got a cookie");

    /* Run the task */
    if (!dm_task_run(dmt)) {
        sflc_log_error("Cannot issue ioctl");
        err = 6;
        goto out;
    }
    sflc_log_debug("Successfully run DM task");

    /* Wait for completion */
    dm_udev_wait(cookie);
    sflc_log_debug("Task completed");

    // No prob
    err = 0;

out:
    dm_task_destroy(dmt);
dup_free:
	free(dup_virt_dev_name);
	free(dup_params);

    return err;
}

/**
 * Close a Shufflecake virtual device (volume) under /dev/mapper.
 *
 * @param virt_dev_name the name of the virtual device, as it appears under
 *  /dev/mapper
 *
 * @return error code (0 on success)
 */
int sflc_dm_destroy(char * virt_dev_name)
{
    struct dm_task *dmt;
    uint32_t cookie = 0;
    uint16_t udev_flags = 0;
    int err = 0;
    
    /* Just to be sure, let's get it on the heap */
    char * dup_virt_dev_name = strdup(virt_dev_name);

    sflc_log_debug("Closing /dev/mapper/%s", dup_virt_dev_name);

    /* Instantiate the DM task (with the REMOVE ioctl command) */
    if (!(dmt = dm_task_create(DM_DEVICE_REMOVE))) {
        sflc_log_error("Cannot create dm_task");
        err = 1;
        goto dup_free;
    }
    sflc_log_debug("Successfully created dm_task");

    /* Set the name of the target device (to be closed) */
    if (!dm_task_set_name(dmt, dup_virt_dev_name)) {
        sflc_log_error("Cannot set device name");
        err = 2;
        goto out;
    }
    sflc_log_debug("Successfully set device name");

    /* Get a cookie (request ID, basically) to wait for task completion */
    if (!dm_task_set_cookie(dmt, &cookie, udev_flags)) {
        sflc_log_error("Cannot set cookie");
        err = 3;
        goto out;
    }
    sflc_log_debug("Successfully got a cookie");

    /* Needed for some reason */
    dm_task_retry_remove(dmt);
    sflc_log_debug("Successful retry_remove");

    /* Run the task */
    if (!dm_task_run(dmt)) {
        sflc_log_error("Cannot issue ioctl");
        err = 4;
        goto out;
    }
    sflc_log_debug("Successfully run task");

    /* Wait for completion */
    dm_udev_wait(cookie);
    sflc_log_debug("Task completed");

    // No prob
    err = 0;

out:
    dm_task_destroy(dmt);
dup_free:
	free(dup_virt_dev_name);

    return err;
}
