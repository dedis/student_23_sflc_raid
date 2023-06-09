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
 * Implementations for all the bio utility functions
 */

/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/

#include "workqueues.h"
#include "log/log.h"

/*****************************************************
 *                     CONSTANTS                     *
 *****************************************************/

#define SFLC_QUEUES_WRITE_WQ_NAME "sflc_write_workqueue"
#define SFLC_QUEUES_DECRYPT_WQ_NAME "sflc_decrypt_workqueue"

/*****************************************************
 *           PUBLIC VARIABLES DEFINITIONS            *
 *****************************************************/

struct workqueue_struct * sflc_queues_writeQueue;
struct workqueue_struct * sflc_queues_decryptQueue;

/*****************************************************
 *           PUBLIC FUNCTIONS DEFINITIONS            *
 *****************************************************/

int sflc_queues_init(void)
{
        int err;

        /* Write workqueue */
        sflc_queues_writeQueue = create_workqueue(SFLC_QUEUES_WRITE_WQ_NAME);
        if (!sflc_queues_writeQueue) {
                pr_err("Could not create write workqueue\n");
                err = -ENOMEM;
                goto err_write_queue;
        }

        /* Decrypt workqueue */
        sflc_queues_decryptQueue = create_workqueue(SFLC_QUEUES_DECRYPT_WQ_NAME);
        if (!sflc_queues_decryptQueue) {
                pr_err("Could not create decrypt workqueue\n");
                err = -ENOMEM;
                goto err_decrypt_queue;
        }

        return 0;


err_decrypt_queue:
        destroy_workqueue(sflc_queues_writeQueue);
err_write_queue:
        return err;
}

void sflc_queues_exit(void)
{
        destroy_workqueue(sflc_queues_decryptQueue);
        destroy_workqueue(sflc_queues_writeQueue);
}
