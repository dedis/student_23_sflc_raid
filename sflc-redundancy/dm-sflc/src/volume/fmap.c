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
#include "crypto/rand/rand.h"
#include "utils/pools.h"
#include "log/log.h"

/*****************************************************
 *                     CONSTANTS                     *
 *****************************************************/

/*****************************************************
 *           PRIVATE FUNCTIONS PROTOTYPES            *
 *****************************************************/

//s32 sflc_vol_mapSlice(sflc_Volume * vol, u32 lsi, int op);

/*****************************************************
 *           PUBLIC FUNCTIONS DEFINITIONS            *
 *****************************************************/

/* Maps a logical 512-byte sector to a physical 512-byte sector. Returns < 0 if error.
 * Specifically, if op == READ, and the logical slice is unmapped, -ENXIO is returned. */
s64 sflc_vol_remapSector(sflc_Volume * vol, sector_t log_sector, int op, u32 * psi_out, u32 * off_in_slice_out)
{
        u32 lsi;
        u32 off_in_slice;
        s32 psi;
        sector_t phys_sector;

        /* Start by scaling down to a Shufflecake sector */
        log_sector /= SFLC_DEV_SECTOR_SCALE;

        /* Get the logical slice index it belongs to */
        lsi = log_sector / SFLC_VOL_LOG_SLICE_SIZE;
        /* Get which block it is within the slice */
        off_in_slice = log_sector % SFLC_VOL_LOG_SLICE_SIZE;
        /* Output the off_in_slice */
        if (off_in_slice_out) {
                *off_in_slice_out = off_in_slice;
        }

        /* Map it to a physical slice */
        psi = sflc_vol_mapSlice(vol, lsi, op);
        /* -ENXIO is a special case */
        if (psi == -ENXIO) {
        	pr_warn("mapSlice returned -ENXIO: stupid READ\n");
        	return -ENXIO;
        }
        /* Other errors */
        if (psi < 0) {
                pr_err("Could not map LSI to PSI; error %d\n", psi);
                return psi;
        }
        /* Output the PSI */
        if (psi_out) {
                *psi_out = psi;
        }

        /* Get the physical sector (the first of every slice contains the IVs) */
        phys_sector = ((sector_t)psi * SFLC_DEV_PHYS_SLICE_SIZE) + 1 + off_in_slice;
        /* Add the device header */
        phys_sector += SFLC_DEV_HEADER_SIZE;

        /* Scale it back up to a kernel sector */
        phys_sector *= SFLC_DEV_SECTOR_SCALE;

        return phys_sector;
}

