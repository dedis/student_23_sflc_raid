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

#include <linux/scatterlist.h>

#include "symkey.h"
#include "skreq_pool.h"
#include "log/log.h"

/*****************************************************
 *                     CONSTANTS                     *
 *****************************************************/

#define SFLC_SK_REQ_POOL_SIZE 1024

#define SFLC_SK_ENCRYPT 0
#define SFLC_SK_DECRYPT 1

/*****************************************************
 *           PRIVATE FUNCTIONS PROTOTYPES            *
 *****************************************************/

static int sflc_sk_encdec(sflc_sk_Context * ctx, u8 * src, u8 * dst, unsigned int len, u8 * iv, int op);

/*****************************************************
 *           PUBLIC FUNCTIONS DEFINITIONS            *
 *****************************************************/

/* Create a new context with the given key. Returns an ERR_PTR() on failure. */
sflc_sk_Context * sflc_sk_createContext(u8 * key)
{
	sflc_sk_Context * ctx;
	int err;

	/* Allocate context */
	ctx = kzalloc(sizeof(sflc_sk_Context), GFP_KERNEL);
	if (!ctx) {
		pr_err("Could not allocate %lu bytes for the sflc_sk_Context\n", sizeof(sflc_sk_Context));
		return ERR_PTR(-ENOMEM);
	}

	/* Allocate crypto transform */
	ctx->tfm = crypto_alloc_skcipher(SFLC_SK_CIPHER_NAME, CRYPTO_ALG_ASYNC, 0);
	if (IS_ERR(ctx->tfm)) {
		err = PTR_ERR(ctx->tfm);
		ctx->tfm = NULL;
		pr_err("Could not allocate skcipher handle: error %d\n", err);
		sflc_sk_destroyContext(ctx);
		return ERR_PTR(err);
	}

	/* Copy and set key */
	memcpy(ctx->key, key, SFLC_SK_KEY_LEN);
	err = crypto_skcipher_setkey(ctx->tfm, ctx->key, SFLC_SK_KEY_LEN);
	if (err) {
		pr_err("Could not set key in crypto transform: error %d\n", err);
		sflc_sk_destroyContext(ctx);
		return ERR_PTR(err);
	}

	/* Create request memory pool */
	ctx->sk_req_pool = sflc_sk_createReqPool(SFLC_SK_REQ_POOL_SIZE, ctx);
	if (!ctx->sk_req_pool) {
		pr_err("Could not allocate skcipher_request memory pool\n");
		sflc_sk_destroyContext(ctx);
		return ERR_PTR(-ENOMEM);
	}

	return ctx;
}

/* Destroy the given context */
void sflc_sk_destroyContext(sflc_sk_Context * ctx)
{
	if (!ctx) {
		return;
	}

	if (ctx->sk_req_pool) {
		mempool_destroy(ctx->sk_req_pool);
		ctx->sk_req_pool = NULL;
	}

	if (ctx->tfm) {
		crypto_free_skcipher(ctx->tfm);
		ctx->tfm = NULL;
	}

	kfree(ctx);

	return;
}

/* Encrypt synchronously. Provide src = dst for in-place operation. */
int sflc_sk_encrypt(sflc_sk_Context * ctx, u8 * src, u8 * dst, unsigned int len, u8 * iv)
{
	return sflc_sk_encdec(ctx, src, dst, len, iv, SFLC_SK_ENCRYPT);
}

int sflc_sk_decrypt(sflc_sk_Context * ctx, u8 * src, u8 * dst, unsigned int len, u8 * iv)
{
	return sflc_sk_encdec(ctx, src, dst, len, iv, SFLC_SK_DECRYPT);
}

/*****************************************************
 *           PRIVATE FUNCTIONS DEFINITIONS           *
 *****************************************************/

static int sflc_sk_encdec(sflc_sk_Context * ctx, u8 * src, u8 * dst, unsigned int len, u8 * iv, int op)
{
	struct skcipher_request * skreq;
	struct scatterlist srcsg;
	struct scatterlist dstsg, * p_dstsg;
	DECLARE_CRYPTO_WAIT(skreq_wait);
	bool in_place = (dst == src);
	int ret;

	/* Provide just one scatterlist if in place */
	p_dstsg = in_place ? &srcsg : &dstsg;

	/* Allocate request (will already have the ctx's tfm into it) */
	skreq = mempool_alloc(ctx->sk_req_pool, GFP_NOIO);
	if (!skreq) {
		pr_err("Could not allocate skcipher_request\n");
		return -ENOMEM;
	}

	/* Set src and dst scatterlist */
	sg_init_one(&srcsg, src, len);
	if (!in_place) {
		sg_init_one(&dstsg, dst, len);
	}

	/* Chuck all into the skreq */
	skcipher_request_set_crypt(skreq, &srcsg, p_dstsg, len, iv);
	/* Also set the basic callback that just waits for completion */
	skcipher_request_set_callback(skreq, CRYPTO_TFM_REQ_MAY_SLEEP | CRYPTO_TFM_REQ_MAY_BACKLOG, 
					crypto_req_done, &skreq_wait);

	/* Do it */
	if (op == SFLC_SK_ENCRYPT) {
		ret = crypto_skcipher_encrypt(skreq);
	} else {
		ret = crypto_skcipher_decrypt(skreq);
	}

	/* Wait for completion */
	ret = crypto_wait_req(ret, &skreq_wait);

	/* Free the request */
	mempool_free(skreq, ctx->sk_req_pool);

	return ret;
}
