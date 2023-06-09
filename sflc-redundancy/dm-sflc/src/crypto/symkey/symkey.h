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
 * A thin wrapper around the kernel's synchronous block cipher API.
 */

#ifndef _SFLC_CRYPTO_SYMKEY_SYMKEY_H_
#define _SFLC_CRYPTO_SYMKEY_SYMKEY_H_

/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/

#include <crypto/skcipher.h>
#include <linux/mempool.h>

/*****************************************************
 *                     CONSTANTS                     *
 *****************************************************/

#define SFLC_SK_CIPHER_NAME "ctr(aes)"
#define SFLC_SK_KEY_LEN 32
#define SFLC_SK_IV_LEN 16

/*****************************************************
 *                       TYPES                       *
 *****************************************************/

/**
 * There is one of these Context's for each volume.
 * No need for locking, methods can be called in parallel.
 */
typedef struct sflc_sk_context_s
{
	/* Only one transform for now */
	struct crypto_skcipher        * tfm;

	/* 32-byte key */
	u8                              key[SFLC_SK_KEY_LEN];

	/* Memory pool for skcipher_request's */
	mempool_t                     * sk_req_pool;

} sflc_sk_Context;

/*****************************************************
 *            PUBLIC FUNCTIONS PROTOTYPES            *
 *****************************************************/

/* Self test using known test vectors and random inputs */
int sflc_sk_selftest(void);

/* Create a new context with the given key. Returns an ERR_PTR() on failure. */
sflc_sk_Context * sflc_sk_createContext(u8 * key);
/* Destroy the given context */
void sflc_sk_destroyContext(sflc_sk_Context * ctx);

/* Encrypt/decrypt synchronously. Provide src = dst for in-place operation. */
int sflc_sk_encrypt(sflc_sk_Context * ctx, u8 * src, u8 * dst, unsigned int len, u8 * iv);
int sflc_sk_decrypt(sflc_sk_Context * ctx, u8 * src, u8 * dst, unsigned int len, u8 * iv);


#endif /* _SFLC_CRYPTO_SYMKEY_SYMKEY_H_ */
