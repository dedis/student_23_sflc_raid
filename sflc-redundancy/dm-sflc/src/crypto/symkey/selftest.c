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
#include <linux/random.h>

#include "symkey.h"
#include "log/log.h"

/*****************************************************
 *                     CONSTANTS                     *
 *****************************************************/

#define KEYS    \
{   \
	{0xF6, 0xD6, 0x6D, 0x6B, 0xD5, 0x2D, 0x59, 0xBB, 0x07, 0x96, 0x36, 0x58, 0x79, 0xEF, 0xF8, 0x86,    \
	 0xC6, 0x6D, 0xD5, 0x1A, 0x5B, 0x6A, 0x99, 0x74, 0x4B, 0x50, 0x59, 0x0C, 0x87, 0xA2, 0x38, 0x84},   \
	\
	{0xFF, 0x7A, 0x61, 0x7C, 0xE6, 0x91, 0x48, 0xE4, 0xF1, 0x72, 0x6E, 0x2F, 0x43, 0x58, 0x1D, 0xE2,    \
	 0xAA, 0x62, 0xD9, 0xF8, 0x05, 0x53, 0x2E, 0xDF, 0xF1, 0xEE, 0xD6, 0x87, 0xFB, 0x54, 0x15, 0x3D},   \
}

#define IVS \
{   \
	{0x00, 0xFA, 0xAC, 0x24, 0xC1, 0x58, 0x5E, 0xF1, 0x5A, 0x43, 0xD8, 0x75, 0x00, 0x00, 0x00, 0x01},   \
	\
	{0x00, 0x1C, 0xC5, 0xB7, 0x51, 0xA5, 0x1D, 0x70, 0xA1, 0xC1, 0x11, 0x48, 0x00, 0x00, 0x00, 0x01},   \
}

#define PLAINTEXTS  \
{   \
	{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,    \
	 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F},   \
	\
	{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,    \
	 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F},   \
}

#define CIPHERTEXTS \
{   \
	{0xF0, 0x5E, 0x23, 0x1B, 0x38, 0x94, 0x61, 0x2C, 0x49, 0xEE, 0x00, 0x0B, 0x80, 0x4E, 0xB2, 0xA9,    \
	 0xB8, 0x30, 0x6B, 0x50, 0x8F, 0x83, 0x9D, 0x6A, 0x55, 0x30, 0x83, 0x1D, 0x93, 0x44, 0xAF, 0x1C},   \
	\
	{0xEB, 0x6C, 0x52, 0x82, 0x1D, 0x0B, 0xBB, 0xF7, 0xCE, 0x75, 0x94, 0x46, 0x2A, 0xCA, 0x4F, 0xAA,    \
	 0xB4, 0x07, 0xDF, 0x86, 0x65, 0x69, 0xFD, 0x07, 0xF4, 0x8C, 0xC0, 0xB5, 0x83, 0xD6, 0x07, 0x1F},   \
}

/*****************************************************
 *                 PRIVATE VARIABLES                 *
 *****************************************************/

/*****************************************************
 *           PRIVATE FUNCTIONS PROTOTYPES            *
 *****************************************************/

/* Test encryption or decryption with known test vectors */
static int testEncdec(bool encrypt);
/* Test that encryption and decryption invert each other */
static int testRand(void);
static void dumpHex(u8 * buf, unsigned count);

/*****************************************************
 *           PUBLIC FUNCTIONS DEFINITIONS            *
 *****************************************************/

/* Self test using known test vectors and random inputs */
int sflc_sk_selftest(void)
{
	int err;

	/* Test encryption */
	err = testEncdec(true);
	if (err) {
		pr_err("Error in encryption test: %d\n", err);
		return err;
	}

	/* Test decryption */
	err = testEncdec(false);
	if (err) {
		pr_err("Error in decryption test: %d\n", err);
		return err;
	}

	/* Test with random inputs */
	err = testRand();
	if (err) {
		pr_err("Error in random test: %d\n", err);
		return err;
	}

	pr_info("All good in crypto symkey selftest\n");
	return 0;
}

/*****************************************************
 *           PRIVATE FUNCTIONS DEFINITIONS            *
 *****************************************************/

