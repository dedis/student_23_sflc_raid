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

#include "skreq_pool.h"
#include "log/log.h"

/*****************************************************
 *                     CONSTANTS                     *
 *****************************************************/

/*****************************************************
 *           PRIVATE FUNCTIONS PROTOTYPES            *
 *****************************************************/

/* A mempool_alloc_t using skcipher_request_alloc as backend */
static void * sflc_sk_allocRequest(gfp_t gfp_mask, void * pool_data);
/* A mempool_free_t using skcipher_request_free as backend */
static void sflc_sk_freeRequest(void * element, void * pool_data);

/*****************************************************
 *           PUBLIC FUNCTIONS DEFINITIONS            *
 *****************************************************/

mempool_t * sflc_sk_createReqPool(int min_nr, sflc_sk_Context * ctx)
{
	return mempool_create(min_nr, sflc_sk_allocRequest, sflc_sk_freeRequest, (void *) ctx);
}

/*****************************************************
 *           PRIVATE FUNCTIONS DEFINITIONS           *
 *****************************************************/

/* A mempool_alloc_t using skcipher_request_alloc as backend */
static void * sflc_sk_allocRequest(gfp_t gfp_mask, void * pool_data)
{
	sflc_sk_Context * ctx = pool_data;
	struct skcipher_request * skreq;

	skreq = skcipher_request_alloc(ctx->tfm, gfp_mask);
	if (!skreq) {
		pr_err("Could not allocate skcipher_request");
		return NULL;
	}

	return (void *) skreq;
}

/* A mempool_free_t using skcipher_request_free as backend */
static void sflc_sk_freeRequest(void * element, void * pool_data)
{
	struct skcipher_request * skreq = element;

	skcipher_request_free(skreq);
}
