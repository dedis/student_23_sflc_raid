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

#include <crypto/rng.h>
#include <linux/random.h>

#include "rand.h"
#include "log/log.h"

/*****************************************************
 *                     CONSTANTS                     *
 *****************************************************/

#define SFLC_RAND_RNG_NAME "drbg_nopr_sha256"

/*****************************************************
 *                 PRIVATE VARIABLES                 *
 *****************************************************/

static struct mutex sflc_rand_tfm_lock;
static struct crypto_rng * sflc_rand_tfm = NULL;

/*****************************************************
 *           PRIVATE FUNCTIONS PROTOTYPES            *
 *****************************************************/

/* Flexible to accommodate for both required and non-required reseeding */
static int sflc_rand_reseed(void);

/*****************************************************
 *           PUBLIC FUNCTIONS DEFINITIONS            *
 *****************************************************/

/* Init the submodule */
int sflc_rand_init(void)
{
	int err;

	/* Init the lock governing the SFLC RNG */
	mutex_init(&sflc_rand_tfm_lock);

	/* Allocate module-wide RNG */
	sflc_rand_tfm = crypto_alloc_rng(SFLC_RAND_RNG_NAME, CRYPTO_ALG_TYPE_RNG, 0);
	if (IS_ERR(sflc_rand_tfm)) {
		err = PTR_ERR(sflc_rand_tfm);
		sflc_rand_tfm = NULL;
		pr_err("Could not allocate RNG %s; error %d\n", SFLC_RAND_RNG_NAME, err);
		return err;
	}

	/* The new RNG comes not seeded, right? */
	err = sflc_rand_reseed();
	if (err) {
		pr_err("Could not seed the RNG; error %d\n", err);
		sflc_rand_exit();
		return err;
	}

	return 0;
}

/* Get random bytes. Might sleep for re-seeding (not implemented yet), or for contention (mutex). */
int sflc_rand_getBytes(u8 * buf, unsigned count)
{
	int ret;

	/* Acquire lock */
	if (mutex_lock_interruptible(&sflc_rand_tfm_lock)) {
		pr_err("Got error while waiting for SFLC RNG\n");
		return -EINTR;
	}

	ret = crypto_rng_get_bytes(sflc_rand_tfm, buf, count);

	/* End of critical region */
	mutex_unlock(&sflc_rand_tfm_lock);

	return ret;
}

/* Get a random s32 from 0 (inclusive) to max (exclusive). Returns < 0 if error. */
s32 sflc_rand_uniform(s32 max)
{
	s32 rand;
	s32 thresh;

	/* We'll basically do rand % max, but to avoid
	   skewing the distribution we have to exclude the
	   highest rand's (in absolute value). */
	thresh = __INT32_MAX__ - (__INT32_MAX__ % max);
	do {
		/* Sample a random positive integer */
		int err = sflc_rand_getBytes((void *) &rand, sizeof(rand));
		if (rand < 0) {
			rand = -rand;
		}

		if (err) {
			pr_err("Got error when sampling random number");
			return -1;
		}
	} while(rand >= thresh);

	return rand % max;
}

/* Tear down the submodule */
void sflc_rand_exit(void)
{
	if (sflc_rand_tfm) {
		crypto_free_rng(sflc_rand_tfm);
		sflc_rand_tfm = NULL;
	}

	return;
}

/*****************************************************
 *           PRIVATE FUNCTIONS DEFINITIONS           *
 *****************************************************/

/* Flexible to accommodate for both required and non-required reseeding */
static int sflc_rand_reseed(void)
{
	int err;

	/* Reseed the RNG */
	err = crypto_rng_reset(sflc_rand_tfm, NULL, crypto_rng_seedsize(sflc_rand_tfm));
	if (err) {
		pr_err("Could not feed seed to the RNG; error %d\n", err);
		return err;
	}

	return 0;
}
