/*
 * Copyright The Shufflecake Project Authors (2022)
 * Copyright The Shufflecake Project Contributors (2022)
 * Copyright Contributors to the The Shufflecake Project.
 *
 * See the AUTHORS file at the top-level directory of this distribution and at
 * <https://www.shufflecake.net/permalinks/shufflecake-userland/AUTHORS>
 *
 * This file is part of the program shufflecake-userland, which is part of the
 * Shufflecake Project. Shufflecake is a plausible deniability (hidden storage)
 * layer for Linux. See <https://www.shufflecake.net>.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version. This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details. You should have received a copy of the
 * GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 */

/*****************************************************
 *                 INCLUDE SECTION                  *
 *****************************************************/

#include <gcrypt.h>

#include "utils/crypto.h"
#include "utils/log.h"


/*****************************************************
 *          PUBLIC FUNCTIONS DEFINITIONS            *
 *****************************************************/

/**
 *Fills the given buffer with strong random bytes, suitable for long-term
 *cryptographic keys (uses the entropy pool). Always succeeds.
 *
 *@param buf The buffer to fill
 *@param buflen The number of desired random bytes
 *
 *@return The error code (0 on success)
 */
int sflc_rand_getStrongBytes(char *buf, size_t buflen)
{
	gcry_randomize(buf, buflen, GCRY_VERY_STRONG_RANDOM);
	return 0;
}


/**
 *Faster than the previous one, fills the given buffer with weak random bytes,
 *suitable for IVs or block filling (does not use the entropy pool).
 *Always succeeds.
 *
 *@param buf The buffer to fill
 *@param buflen The number of desired random bytes
 *
 *@return The error code (0 on success)
 */
int sflc_rand_getWeakBytes(char *buf, size_t buflen)
{
	gcry_create_nonce(buf, buflen);
	return 0;
}


/**
 *AES-CTR encryption, does not touch the IV. Set ct = NULL for in-place.
 *
 *@param key The 32-byte AES key
 *@param pt The plaintext
 *@param pt_len The length of the plaintext, must be a multiple of the AES
 * block size (16 bytes)
 *@param iv The 16-byte AES-CTR IV
 *@param ct A caller-allocated buffer that will contain the output ciphertext,
 * cannot overlap with pt. If NULL, in-place encryption.
 *
 *@return The error code (0 on success)
 */
int sflc_aes256ctr_encrypt(char *key, char *pt, size_t pt_len, char *iv, char *ct)
{
	gcry_cipher_hd_t hd;
	gcry_error_t err;

	// Instantiate the handle
	err = gcry_cipher_open(&hd, GCRY_CIPHER_AES256, GCRY_CIPHER_MODE_CTR, GCRY_CIPHER_SECURE);
	if (err) {
		sflc_log_error("Could not instantiate AES256-CTR cipher handle: error %d", err);
		goto bad_open;
	}
	sflc_log_debug("Successfully instantiated AES256-CTR cipher handle");

	// Set the key
	err = gcry_cipher_setkey(hd, key, SFLC_CRYPTO_KEYLEN);
	if (err) {
		sflc_log_error("Could not set AES key: error %d", err);
		goto bad_setkey;
	}
	sflc_log_debug("Successfully set the AES key");

	// Set the counter (not an IV, as per Gcrypt docs)
	err = gcry_cipher_setctr(hd, iv, SFLC_AESCTR_IVLEN);
	if (err) {
		sflc_log_error("Could not set AES-CTR IV: error %d", err);
		goto bad_setctr;
	}
	sflc_log_debug("Successfully set the IV");

	// Encrypt
	if (ct == NULL) {	// In-place
		err = gcry_cipher_encrypt(hd, pt, pt_len, NULL, 0);
	}
	else {	// Out-of-place
		err = gcry_cipher_encrypt(hd, ct, pt_len, pt, pt_len);
	}
	// Error check
	if (err) {
		sflc_log_error("Could not encrypt: error %d", err);
		goto bad_encrypt;
	}
	sflc_log_debug("Successfully encrypted");

	// No prob?
	err = 0;


bad_encrypt:
bad_setctr:
bad_setkey:
	gcry_cipher_close(hd);
bad_open:
	return err;
}

