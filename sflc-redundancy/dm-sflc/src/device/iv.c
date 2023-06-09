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

/* Capacity of IV cache */
#define SFLC_DEV_IV_CACHE_CAPACITY 1024

/*****************************************************
 *                      MACROS                       *
 *****************************************************/

#define sflc_dev_psiToIvBlockSector(psi) (SFLC_DEV_HEADER_SIZE + (sector_t)(psi) * SFLC_DEV_PHYS_SLICE_SIZE)

/*****************************************************
 *           PRIVATE FUNCTIONS PROTOTYPES            *
 *****************************************************/

static sflc_dev_IvCacheEntry * sflc_dev_newIvCacheEntry(sflc_Device * dev, u32 psi);
static int sflc_dev_destroyIvCacheEntry(sflc_Device * dev, sflc_dev_IvCacheEntry * entry);

/*****************************************************
 *           PUBLIC FUNCTIONS DEFINITIONS            *
 *****************************************************/

/* Get a read/write pointer to the specified IV block. Increases the refcount.
   Returns an ERR_PTR() if error. */
u8 * sflc_dev_getIvBlockRef(sflc_Device * dev, u32 psi, int rw)
{
        sflc_dev_IvCacheEntry * entry;
        int err;

        /* Lock + waitqueue pattern */

        /* Acquire the lock */
        if (mutex_lock_interruptible(&dev->iv_cache_lock)) {
                pr_err("Interrupted while waiting to lock IV cache\n");
                err = -EINTR;
                goto err_lock_cache;
        }

        /* Check for either of two conditions in order to go through */
        while (dev->iv_cache[psi] == NULL && dev->iv_cache_nr_entries >= SFLC_DEV_IV_CACHE_CAPACITY) {
                /* We can't go through, yield the lock */
                mutex_unlock(&dev->iv_cache_lock);

                /* Sleep in the waitqueue (same conditions) */
                if (wait_event_interruptible(dev->iv_cache_waitqueue, dev->iv_cache[psi] != NULL || 
                                                dev->iv_cache_nr_entries < SFLC_DEV_IV_CACHE_CAPACITY)) {
                        err = -EINTR;
                        pr_err("Interrupted while waiting in waitqueue\n");
                        goto err_wait_queue;
                }

                /* Re-acquire the lock, hoping that either condition will be true at the next iteration */
                if (mutex_lock_interruptible(&dev->iv_cache_lock)) {
                        pr_err("Interrupted while waiting to re-lock IV cache\n");
                        err = -EINTR;
                        goto err_relock_cache;
                }
        }

        /* Phew, we're out! At this point, we hold the lock, and one of two conditions is true (or both):
           either our desired cache entry is already in cache (in which case we can just grab a new reference),
           or there is enough space in the cache for us to create it. */

        /* Let's see which one it is */
        entry = dev->iv_cache[psi];
        if (!entry) {
                /* Create it */
                entry = sflc_dev_newIvCacheEntry(dev, psi);
                if (IS_ERR(entry)) {
                        err = PTR_ERR(entry);
                        pr_err("Could not  create new cache entry; error %d\n", err);
                        goto err_create_entry;
                }

                /* Insert it into the cache */
                dev->iv_cache[psi] = entry;
                /* Update cache size */
                dev->iv_cache_nr_entries += 1;

                /* Insert it anywhere in the LRU list (won't be evicted anyway as long as it's reffed) */
                list_add(&entry->lru_node, &dev->iv_lru_list);

                /* We just altered the condition someone might be sleeping for: tell the waitqueue */
                wake_up_interruptible(&dev->iv_cache_waitqueue);
        }

        /* Increase refcount, and possibly dirtyness */
        entry->refcnt += 1;
        if (rw == WRITE) {
                entry->dirtyness += 1;
        }

        /* Finally yield the lock */
        mutex_unlock(&dev->iv_cache_lock);

        return page_address(entry->iv_page);


err_create_entry:
        mutex_unlock(&dev->iv_cache_lock);
err_relock_cache:
err_wait_queue:
err_lock_cache:
        return ERR_PTR(err);
}

