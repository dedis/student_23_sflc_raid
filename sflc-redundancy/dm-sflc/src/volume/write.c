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
 * A write bio is handled by deep cloning it (with owned pages), remapping
 * the sector, and encrypting the data. The cloned bio is the one that's
 * submitted to the underlying device. Its bi_endio function marks the
 * original bio as complete.
 */

/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/

#include "volume.h"
#include "crypto/rand/rand.h"
#include "utils/pools.h"
#include "log/log.h"

/*****************************************************
 *                     CONSTANTS                     *
 *****************************************************/

/*****************************************************
 *           PRIVATE FUNCTIONS PROTOTYPES            *
 *****************************************************/

static int sflc_vol_encryptOrigBio(sflc_Volume *vol, struct bio *orig_bio, struct bio *phys_bio, u32 psi, u32 off_in_slice);
static int sflc_vol_sampleAndWriteIv(sflc_Volume *vol, u8 *iv, u32 psi, u32 off_in_slice);
static void sflc_vol_writeEndIo(struct bio *phys_bio);

/*****************************************************
 *                PRIVATE VARIABLES                  *
 *****************************************************/

/*****************************************************
 *           PUBLIC FUNCTIONS DEFINITIONS            *
 *****************************************************/

/* Executed in workqueue bottom half */
void sflc_vol_doWrite(struct work_struct *work)
{
        sflc_vol_WriteWork *write_work = container_of(work, sflc_vol_WriteWork, work);
        sflc_Volume *vol = write_work->vol;
        sflc_Device *dev = vol->dev;
        struct bio *orig_bio = write_work->orig_bio;
        struct bio *phys_bio;
        s64 phys_sector;
        u32 psi;
        u32 off_in_slice;

        /* Get an extra reference to the original bio */
        bio_get(orig_bio);

        /* Deep-copy the bio and encrypt the data.
           We need to deep-copy because I'm not sure
           we can encrypt in place and change the data in
           non-owned bio's. So we need our own. */

        /* Allocate an empty bio */
        phys_bio = bio_alloc_bioset(GFP_NOIO, bio_segments(orig_bio), &sflc_pools_bioset);
        if (!phys_bio)
        {
                pr_err("Could not allocate bio\n");
                goto err_alloc_phys_bio;
        }

        /* Set real backing device */
        bio_set_dev(phys_bio, dev->real_dev->bdev);
        /* Remap sector */
        phys_sector = sflc_vol_remapSector(vol, orig_bio->bi_iter.bi_sector, WRITE, &psi, &off_in_slice);
        if (phys_sector < 0)
        {
                pr_err("Could not remap sector for physical bio; error %d\n", (int)phys_sector);
                goto err_remap_sector;
        }
        phys_bio->bi_iter.bi_sector = phys_sector;
        /* Copy operation and flags */
        phys_bio->bi_opf = orig_bio->bi_opf;

        /* Set fields for the endio */
        phys_bio->bi_end_io = sflc_vol_writeEndIo;
        phys_bio->bi_private = write_work;

        /* Encrypt the original bio into the physical bio (newly-allocated pages) */
        int err = sflc_vol_encryptOrigBio(vol, orig_bio, phys_bio, psi, off_in_slice);
        if (err)
        {
                pr_err("Could not encrypt original bio; error %d\n", err);
                goto err_encrypt_orig_bio;
        }

        /* Only submit the physical bio */
        submit_bio(phys_bio);

        return;

err_encrypt_orig_bio:
err_remap_sector:
        bio_put(phys_bio);
err_alloc_phys_bio:
        bio_put(orig_bio);

        orig_bio->bi_status = BLK_STS_IOERR;
        bio_endio(orig_bio);
        return;
}

/*****************************************************
 *          PRIVATE FUNCTIONS DEFINITIONS            *
 *****************************************************/

