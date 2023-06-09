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

#include <assert.h>

#include "crypto.h"

/* Initializes Sodium. */
void init_crypto() {
    /* Use internal RNG, instead of /dev/urandom */
    randombytes_set_implementation(&randombytes_internal_implementation);
    if (sodium_init() < 0) {
        die("ERR: Sodium init failed");
    }
}

/* Cleans up Sodium. */
void cleanup_crypto() {

}


/*
 * Key derivation
 */

/* Derives a key from a password with Argon */
int kdf_pwhash(char * pass, int key_len, char * key, char * salt) {
    return crypto_pwhash(key, key_len, pass, strlen(pass), salt, 
                        crypto_pwhash_OPSLIMIT_MIN, crypto_pwhash_MEMLIMIT_MIN, 
                        crypto_pwhash_ALG_ARGON2ID13);
}

/* 
 * Encryption and decryption. Use XSalsa20 + Poly1305
 */

/* Always succeeds. */
void ae_encrypt(char * pt, unsigned pt_len, char * ct, char * mac, char * key, char * iv)
{
    crypto_secretbox_detached(ct, mac, pt, pt_len, iv, key);
}

/* Returns 0 if successful, -1 if MAC verification failed */
int ae_decrypt(char * ct, unsigned ct_len, char * mac, char * pt, char * key, char * iv)
{
    return crypto_secretbox_open_detached(pt, ct, mac, ct_len, iv, key);
}

