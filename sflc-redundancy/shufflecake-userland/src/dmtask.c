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


/*
 * Helpers for DM tasks
 */

/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/

#include "dmtask.h"
#include "sflc.h"
#include "utils.h"

/*****************************************************
 *           PUBLIC FUNCTIONS DEFINITIONS            *
 *****************************************************/

int sflc_dmt_create(char * virt_dev_name, uint64_t num_sectors, char * param)
{
    struct dm_task *dmt;
    uint32_t cookie = 0;
    uint16_t udev_flags = 0;
    int is_success = 0;

    /* Just to be sure */
    char * _virt_dev_name = strdup(virt_dev_name);
    char * _param = strdup(param);

    if (!(dmt = dm_task_create(DM_DEVICE_CREATE))) {
        print_red("DEBUG: Cannot create dm_task\n");
        return 0;
    }

    if (!dm_task_set_name(dmt, _virt_dev_name)) {
        print_red("DEBUG: Cannot set device name\n");
        goto out;
    }

    if (!dm_task_add_target(dmt, 0, num_sectors, SFLC_TARGET_NAME, _param)) {
        goto out;
    }

    if (!dm_task_set_add_node(dmt, DM_ADD_NODE_ON_CREATE)) {
        print_red("DEBUG: Cannot add node\n");
        goto out;
    }

    if (!dm_task_set_cookie(dmt, &cookie, udev_flags)) {
        print_red("DEBUG: Cannot set cookie\n");
        goto out;
    }

    if (!dm_task_run(dmt)) {
        print_red("DEBUG: Cannot issue ioctl\n");
        goto out;
    }

    is_success = 1;

out:
    dm_udev_wait(cookie);
    dm_task_destroy(dmt);

    return is_success;
}

int sflc_dmt_destroy(char * virt_dev_name)
{
    struct dm_task *dmt;
    uint32_t cookie = 0;
    uint16_t udev_flags = 0;
    int is_success = 0;

    print_green("Closing: %s\n", virt_dev_name);
    
    /* Just to be sure */
    char * _virt_dev_name = strdup(virt_dev_name);

    if (!(dmt = dm_task_create(DM_DEVICE_REMOVE))) {
        print_red("DEBUG: Cannot create dm_task\n");
        return 0;
    }

    if (!dm_task_set_name(dmt, _virt_dev_name)) {
        print_red("DEBUG: Cannot set device name\n");
        goto out;
    }

    if (!dm_task_set_cookie(dmt, &cookie, udev_flags)) {
        goto out;
    }

    dm_task_retry_remove(dmt);

    if (!dm_task_run(dmt)) {
        print_red("DEBUG: Cannot issue ioctl\n");
        goto out;
    }
    print_green("Done!\n");
    is_success = 1;

out:
    dm_udev_wait(cookie);
    dm_task_destroy(dmt);

    return is_success;
}
