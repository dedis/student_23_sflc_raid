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

#include "device.h"
#include "utils/pools.h"
#include "log/log.h"

/*****************************************************
 *                     CONSTANTS                     *
 *****************************************************/

/*****************************************************
 *           PRIVATE FUNCTIONS PROTOTYPES            *
 *****************************************************/

/*****************************************************
 *           PUBLIC FUNCTIONS DEFINITIONS            *
 *****************************************************/

/* Synchronously reads/writes one 4096-byte sector from/to the underlying device 
   to/from the provided page */
int sflc_dev_rwSector(sflc_Device * dev, struct page * page, sector_t sector, int rw)
{
        struct bio * bio;
        int err;

        /* Allocate bio */
        bio = bio_alloc_bioset(GFP_NOIO, 1, &sflc_pools_bioset);
        if (!bio) {
                pr_err("Could not allocate bio\n");
                return -ENOMEM;
        }

        /* Set real backing device */
	bio_set_dev(bio, dev->real_dev->bdev);
        /* Set sector */
        bio->bi_iter.bi_sector = sector * SFLC_DEV_SECTOR_SCALE;
        /* Set flags */
        bio->bi_opf = ((rw == READ) ? REQ_OP_READ : REQ_OP_WRITE);
        /* Add page */
        if (!bio_add_page(bio, page, SFLC_DEV_SECTOR_SIZE, 0)) {
                pr_err("Catastrophe: could not add page to bio! WTF?\n");
                err = EINVAL;
                goto out;
        }

        /* Submit */
        err = submit_bio_wait(bio);

out:
        /* Free and return; */
        bio_put(bio);
        return err;
}

/*****************************************************
 *          PRIVATE FUNCTIONS DEFINITIONS            *
 *****************************************************/
