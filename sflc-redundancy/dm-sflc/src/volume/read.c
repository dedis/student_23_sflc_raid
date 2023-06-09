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
 * A read bio is handeld by shallow-cloning it (the pages remain those
 * of the original bio) and remapping the sector. The clone is 
 * submitted to the underlying device. Its bi_endio function decrypts 
 * the pages in place, and marks the original bio as complete.
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

static void sflc_vol_fillBioWithZeros(struct bio * orig_bio);
static void sflc_vol_readEndIo(struct bio * phys_bio);
static void sflc_vol_readEndIoBottomHalf(struct work_struct * work);
static int sflc_vol_decryptBio(sflc_Volume * vol, struct bio * orig_bio, u32 psi, u32 off_in_slice);
static int sflc_vol_fetchIv(sflc_Volume * vol, u8 * iv, u32 psi, u32 off_in_slice);

/*****************************************************
 *           PUBLIC FUNCTIONS DEFINITIONS            *
 *****************************************************/

/* Executed in context from sflc_tgt_map() */
void sflc_vol_doRead(sflc_Volume * vol, struct bio * bio)
{
        sflc_Device * dev = vol->dev;
        struct bio * orig_bio = bio;
        struct bio * phys_bio;
        s64 phys_sector;
        sflc_vol_DecryptWork * dec_work;
        blk_status_t status;

        /* Allocate decryptWork structure */
        dec_work = mempool_alloc(sflc_pools_decryptWorkPool, GFP_NOIO);
        if (!dec_work) {
                pr_err("Could not allocate decryptWork structure\n");
                status = BLK_STS_IOERR;
                goto err_alloc_dec_work;
        }

        /* Get an extra reference to the original bio */
        bio_get(orig_bio);

        /* Shallow-copy the bio and submit it (change the bi_endio).
           We can shallow-copy because we don't need to own the pages,
           we can decrypt in place. */

        /* Shallow copy */
        phys_bio = bio_clone_fast(orig_bio, GFP_NOIO, &sflc_pools_bioset);
        if (!phys_bio) {
                pr_err("Could not clone original bio\n");
                status = BLK_STS_IOERR;
                goto err_clone_orig_bio;
        }

        /* Set real backing device */
	bio_set_dev(phys_bio, dev->real_dev->bdev);
        /* Remap sector */
	phys_sector = sflc_vol_remapSector(vol, orig_bio->bi_iter.bi_sector, READ, &dec_work->psi, &dec_work->off_in_slice);
	/* If -ENXIO, special case: stupid READ */
	if (phys_sector == -ENXIO) {
		pr_warn("Stupid READ. Returning all zeros\n");
		sflc_vol_fillBioWithZeros(orig_bio);
		status = BLK_STS_OK;
		goto err_stupid_read;
	}
	/* Other errors */
        if (phys_sector < 0) {
                pr_err("Could not remap sector for physical bio; error %d\n", (int) phys_sector);
                status = BLK_STS_IOERR;
                goto err_remap_sector;
        }
        /* No errors */
        phys_bio->bi_iter.bi_sector = phys_sector;

        /* Set field in dec_work */
        dec_work->vol = vol;
        dec_work->orig_bio = orig_bio;
        dec_work->phys_bio = phys_bio;
        /* Set fields for the endio */
        phys_bio->bi_end_io = sflc_vol_readEndIo;
	phys_bio->bi_private = dec_work;

        /* Only submit the physical bio */
        submit_bio(phys_bio);

        return;


err_remap_sector:
err_stupid_read:
        bio_put(phys_bio);
err_clone_orig_bio:
        bio_put(orig_bio);
        mempool_free(dec_work, sflc_pools_decryptWorkPool);
err_alloc_dec_work:

        orig_bio->bi_status = status;
        bio_endio(orig_bio);
        return;
}

/*****************************************************
 *          PRIVATE FUNCTIONS DEFINITIONS            *
 *****************************************************/

