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

/*
 * Crypto functions for Shufflecake.
 */

#ifndef _CRYPTO_H
#define _CRYPTO_H

#include "sodium.h"

#include "utils.h"

#define SFLC_USR_SALT_LEN crypto_pwhash_SALTBYTES    /* 16 bytes */
#define SFLC_USR_KEY_LEN crypto_secretbox_KEYBYTES   /* 32 bytes */
#define SFLC_USR_IV_LEN crypto_secretbox_NONCEBYTES  /* 24 bytes */
#define SFLC_USR_MAC_LEN crypto_secretbox_MACBYTES   /* 16 bytes */

void init_crypto();
void cleanup_crypto();

/* Argon key derivation */
int kdf_pwhash(char * pwd, int key_len, char * key, char * salt);

/* Encryption & decryption utilities */

/* Always succeeds */
void ae_encrypt(char * pt, unsigned pt_len, char * ct, char * mac, char * key, char * iv);
/* Returns 0 if successful, -1 if MAC verification failed */
int ae_decrypt(char * ct, unsigned ct_len, char * mac, char * pt, char * key, char * iv);


#endif /* _CRYPTO_H */