/* Loads (and decrypts) the position map from the volume's header */
int sflc_vol_loadFmap(sflc_Volume * vol)
{
        sflc_Device * dev = vol->dev;
        sector_t sector;
        struct page * iv_page;
        u8 * iv_ptr;
        struct page * data_page;
        u8 * data_ptr;
        u32 lsi;
        int err;

        /* Allocate pages */
        iv_page = mempool_alloc(sflc_pools_pagePool, GFP_NOIO);
        if (!iv_page) {
                pr_err("Could not allocate IV page\n");
                return -ENOMEM;
        }
        data_page = mempool_alloc(sflc_pools_pagePool, GFP_NOIO);
        if (!data_page) {
                pr_err("Could not allocate data page\n");
                return -ENOMEM;
        }
        /* Kmap them */
        iv_ptr = kmap(iv_page);
        data_ptr = kmap(data_page);

        /* Lock both the forward and the reverse position maps */
        if (mutex_lock_interruptible(&vol->fmap_lock)) {
                pr_err("Interrupted while waiting to lock fmap\n");
                return -EINTR;
        }
        if (mutex_lock_interruptible(&dev->rmap_lock)) {
                pr_err("Interrupted while waiting to lock rmap\n");
                mutex_unlock(&vol->fmap_lock);
                return -EINTR;
        }

        /* Starting sector of the volume header (first sector is reserved to userland tool) */
        sector = (vol->vol_idx * SFLC_VOL_HEADER_SIZE) + 1;
        /* Starting LSI in the fmap */
        lsi = 0;

        /* Loop over the 4 IV-data slices */
        int i;
        for (i = 0; i < SFLC_VOL_HEADER_IV_BLOCKS && lsi < dev->tot_slices; i++) {
                /* Load the IV block */
                err = sflc_dev_rwSector(dev, iv_page, sector, READ);
                if (err) {
                        pr_err("Could not read IV block i=%d at sector %llu; error %d\n", i, sector, err);
                        goto out;
                }
                sector += 1;

                /* Loop over the 256 data blocks */
                int j;
                for (j = 0; j < SFLC_DEV_SECTOR_TO_IV_RATIO && lsi < dev->tot_slices; j++) {
                        /* Load the data block */
                        err = sflc_dev_rwSector(dev, data_page, sector, READ);
                        if (err) {
                                pr_err("Could not read data block i=%d, j=%d at sector %llu; error %d\n", i, j, sector, err);
                                goto out;
                        }
                        sector += 1;

                        /* Decrypt it in place */
                        err = sflc_sk_decrypt(vol->skctx, data_ptr, data_ptr, SFLC_DEV_SECTOR_SIZE, (iv_ptr + j*SFLC_SK_IV_LEN));
                        if (err) {
                                pr_err("Could not decrypt data block i=%d, j=%d at sector %llu; error %d\n", i, j, sector, err);
                                goto out;
                        }

                        /* Loop over the 1024 fmap entries in this data block */
                        int k;
                        for (k = 0; k < SFLC_VOL_HEADER_MAPPINGS_PER_BLOCK && lsi < dev->tot_slices; k++) {
                                /* An entry is just a single big-endian PSI, the LSI
                                   is implicitly the index of this entry */
                                __be32 * be_psi = (void *) (data_ptr + (k * sizeof(__be32)));
                                u32 psi = be32_to_cpu(*be_psi);

                                /* Add mapping to the volume's fmap */
                                vol->fmap[lsi] = psi;
                                /* Also add it to the device's rmap and to the count, if LSI is actually mapped */
                                if (psi != SFLC_VOL_FMAP_INVALID_PSI) {
                                        sflc_dev_setRmap(dev, psi, vol->vol_idx);
                                        vol->mapped_slices += 1;
                                }

                                /* Next iteration */
                                lsi += 1;
                        }
                }
        }
        /* No error if we made it here */
        err = 0;

out:
        /* Unlock both maps */
        mutex_unlock(&dev->rmap_lock);
        mutex_unlock(&vol->fmap_lock);
        /* Kunmap pages */
        kunmap(iv_page);
        kunmap(data_page);
        /* Free them */
        mempool_free(iv_page, sflc_pools_pagePool);
        mempool_free(data_page, sflc_pools_pagePool);

        return err;
}