static void sflc_vol_fillBioWithZeros(struct bio * orig_bio)
{
	struct bio_vec bvl = bio_iovec(orig_bio);
        void * sector_ptr = kmap(bvl.bv_page) + bvl.bv_offset;

        memset(sector_ptr, 0, SFLC_DEV_SECTOR_SIZE);
        bio_advance(orig_bio, SFLC_DEV_SECTOR_SIZE);

        kunmap(bvl.bv_page);
        return;
}

/* Pushes all the decryption work to a workqueue bottom half */
static void sflc_vol_readEndIo(struct bio * phys_bio)
{
        sflc_vol_DecryptWork * dec_work = phys_bio->bi_private;

        /* Init work structure */
        INIT_WORK(&dec_work->work, sflc_vol_readEndIoBottomHalf);
        /* Enqueue it */
        queue_work(sflc_queues_decryptQueue, &dec_work->work);

        return;
}

static void sflc_vol_readEndIoBottomHalf(struct work_struct * work)
{
	sflc_vol_DecryptWork * dec_work = container_of(work, sflc_vol_DecryptWork, work);
        sflc_Volume * vol = dec_work->vol;
        struct bio * orig_bio = dec_work->orig_bio;
        struct bio * phys_bio = dec_work->phys_bio;
        blk_status_t status = phys_bio->bi_status;
        int err;

        /* Decrypt the physical bio and advance the original bio */
        err = sflc_vol_decryptBio(vol, orig_bio, dec_work->psi, dec_work->off_in_slice);
        if (err) {
                pr_err("Could not decrypt bio; error %d\n", err);
                status = BLK_STS_IOERR;
        }

        /* Release the extra reference to the original bio */
        bio_put(orig_bio);
        /* End I/O on the original bio */
        orig_bio->bi_status = status;
        bio_endio(orig_bio);

        /* Free the physical bio */
	bio_put(phys_bio);
        /* Free the work item */
        mempool_free(dec_work, sflc_pools_decryptWorkPool);

	return;
}

/* Decrypts the content of the physical bio, and at the same time it advances the original bio */
static int sflc_vol_decryptBio(sflc_Volume * vol, struct bio * orig_bio, u32 psi, u32 off_in_slice)
{
        u8 iv[SFLC_SK_IV_LEN];
        int err;

        /* Fetch IV */
        err = sflc_vol_fetchIv(vol, iv, psi, off_in_slice);
        if (err) {
                pr_err("Could not fetch IV; error %d\n", err);
                return err;
        }

        /* Decrypt sector in place */
        struct bio_vec bvl = bio_iovec(orig_bio);
        void * sector_ptr = kmap(bvl.bv_page) + bvl.bv_offset;
        err = sflc_sk_decrypt(vol->skctx, sector_ptr, sector_ptr, SFLC_DEV_SECTOR_SIZE, iv);
        kunmap(bvl.bv_page);
        if (err) {
                pr_err("Error while decrypting sector: %d\n", err);
                return err;
        }

        /* Advance original bio by one sector */
        bio_advance(orig_bio, SFLC_DEV_SECTOR_SIZE);

        return 0;
}

static int sflc_vol_fetchIv(sflc_Volume * vol, u8 * iv, u32 psi, u32 off_in_slice)
{
        sflc_Device * dev = vol->dev;
        u8 * iv_block;
        int err;

        /* Acquire a reference to the whole relevant IV block */
        iv_block = sflc_dev_getIvBlockRef(dev, psi, READ);
        if (IS_ERR(iv_block)) {
                err = PTR_ERR(iv_block);
                pr_err("Could not acquire reference to IV block; error %ld\n", PTR_ERR(iv_block));
                goto err_get_iv_block_ref;
        }

        /* Copy the relevant portion */
        memcpy(iv, iv_block + (off_in_slice * SFLC_SK_IV_LEN), SFLC_SK_IV_LEN);

        /* Release reference to the IV block */
        err = sflc_dev_putIvBlockRef(dev, psi);
        if (err) {
                pr_err("Could not release reference to IV block; error %d\n", err);
                goto err_put_iv_block_ref;
        }

        return 0;


err_put_iv_block_ref:
err_get_iv_block_ref:
        return err;
}
