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

#include "pools.h"
#include "log/log.h"

/*****************************************************
 *                     CONSTANTS                     *
 *****************************************************/

/* Pool sizes */
#define SFLC_POOLS_BIOSET_POOL_SIZE 1024
#define SFLC_POOLS_PAGE_POOL_SIZE 1024
#define SFLC_POOLS_WRITE_WORK_POOL_SIZE 1024
#define SFLC_POOLS_DECRYPT_WORK_POOL_SIZE 1024

/* Slab cache names */
#define SFLC_POOLS_WRITE_WORK_SLAB_NAME "sflc_write_work_slab"
#define SFLC_POOLS_DECRYPT_WORK_SLAB_NAME "sflc_decrypt_work_slab"
#define SFLC_POOLS_IV_SLAB_NAME "sflc_iv_slab"

/*****************************************************
 *           PUBLIC VARIABLES DEFINITIONS            *
 *****************************************************/

struct bio_set sflc_pools_bioset;
mempool_t * sflc_pools_pagePool;
mempool_t * sflc_pools_writeWorkPool;
mempool_t * sflc_pools_decryptWorkPool;
struct kmem_cache * sflc_pools_ivSlab;

/*****************************************************
 *                 PRIVATE VARIABLES                 *
 *****************************************************/

static struct kmem_cache * sflc_pools_writeWorkSlab;
static struct kmem_cache * sflc_pools_decryptWorkSlab;

/*****************************************************
 *           PUBLIC FUNCTIONS DEFINITIONS            *
 *****************************************************/

int sflc_pools_init(void)
{
	int err;

	/* Memory pools: bioset */
	err = bioset_init(&sflc_pools_bioset, SFLC_POOLS_BIOSET_POOL_SIZE, 0, BIOSET_NEED_BVECS);
	if (err) {
		pr_err("Could not init bioset: error %d\n", err);
		goto err_bioset;
	}

	/* Memory pools: page_pool */
	sflc_pools_pagePool = mempool_create_page_pool(SFLC_POOLS_PAGE_POOL_SIZE, 0);
	if (!sflc_pools_pagePool) {
		pr_err("Could not create page pool\n");
		err = -ENOMEM;
                goto err_pagepool;
	}

        /* Memory pools: writeWork slab cache */
        sflc_pools_writeWorkSlab = kmem_cache_create(SFLC_POOLS_WRITE_WORK_SLAB_NAME, sizeof(sflc_vol_WriteWork), 0, SLAB_POISON | SLAB_RED_ZONE, NULL);
        if (IS_ERR(sflc_pools_writeWorkSlab)) {
                err = PTR_ERR(sflc_pools_writeWorkSlab);
                pr_err("Could not create writeWork slab cache; error %d\n", err);
                goto err_create_write_work_slab;
        }

        /* Memory pools: writeWork pool */
        sflc_pools_writeWorkPool = mempool_create_slab_pool(SFLC_POOLS_WRITE_WORK_POOL_SIZE, sflc_pools_writeWorkSlab);
        if (!sflc_pools_writeWorkPool) {
                pr_err("Could not create writeWork pool\n");
                err = -ENOMEM;
                goto err_write_work_pool;
        }

        /* Memory pools: decryptWork slab cache */
        sflc_pools_decryptWorkSlab = kmem_cache_create(SFLC_POOLS_DECRYPT_WORK_SLAB_NAME, sizeof(sflc_vol_DecryptWork), 0, SLAB_POISON | SLAB_RED_ZONE, NULL);
        if (IS_ERR(sflc_pools_decryptWorkSlab)) {
                err = PTR_ERR(sflc_pools_decryptWorkSlab);
                pr_err("Could not create decryptWork slab cache; error %d\n", err);
                goto err_create_decrypt_work_slab;
        }

        /* Memory pools: decryptWork pool */
        sflc_pools_decryptWorkPool = mempool_create_slab_pool(SFLC_POOLS_DECRYPT_WORK_POOL_SIZE, sflc_pools_decryptWorkSlab);
        if (!sflc_pools_decryptWorkPool) {
                pr_err("Could not create decryptWork pool\n");
                err = -ENOMEM;
                goto err_decrypt_work_pool;
        }

	/* Memory pools: IV slab cache */
        sflc_pools_ivSlab = kmem_cache_create(SFLC_POOLS_IV_SLAB_NAME, sizeof(sflc_dev_IvCacheEntry), 0, SLAB_POISON | SLAB_RED_ZONE, NULL);
        if (IS_ERR(sflc_pools_ivSlab)) {
                err = PTR_ERR(sflc_pools_ivSlab);
                pr_err("Could not create IV slab cache; error %d\n", err);
                goto err_create_iv_slab;
        }

        return 0;


err_create_iv_slab:
        mempool_destroy(sflc_pools_decryptWorkPool);
err_decrypt_work_pool:
        kmem_cache_destroy(sflc_pools_decryptWorkSlab);
err_create_decrypt_work_slab:
        mempool_destroy(sflc_pools_writeWorkPool);
err_write_work_pool:
        kmem_cache_destroy(sflc_pools_writeWorkSlab);
err_create_write_work_slab:
        mempool_destroy(sflc_pools_pagePool);
err_pagepool:
        bioset_exit(&sflc_pools_bioset);
err_bioset:
	return err;
}

void sflc_pools_exit(void)
{
        kmem_cache_destroy(sflc_pools_ivSlab);
        mempool_destroy(sflc_pools_decryptWorkPool);
        kmem_cache_destroy(sflc_pools_decryptWorkSlab);
        mempool_destroy(sflc_pools_writeWorkPool);
        kmem_cache_destroy(sflc_pools_writeWorkSlab);
        mempool_destroy(sflc_pools_pagePool);
        bioset_exit(&sflc_pools_bioset);
}