/* Stores (and encrypts) the position map to the volume's header */
int sflc_vol_storeFmap(sflc_Volume * vol)
{
        sflc_Device * dev = vol->dev;
        sector_t sector;
        struct page * iv_page;
        u8 * iv_ptr;
        struct page * data_page;
        u8 * data_ptr;
        u32 lsi;
        int err;

        /* Allocate pages */
        iv_page = mempool_alloc(sflc_pools_pagePool, GFP_NOIO);
        if (!iv_page) {
                pr_err("Could not allocate IV page\n");
                return -ENOMEM;
        }
        data_page = mempool_alloc(sflc_pools_pagePool, GFP_NOIO);
        if (!data_page) {
                pr_err("Could not allocate data page\n");
                return -ENOMEM;
        }
        /* Kmap them */
        iv_ptr = kmap(iv_page);
        data_ptr = kmap(data_page);

        /* Lock both the forward and the reverse position maps */
        if (mutex_lock_interruptible(&vol->fmap_lock)) {
                pr_err("Interrupted while waiting to lock fmap\n");
                return -EINTR;
        }
        if (mutex_lock_interruptible(&dev->rmap_lock)) {
                pr_err("Interrupted while waiting to lock rmap\n");
                mutex_unlock(&vol->fmap_lock);
                return -EINTR;
        }

        /* Starting sector of the volume header */
        sector = (vol->vol_idx * SFLC_VOL_HEADER_SIZE) + 1;
        /* Starting LSI in the fmap */
        lsi = 0;

        /* Loop over the 4 IV-data slices */
        int i;
        for (i = 0; i < SFLC_VOL_HEADER_IV_BLOCKS && lsi < dev->tot_slices; i++) {
                /* Fill the IV block with random bytes */
                err = sflc_rand_getBytes(iv_ptr, SFLC_DEV_SECTOR_SIZE);
                if (err) {
                        pr_err("Could not sample random IV for block i=%d at sector %llu; error %d\n", i, sector, err);
                        goto out;
                }
                /* Store it on the disk (before it gets changed by the encryption) */
                err = sflc_dev_rwSector(dev, iv_page, sector, WRITE);
                if (err) {
                        pr_err("Could not read IV block i=%d at sector %llu; error %d\n", i, sector, err);
                        goto out;
                }
                sector += 1;

                /* Loop over the 256 data blocks */
                int j;
                for (j = 0; j < SFLC_DEV_SECTOR_TO_IV_RATIO && lsi < dev->tot_slices; j++) {
                        /* Loop over the 1024 fmap entries that fit in this data block */
                        int k;
                        for (k = 0; k < SFLC_VOL_HEADER_MAPPINGS_PER_BLOCK && lsi < dev->tot_slices; k++) {
                                /* Get the PSI for the current LSI */
                                u32 psi = vol->fmap[lsi];
                                /* Write it into the block as big-endian */
                                __be32 * be_psi = (void *) (data_ptr + (k * sizeof(__be32)));
                                *be_psi = cpu_to_be32(psi);

                                /* Next iteration */
                                lsi += 1;
                        }

                        /* Encrypt it in place */
                        err = sflc_sk_encrypt(vol->skctx, data_ptr, data_ptr, SFLC_DEV_SECTOR_SIZE, (iv_ptr + j*SFLC_SK_IV_LEN));
                        if (err) {
                                pr_err("Could not encrypt data block i=%d, j=%d at sector %llu; error %d\n", i, j, sector, err);
                                goto out;
                        }

                        /* Store the data block */
                        err = sflc_dev_rwSector(dev, data_page, sector, WRITE);
                        if (err) {
                                pr_err("Could not write data block i=%d, j=%d at sector %llu; error %d\n", i, j, sector, err);
                                goto out;
                        }
                        sector += 1;              
                }
        }
        /* No error if we made it here */
        err = 0;

out:
        /* Unlock both maps */
        mutex_unlock(&dev->rmap_lock);
        mutex_unlock(&vol->fmap_lock);
        /* Kunmap pages */
        kunmap(iv_page);
        kunmap(data_page);
        /* Free them */
        mempool_free(iv_page, sflc_pools_pagePool);
        mempool_free(data_page, sflc_pools_pagePool);

        return err;
}

/*****************************************************
 *          PRIVATE FUNCTIONS DEFINITIONS            *
 *****************************************************/

s32 sflc_vol_mapSlice(sflc_Volume * vol, u32 lsi, int op)
{
        s32 psi;
        sflc_Device * dev = vol->dev;

        /* Lock the volume's forward map */
        if (mutex_lock_interruptible(&vol->fmap_lock)) {
                pr_err("Interrupted while waiting to lock the forward position map\n");
                return -EINTR;
        }

        /* If slice is already mapped, just return the mapping */
        if (vol->fmap[lsi] != SFLC_VOL_FMAP_INVALID_PSI) {
                mutex_unlock(&vol->fmap_lock);
                return vol->fmap[lsi];
        }

        /* If slice is not mapped, but the operation is a READ, return -ENXIO */
        if (op == READ) {
                mutex_unlock(&vol->fmap_lock);
                return -ENXIO;
        }

        /* Otherwise, create a new slice mapping */

        /* Also lock the device's reverse map */
        if (mutex_lock_interruptible(&dev->rmap_lock)) {
                pr_err("Interrupted while waiting to lock the reverse position map\n");
                mutex_unlock(&vol->fmap_lock);
                return -EINTR;
        }

        /* Get a free physical slice */
        psi = sflc_dev_getRandomFreePsi(dev);
        if (psi < 0) {
                pr_err("Could not get a random free physical slice; error %d\n", psi);
                mutex_unlock(&dev->rmap_lock);
                mutex_unlock(&vol->fmap_lock);
                return psi;
        }

        /* Insert the mapping into the volume's fmap */
        vol->fmap[lsi] = psi;
        vol->mapped_slices += 1;
        /* And in the device's rmap */
        sflc_dev_setRmap(dev, psi, vol->vol_idx);

        /* Unlock both maps */
        mutex_unlock(&dev->rmap_lock);
        mutex_unlock(&vol->fmap_lock);

        return psi;
}
