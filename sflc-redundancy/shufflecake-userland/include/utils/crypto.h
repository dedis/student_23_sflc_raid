/*
 *  Copyright The Shufflecake Project Authors (2022)
 *  Copyright The Shufflecake Project Contributors (2022)
 *  Copyright Contributors to the The Shufflecake Project.
 *  
 *  See the AUTHORS file at the top-level directory of this distribution and at
 *  <https://www.shufflecake.net/permalinks/shufflecake-userland/AUTHORS>
 *  
 *  This file is part of the program shufflecake-userland, which is part of the 
 *  Shufflecake Project. Shufflecake is a plausible deniability (hidden storage) 
 *  layer for Linux. See <https://www.shufflecake.net>.
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

#ifndef _UTILS_CRYPTO_H_
#define _UTILS_CRYPTO_H_


/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "utils/sflc.h"


/*****************************************************
 *                    CONSTANTS                      *
 *****************************************************/

// Key length, for input into AES-CTR and AES-GCM, and for output from Scrypt
#define SFLC_CRYPTO_KEYLEN		32	/* bytes */

// IV length for AES-GCM
#define SFLC_AESGCM_IVLEN		12	/* bytes */
// MAC length for AES-GCM
#define SFLC_AESGCM_TAGLEN		16	/* bytes */
// Content of output plaintext upon MAC verification failure
#define SFLC_AESGCM_POISON_PT	0xFF

/* Scrypt parameters */

// Scrypt salt length
#define SFLC_SCRYPT_SALTLEN		16	/* bytes */
// Scrypt CPU/memory cost parameter. Tweakable via Makefile.
#ifndef SFLC_SCRYPT_N
	#define SFLC_SCRYPT_N		(1 << 15)
#endif
// Scrypt parallelisation parameter. Tweakable via Makefile.
#ifndef SFLC_SCRYPT_P
	#define SFLC_SCRYPT_P		1
#endif


/*****************************************************
 *           PUBLIC FUNCTIONS PROTOTYPES             *
 *****************************************************/

/* Get slow, true random bytes (suited for keys) */
int sflc_rand_getStrongBytes(char *buf, size_t buflen);
/* Get fast, pseudo random bytes (suited for IVs and padding) */
int sflc_rand_getWeakBytes(char *buf, size_t buflen);

/* AES256-CTR encryption, does not touch the IV. Set ct = NULL for in-place. */
int sflc_aes256ctr_encrypt(char *key, char *pt, size_t pt_len, char *iv, char *ct);
/* AES256-CTR decryption, does not touch the IV. Set pt = NULL for in-place. */
int sflc_aes256ctr_decrypt(char *key, char *ct, size_t ct_len, char *iv, char *pt);

/* AES256-GCM encryption, does not touch the IV */
int sflc_aes256gcm_encrypt(char *key, char *pt, size_t pt_len, char *iv, char *ct, char *tag);
/* AES256-GCM decryption, does not touch the IV (only decrypts if MAC is valid) */
int sflc_aes256gcm_decrypt(char *key, char *ct, size_t ct_len, char *tag, char *iv, char *pt, bool *match);

/* Compute Scrypt KDF */
int sflc_scrypt_derive(char *pwd, size_t pwd_len, char *salt, char *hash);


#endif /* _UTILS_CRYPTO_H_ */