/**
 *AES-CTR decryption, does not touch the IV. Set pt = NULL for in-place.
 *
 *@param key The 32-byte AES key
 *@param ct The ciphertext
 *@param ct_len The length of the ciphertext, must be a multiple of the AES
 * block size (16 bytes)
 *@param iv The 16-byte AES-CTR IV
 *@param pt A caller-allocated buffer that will contain the output plaintext,
 * cannot overlap with ct. If NULL, in-place decryption.
 *
 *@return The error code (0 on success)
 */
int sflc_aes256ctr_decrypt(char *key, char *ct, size_t ct_len, char *iv, char *pt)
{
	gcry_cipher_hd_t hd;
	gcry_error_t err;

	// Instantiate the handle
	err = gcry_cipher_open(&hd, GCRY_CIPHER_AES256, GCRY_CIPHER_MODE_CTR, GCRY_CIPHER_SECURE);
	if (err) {
		sflc_log_error("Could not instantiate AES256-CTR cipher handle: error %d", err);
		goto bad_open;
	}
	sflc_log_debug("Successfully instantiated AES256-CTR cipher handle");

	// Set the key
	err = gcry_cipher_setkey(hd, key, SFLC_CRYPTO_KEYLEN);
	if (err) {
		sflc_log_error("Could not set AES key: error %d", err);
		goto bad_setkey;
	}
	sflc_log_debug("Successfully set AES key");

	// Set the counter (not an IV, as per Gcrypt docs)
	err = gcry_cipher_setctr(hd, iv, SFLC_AESCTR_IVLEN);
	if (err) {
		sflc_log_error("Could not set AES-CTR IV: error %d", err);
		goto bad_setctr;
	}
	sflc_log_debug("Successfully set IV");

	// Decrypt
	if (pt == NULL) {	// In-place
		err = gcry_cipher_decrypt(hd, ct, ct_len, NULL, 0);
	}
	else {	// Out-of-place
		err = gcry_cipher_decrypt(hd, pt, ct_len, ct, ct_len);
	}
	// Error check
	if (err) {
		sflc_log_error("Could not decrypt: error %d", err);
		goto bad_decrypt;
	}
	sflc_log_debug("Successfully decrypted");

	// No prob
	err = 0;


bad_decrypt:
bad_setctr:
bad_setkey:
	gcry_cipher_close(hd);
bad_open:
	return err;
}


/**
 *AES-GCM encryption, does not touch the IV.
 *
 *@param key The 32-byte AES key
 *@param pt The plaintext
 *@param pt_len The length of the plaintext, must be a multiple of the AES
 * block size (16 bytes)
 *@param iv The 12-byte AES-GCM IV
 *@param ct A caller-allocated buffer that will contain the output ciphertext,
 * cannot overlap with pt
 *@param tag A caller-allocated buffer that will contain the 16-byte output MAC
 *
 *@return The error code (0 on success)
 */
int sflc_aes256gcm_encrypt(char *key, char *pt, size_t pt_len, char *iv, char *ct, char *tag)
{
	gcry_cipher_hd_t hd;
	gcry_error_t err;

	// Instantiate the handle
	err = gcry_cipher_open(&hd, GCRY_CIPHER_AES256, GCRY_CIPHER_MODE_GCM, GCRY_CIPHER_SECURE);
	if (err) {
		sflc_log_error("Could not instantiate AES256-GCM cipher handle: error %d", err);
		goto bad_open;
	}
	sflc_log_debug("Successfully instantiated AES256-GCM cipher handle");

	// Set the key
	err = gcry_cipher_setkey(hd, key, SFLC_CRYPTO_KEYLEN);
	if (err) {
		sflc_log_error("Could not set AES key: error %d", err);
		goto bad_setkey;
	}
	sflc_log_debug("Successfully set the AES key");

	// Set the IV
	err = gcry_cipher_setiv(hd, iv, SFLC_AESGCM_IVLEN);
	if (err) {
		sflc_log_error("Could not set AES-GCM IV: error %d", err);
		goto bad_setiv;
	}
	sflc_log_debug("Successfully set the IV");

	// Encrypt
	err = gcry_cipher_encrypt(hd, ct, pt_len, pt, pt_len);
	if (err) {
		sflc_log_error("Could not encrypt: error %d", err);
		goto bad_encrypt;
	}
	sflc_log_debug("Successfully encrypted");

	// Get MAC
	err = gcry_cipher_gettag(hd, tag, SFLC_AESGCM_TAGLEN);
	if (err) {
		sflc_log_error("Could not get MAC: error %d", err);
		goto bad_gettag;
	}
	sflc_log_debug("Successfully gotten MAC");

	// No prob?
	err = 0;


bad_gettag:
bad_encrypt:
bad_setiv:
bad_setkey:
	gcry_cipher_close(hd);
bad_open:
	return err;
}