/* Signal end of usage of an IV block. Decreases the refcount. */
int sflc_dev_putIvBlockRef(sflc_Device * dev, u32 psi)
{
        sflc_dev_IvCacheEntry * entry;
        int err;

        /* No condition needed besides mutual exclusion: just grab the lock (no waitqueue) */
        if (mutex_lock_interruptible(&dev->iv_cache_lock)) {
                pr_err("Interrupted while waiting to lock IV cache\n");
                err = -EINTR;
                goto err_lock_cache;
        }

        /* Retrieve entry */
        entry = dev->iv_cache[psi];

        /* Decrease refcount */
        entry->refcnt -= 1;

        /* Pull it to the the head of the LRU list */
        __list_del_entry(&entry->lru_node);
        list_add(&entry->lru_node, &dev->iv_lru_list);

        /* If cache is not full, we can return now */
        if (dev->iv_cache_nr_entries < SFLC_DEV_IV_CACHE_CAPACITY) {
                goto out;
        }

        /* Otherwise, let's look for the least recent unreffed entry, and evict it */
        sflc_dev_IvCacheEntry * evicted;
        bool found = false;
        list_for_each_entry_reverse(evicted, &dev->iv_lru_list, lru_node) {
                if (evicted->refcnt == 0) {
                        found = true;
                        break;
                }
        }

        /* If we didn't find one (all entries are reffed), no luck, return now */
        if (!found) {
                goto out;
        }

        /* Take it out of the cache */
        dev->iv_cache[evicted->psi] = NULL;
        dev->iv_cache_nr_entries -= 1;
        
        /* Pull it out of the LRU list */
        __list_del_entry(&evicted->lru_node);
        /* Destroy it (free and flush to disk) */
        err = sflc_dev_destroyIvCacheEntry(dev, evicted);
        if (err) {
                pr_err("Could not evict cache entry for PSI %u; error %d\n", evicted->psi, err);
                goto err_destroy_entry;
        }
        /* We just altered the condition someone might be sleeping for: tell the waitqueue */
        wake_up_interruptible(&dev->iv_cache_waitqueue);

out:
        /* Yield the lock */
        mutex_unlock(&dev->iv_cache_lock);

        return 0;


err_destroy_entry:
        /* Add it back to the list */
        list_add(&evicted->lru_node, &dev->iv_lru_list);
        /* Add it back to the cache */
        dev->iv_cache[evicted->psi] = evicted;
        dev->iv_cache_nr_entries+= 1;
        
        mutex_unlock(&dev->iv_cache_lock);
err_lock_cache:
        return err;
}

/* Flush all dirty IV blocks */
void sflc_dev_flushIvs(sflc_Device * dev)
{
	sflc_dev_IvCacheEntry * entry, * _next;
        int err;

        /* Iterate over all entries */
        list_for_each_entry_safe(entry, _next, &dev->iv_lru_list, lru_node) {
                /* Pop it from the list */
                __list_del_entry(&entry->lru_node);

                /* Destroy it */
                err = sflc_dev_destroyIvCacheEntry(dev, entry);
                if (err) {
                        pr_err("Could not destroy IV cache entry for PSI %u; error %d\n", entry->psi, err);
                }
        }
}

/*****************************************************
 *           PRIVATE FUNCTIONS PROTOTYPES            *
 *****************************************************/

static sflc_dev_IvCacheEntry * sflc_dev_newIvCacheEntry(sflc_Device * dev, u32 psi)
{
        sflc_dev_IvCacheEntry * entry;
        int err;
        sector_t sector;

        /* Allocate and init structure */

        /* Allocate structure */
        entry = kmem_cache_alloc(sflc_pools_ivSlab, GFP_NOIO);
        if (!entry) {
                pr_err("Could not allocate IvCacheEntry structure\n");
                err = -ENOMEM;
                goto err_alloc_entry;
        }

        /* Set PSI */
        entry->psi = psi;
        /* Allocate page */
        entry->iv_page = mempool_alloc(sflc_pools_pagePool, GFP_NOIO);
        if (!entry->iv_page) {
                pr_err("Could not allocate IV page\n");
                err = -ENOMEM;
                goto err_alloc_page;
        }
        /* Kmap it */
        kmap(entry->iv_page);

        /* Clear refcount and dirtyness */
        entry->refcnt = 0;
        entry->dirtyness = 0;

        /* Init list node */
        INIT_LIST_HEAD(&entry->lru_node);


        /* Read from disk */

        /* Position on disk */
        sector = sflc_dev_psiToIvBlockSector(psi);

        /* Read */
        err = sflc_dev_rwSector(dev, entry->iv_page, sector, READ);
        if (err) {
                pr_err("Could not read IV block from disk; error %d\n", err);
                goto err_read;
        }

        return entry;


err_read:
        kunmap(entry->iv_page);
        mempool_free(entry->iv_page, sflc_pools_pagePool);
err_alloc_page:
        kmem_cache_free(sflc_pools_ivSlab, entry);
err_alloc_entry:
        return ERR_PTR(err);
}

static int sflc_dev_destroyIvCacheEntry(sflc_Device * dev, sflc_dev_IvCacheEntry * entry)
{
        int err;
        sector_t sector;

        /* Write to disk */

        /* Position on disk */
        sector = sflc_dev_psiToIvBlockSector(entry->psi);

        /* Write (if necessary) */
        if (entry->dirtyness) {
                err = sflc_dev_rwSector(dev, entry->iv_page, sector, WRITE);
                if (err) {
                        pr_err("Could not write IV block to disk; error %d\n", err);
                        return err;
                }
        }


        /* Deinit and free IV cache entry */

        /* Kunmap page */
        kunmap(entry->iv_page);
        /* Free it */
        mempool_free(entry->iv_page, sflc_pools_pagePool);

        /* Free structure */
        kmem_cache_free(sflc_pools_ivSlab, entry);

        return 0;
}
