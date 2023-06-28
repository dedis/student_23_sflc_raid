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
 * This file only implements the IO-related volume functions.
 */

/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/

#include "volume.h"
#include "utils/pools.h"
#include "utils/workqueues.h"
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

/* Remaps the underlying block device and the sector number */
int sflc_vol_remapBioFast(sflc_Volume *vol, struct bio *bio)
{
        s64 phys_sector;
        int err;

        /* Replace the underlying block device */
        bio_set_dev(bio, vol->dev->real_dev->bdev);

        /* Remap the starting sector (we don't care about PSI and off_in_slice). Also no slice allocation */
        phys_sector = sflc_vol_remapSector(vol, bio->bi_iter.bi_sector, READ, NULL, NULL);
        if (phys_sector < 0)
        {
                err = (int)phys_sector;
                pr_err("Could not remap sector; error %d\n", err);
                return err;
        }
        bio->bi_iter.bi_sector = phys_sector;

        return 0;
}

/* Submits the bio to the device's workqueue */
int sflc_vol_processBio(sflc_Volume *vol, struct bio *bio)
{
        sflc_vol_WriteWork *write_work;

        /* If it is a READ, no need to pass it through a workqueue */
        if (bio_data_dir(bio) == READ)
        {
                sflc_vol_doRead(vol, bio);
                return 0;
        }

        /* Allocate writeWork structure */
        write_work = mempool_alloc(sflc_pools_writeWorkPool, GFP_NOIO);
        if (!write_work)
        {
                pr_err("Failed allocation of work structure\n");
                return -ENOMEM;
        }

        /* Set fields */
        write_work->vol = vol;
        write_work->orig_bio = bio;
        INIT_WORK(&write_work->work, sflc_vol_doWrite);

        /* Enqueue */
        queue_work(sflc_queues_writeQueue, &write_work->work);

        return 0;
}

// sflc-raid START
int sflc_vol_processBioRedundantlyAmong(sflc_Volume *vol, sflc_Volume *copy_vol, struct bio *bio)
{
        sflc_vol_WriteWork *write_work;
        sflc_vol_WriteWork *red_write_work;
        struct bio *red_bio;

        /* If it is a READ, no need to pass it through a workqueue */
        if (bio_data_dir(bio) == READ)
        {
                sflc_vol_doRead(vol, bio);
                return 0;
        }

        /* Allocate writeWork structure */
        write_work = mempool_alloc(sflc_pools_writeWorkPool, GFP_NOIO);
        if (!write_work)
        {
                pr_err("Failed allocation of work structure\n");
                return -ENOMEM;
        }

        /* Set fields */
        write_work->vol = vol;
        write_work->orig_bio = bio;
        INIT_WORK(&write_work->work, sflc_vol_doWrite);

        /* Enqueue original bio */
        queue_work(sflc_queues_writeQueue, &write_work->work);

        // Dirty hack to not take superblocks in 1GB device
        if (bio->bi_iter.bi_sector == 29560)
        {
                return 0;
        }

        /* Allocate writeWork structure */
        red_write_work = mempool_alloc(sflc_pools_writeWorkPool, GFP_NOIO);
        if (!red_write_work)
        {
                pr_err("Failed allocation of work structure\n");
                return -ENOMEM;
        }

        bio_get(bio);

        /* Shallow copy bio (as it is already a cloned bio it is ok); a new bio will be allocated later*/
        red_bio = bio_clone_fast(bio, GFP_NOIO, &sflc_pools_bioset);
        if (!red_bio)
        {
                pr_err("Could not allocate bio\n");
                return -ENOMEM;
        }

        /* Set sector */
        red_bio->bi_iter.bi_sector = bio->bi_iter.bi_sector;
        /* Set flags */
        red_bio->bi_opf = bio->bi_opf;

        /* Set fields */
        red_write_work->vol = copy_vol;
        red_write_work->orig_bio = red_bio;
        INIT_WORK(&red_write_work->work, sflc_vol_doWrite);

        /* Enqueue original bio */
        queue_work(sflc_queues_writeQueue, &red_write_work->work);

        return 0;
}

int sflc_vol_processBioRedundantlyWithin(sflc_Volume *vol, struct bio *bio)
{
        sflc_vol_WriteWork *write_work;
        sflc_vol_WriteWork *red_write_work;
        struct bio *red_bio;

        /* If it is a READ, no need to pass it through a workqueue */
        if (bio_data_dir(bio) == READ)
        {
                sflc_vol_doRead(vol, bio);
                return 0;
        }

        /* Allocate writeWork structure */
        write_work = mempool_alloc(sflc_pools_writeWorkPool, GFP_NOIO);
        if (!write_work)
        {
                pr_err("Failed allocation of work structure\n");
                return -ENOMEM;
        }

        /* Set fields */
        write_work->vol = vol;
        write_work->orig_bio = bio;
        INIT_WORK(&write_work->work, sflc_vol_doWrite);

        /* Enqueue original bio */
        queue_work(sflc_queues_writeQueue, &write_work->work);

        // Dirty hack to not take superblocks in 1GB device
        if (bio->bi_iter.bi_sector == 29560 || bio->bi_iter.bi_sector == 29520 || bio->bi_iter.bi_sector == 262144)
        {
                return 0;
        }

        /* Allocate writeWork structure */
        red_write_work = mempool_alloc(sflc_pools_writeWorkPool, GFP_NOIO);
        if (!red_write_work)
        {
                pr_err("Failed allocation of work structure\n");
                return -ENOMEM;
        }

        bio_get(bio);

        /* Shallow copy bio (as it is already a cloned bio it is ok); a new bio will be allocated later*/
        red_bio = bio_clone_fast(bio, GFP_NOIO, &sflc_pools_bioset);
        if (!red_bio)
        {
                pr_err("Could not allocate bio\n");
                return -ENOMEM;
        }

        sector_t red_sector;
        if (bio->bi_iter.bi_sector / (SFLC_VOL_LOG_SLICE_SIZE * SFLC_DEV_SECTOR_SCALE) % 2 == 0)
        {
                red_sector = bio->bi_iter.bi_sector + SFLC_VOL_LOG_SLICE_SIZE * SFLC_DEV_SECTOR_SCALE;
        }
        else
        {
                red_sector = bio->bi_iter.bi_sector - SFLC_VOL_LOG_SLICE_SIZE * SFLC_DEV_SECTOR_SCALE;
        }

        /* Set sector */
        red_bio->bi_iter.bi_sector = red_sector;
        /* Set flags */
        red_bio->bi_opf = bio->bi_opf;

        /* Set fields */
        red_write_work->vol = vol;
        red_write_work->orig_bio = red_bio;
        INIT_WORK(&red_write_work->work, sflc_vol_doWrite);

        /* Enqueue original bio */
        queue_work(sflc_queues_writeQueue, &red_write_work->work);

        return 0;
}
// sflc-raid END

/*****************************************************
 *          PRIVATE FUNCTIONS DEFINITIONS            *
 *****************************************************/