/* Test encryption or decryption with known test vectors */
static int testEncdec(bool encrypt)
{
	sflc_sk_Context * ctx[2];
	u8 scratchpad[32];
	u8 key[2][32] = KEYS;
	u8 iv[2][16] = IVS;
	u8 pt[2][32] = PLAINTEXTS;
	u8 ct[2][32] = CIPHERTEXTS;
	int err;

	int i;
	for (i = 0; i < 2; i++) {
		memset(scratchpad, 0xa7, 32);

		ctx[i] = sflc_sk_createContext(key[i]);
		if (IS_ERR(ctx)) {
			err = PTR_ERR(ctx[i]);
			pr_err("Could not create sk context; error %d\n", err);
			return err;
		}

		if (encrypt) {
			err = sflc_sk_encrypt(ctx[i], pt[i], scratchpad, 32, iv[i]);
			if (err) {
				pr_err("Failure during encryption %d; error %d\n", i, err);
				sflc_sk_destroyContext(ctx[i]);
				return err;
			}
			if(memcmp(scratchpad, ct[i], 32) != 0) {
				pr_err("Mismatch for encryption %d\n", i);
				dumpHex(scratchpad, 16);
				sflc_sk_destroyContext(ctx[i]);
				return -EINVAL;
			}
		}
		else /* decrypt*/ {
			err = sflc_sk_decrypt(ctx[i], ct[i], scratchpad, 32, iv[i]);
			if (err) {
				pr_err("Failure during decryption %d; error %d\n", i, err);
				sflc_sk_destroyContext(ctx[i]);
				return err;
			}
			if (memcmp(scratchpad, pt[i], 32) != 0) {
				pr_err("Mismatch for decryption %d\n", i);
				dumpHex(scratchpad, 32);
				sflc_sk_destroyContext(ctx[i]);
				return -EINVAL;
			}
		}

		 sflc_sk_destroyContext(ctx[i]);
	}

	return 0;
}

/* Test that encryption and decryption invert each other */
static int testRand(void)
{
	u8 pt[48];
	u8 scratchpad[48];
	u8 key[32];
	u8 iv[16];
	sflc_sk_Context * ctx;
	int err;

	get_random_bytes(key, 32);
	ctx = sflc_sk_createContext(key);
	if (IS_ERR(ctx)) {
		err = PTR_ERR(ctx);
		pr_err("Could not create context; error %d\n", err);
		return err;
	}
	memset(iv, 0, 16);

	int i;
	for (i = 0; i < 200; i++) {
		get_random_bytes(pt, 48);
		err = sflc_sk_encrypt(ctx, pt, scratchpad, 48, iv);
		if (err) {
			pr_err("Could not encrypt; error %d\n", err);
			sflc_sk_destroyContext(ctx);
			return err;
		}
		if (memcmp(pt, scratchpad, 48) == 0) {
			pr_err("Random iteration %d; pt=scratchpad\n", i);
			sflc_sk_destroyContext(ctx);
			return -EINVAL;
		}

		/* Reset IV */
		iv[15] = 0;

		err = sflc_sk_decrypt(ctx, scratchpad, scratchpad, 48, iv);
		if (err) {
			pr_err("Could not decrypt; error %d\n", err);
			sflc_sk_destroyContext(ctx);
			return err;
		}
		if (memcmp(pt, scratchpad, 48) != 0) {
			pr_err("Random iteration %d; mismatch. Dumping plaintext and scratchpad\n", i);
			dumpHex(pt, 48);
			dumpHex(scratchpad, 48);
			sflc_sk_destroyContext(ctx);
			return -EINVAL;
		}

		/* Reset IV */
		iv[15] = 0;
	}

	sflc_sk_destroyContext(ctx);
	return 0;
}

static void dumpHex(u8 * buf, unsigned count)
{
	char * hex;

	hex = kmalloc(6*count + 1, GFP_KERNEL);
	if (!hex) {
		pr_err("Could not allocate hex dump string\n");
		return;
	}

	int i;
	for (i = 0; i < count; i++) {
		sprintf(hex+6*i, "0x%02X, ", buf[i]);
	}

	pr_notice("---- Hex dump ----\n");
	pr_notice("%s", hex);
	pr_notice("---- End of hex dump ----\n");

	kfree(hex);
	return;
}