/**
 *AES-GCM decryption, does not touch the IV.
 *
 *@param key The 32-byte AES key
 *@param ct The ciphertext
 *@param ct_len The length of the ciphertext, must be a multiple of the AES
 * block size (16 bytes)
 *@param tag The 16-byte MAC
 *@param iv The 12-byte AES-GCM IV
 *@param pt A caller-allocated buffer that will contain the output plaintext,
 * cannot overlap with ct
 *@param match A pointer to a single output boolean, indicating MAC verification
 * success or failure. On failure, pt is filled with poison bytes.
 *
 *@return The error code (0 on success)
 */
int sflc_aes256gcm_decrypt(char *key, char *ct, size_t ct_len, char *tag, char *iv, char *pt, bool *match)
{
	gcry_cipher_hd_t hd;
	gcry_error_t err;

	// Instantiate the handle
	err = gcry_cipher_open(&hd, GCRY_CIPHER_AES256, GCRY_CIPHER_MODE_GCM, GCRY_CIPHER_SECURE);
	if (err) {
		sflc_log_error("Could not instantiate AES256-GCM cipher handle: error %d", err);
		goto bad_open;
	}
	sflc_log_debug("Successfully instantiated AES256-GCM cipher handle");

	// Set the key
	err = gcry_cipher_setkey(hd, key, SFLC_CRYPTO_KEYLEN);
	if (err) {
		sflc_log_error("Could not set AES key: error %d", err);
		goto bad_setkey;
	}
	sflc_log_debug("Successfully set AES key");

	// Set the IV
	err = gcry_cipher_setiv(hd, iv, SFLC_AESGCM_IVLEN);
	if (err) {
		sflc_log_error("Could not set AES-GCM IV: error %d", err);
		goto bad_setiv;
	}
	sflc_log_debug("Successfully set IV");

	// Decrypt
	err = gcry_cipher_decrypt(hd, pt, ct_len, ct, ct_len);
	if (err) {
		sflc_log_error("Could not decrypt: error %d", err);
		goto bad_decrypt;
	}
	sflc_log_debug("Successfully decrypted");

	// Check MAC
	err = gcry_cipher_checktag(hd, tag, SFLC_AESGCM_TAGLEN);
	if (gcry_err_code(err) == GPG_ERR_CHECKSUM) {
		// Undo decryption
		memset(pt, SFLC_AESGCM_POISON_PT, ct_len);
		// Flag it
		*match = false;
	}
	else if (err) {
		sflc_log_error("Could not check MAC: error %d", err);
		goto bad_checktag;
	}
	else {
		// Flag MAC verification success
		*match = true;
	}
	sflc_log_debug("Successfully checked MAC: match = %d", *match);

	// No prob, whether MAC verified or not
	err = 0;


bad_checktag:
bad_decrypt:
bad_setiv:
bad_setkey:
	gcry_cipher_close(hd);
bad_open:
	return err;
}


/**
 *Scrypt KDF.
 *
 *@param pwd The password
 *@param pwd_len The length of the password
 *@param salt The 32-byte KDF salt
 *@param hash A caller-allocated 32-byte output buffer that will contain the
 * password hash
 *
 *@return The error code (0 on success)
 */
int sflc_scrypt_derive(char *pwd, size_t pwd_len, char *salt, char *hash)
{
	gcry_error_t err;

	// Overloading of parameters "subalgo" and "iterations", as per Gcrypt docs
	err = gcry_kdf_derive(pwd, pwd_len, GCRY_KDF_SCRYPT, SFLC_SCRYPT_N, salt,
			SFLC_SCRYPT_SALTLEN, SFLC_SCRYPT_P, SFLC_CRYPTO_KEYLEN, hash);
	if (err) {
		sflc_log_error("Could not compute Scrypt KDF: error %d", err);
		return err;
	}
	sflc_log_debug("Successfully computed Scrypt KDF");

	return 0;
}