/* Encrypts the contents of the original bio into newly-allocated pages for the physical bio */
static int sflc_vol_encryptOrigBio(sflc_Volume *vol, struct bio *orig_bio, struct bio *phys_bio, u32 psi, u32 off_in_slice)
{
        sflc_vol_WriteWork *write_work = phys_bio->bi_private;
        u8 iv[SFLC_SK_IV_LEN];
        int err;

        /* Allocate new page for the physical bio */
        write_work->page = mempool_alloc(sflc_pools_pagePool, GFP_NOIO);
        if (!write_work->page)
        {
                pr_err("Could not allocate page\n");
                err = -ENOMEM;
                goto err_alloc_page;
        }

        /* Add it to physical bio */
        if (!bio_add_page(phys_bio, write_work->page, SFLC_DEV_SECTOR_SIZE, 0))
        {
                pr_err("Catastrophe. Could not add page to copy bio. WTF?\n");
                err = -EIO;
                goto err_bio_add_page;
        }

        /* Sample a random IV and write it in the IV block */
        err = sflc_vol_sampleAndWriteIv(vol, iv, psi, off_in_slice);
        if (err)
        {
                pr_err("Could not sample and write IV; error %d\n", err);
                err = -EIO;
                goto err_sample_and_write_iv;
        }

        /* Encrypt sector out of place */
        struct bio_vec bvl = bio_iovec(orig_bio);
        void *enc_sector_ptr = kmap(write_work->page);
        void *plain_sector_ptr = kmap(bvl.bv_page) + bvl.bv_offset;
        err = sflc_sk_encrypt(vol->skctx, plain_sector_ptr, enc_sector_ptr, SFLC_DEV_SECTOR_SIZE, iv);
        kunmap(bvl.bv_page);
        kunmap(write_work->page);
        if (err)
        {
                pr_err("Error while encrypting sector: %d\n", err);
                goto err_encrypt;
        }

        return 0;

err_encrypt:
err_sample_and_write_iv:
err_bio_add_page:
        mempool_free(write_work->page, sflc_pools_pagePool);
err_alloc_page:
        return err;
}

/* Allocates the io_work's IV (will need to be freed afterwards), fills it with random
   bytes, and writes it into the IV block pointed by the io_work's PSI and off_in_slice. */
static int sflc_vol_sampleAndWriteIv(sflc_Volume *vol, u8 *iv, u32 psi, u32 off_in_slice)
{
        sflc_Device *dev = vol->dev;
        u8 *iv_block;
        int err;

        /* Sample IV */
        err = sflc_rand_getBytes(iv, SFLC_SK_IV_LEN);
        if (err)
        {
                pr_err("Could not sample IV; error %d\n", err);
                err = -EIO;
                goto err_sample_iv;
        }

        /* Acquire a reference to the whole relevant IV block */
        iv_block = sflc_dev_getIvBlockRef(dev, psi, WRITE);
        if (IS_ERR(iv_block))
        {
                err = PTR_ERR(iv_block);
                pr_err("Could not acquire reference to IV block; error %d\n", err);
                goto err_get_iv_block_ref;
        }

        /* Copy it into the relevant portion of the block */
        memcpy(iv_block + (off_in_slice * SFLC_SK_IV_LEN), iv, SFLC_SK_IV_LEN);

        /* Release reference to the IV block */
        err = sflc_dev_putIvBlockRef(dev, psi);
        if (err)
        {
                pr_err("Could not release reference to IV block; error %d\n", err);
                goto err_put_iv_block_ref;
        }

        return 0;

err_put_iv_block_ref:
err_get_iv_block_ref:
err_sample_iv:
        return err;
}

static void sflc_vol_writeEndIo(struct bio *phys_bio)
{
        sflc_vol_WriteWork *write_work = phys_bio->bi_private;
        struct bio *orig_bio = write_work->orig_bio;
        unsigned completed_bytes;

        /* Release the extra reference to the original bio */
        bio_put(orig_bio);
        /* End I/O on the original bio */
        completed_bytes = orig_bio->bi_iter.bi_size - phys_bio->bi_iter.bi_size;
        bio_advance(orig_bio, completed_bytes);
        if (unlikely(orig_bio->bi_iter.bi_size))
        {
                pr_err("Incomplete orig_bio: %u\n", orig_bio->bi_iter.bi_size);
        }
        orig_bio->bi_status = phys_bio->bi_status;
        bio_endio(orig_bio);

        /* Free the physical bio */
        bio_put(phys_bio);
        /* Free the page, and the work item */
        if (unlikely(page_ref_count(write_work->page) != 1))
        {
                pr_err("WTF: page_ref_count = %d\n", page_ref_count(write_work->page));
        }
        mempool_free(write_work->page, sflc_pools_pagePool);
        mempool_free(write_work, sflc_pools_writeWorkPool);

        return;
}
