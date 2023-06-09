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

/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>	// Network byte order

#include "header/volume_master_block.h"
#include "utils/crypto.h"
#include "utils/log.h"


/*****************************************************
 *          PRIVATE FUNCTIONS PROTOTYPES             *
 *****************************************************/

/* AES-GCM-encrypt the VMB key with the KDF-generated key */
static int _sealPrologue(char *pwd, size_t pwd_len, char *vmb_key, char *disk_block);

/* AES-GCM-encrypt the payload with the VMB key */
static int _sealPayload(sflc_VolumeMasterBlock *vmb, char *disk_block);

/* Serialise the payload before encrypting it */
static void _serialisePayload(sflc_VolumeMasterBlock *vmb, char *payload);


/*****************************************************
 *          PUBLIC FUNCTIONS DEFINITIONS             *
 *****************************************************/

/**
 * Builds the on-disk master block (indistinguishable from random).
 *
 * @param vmb The useful information stored in this volume master block
 * @param pwd The password that unlocks the key encrypting the payload
 * @param pwd_len The length of the password
 * @param disk_block The 4096-byte buffer that will contain the random-looking
 *  bytes to be written on-disk
 *
 * @return The error code, 0 on success
 */
int sflc_vmb_seal(sflc_VolumeMasterBlock *vmb, char *pwd, size_t pwd_len, char *disk_block)
{
	int err;

	/* Encrypt the VMB key with the password using KDF */
	err = _sealPrologue(pwd, pwd_len, vmb->vmb_key, disk_block);
	if (err) {
		sflc_log_error("Could not encrypt VMB key with password: error %d", err);
		goto bad_seal_prologue;
	}
	sflc_log_debug("Successfully encrypted VMB key with password");

	/* Encrypt the payload using the VMB key */
	err = _sealPayload(vmb, disk_block);
	if (err) {
		sflc_log_error("Could not encrypt the payload with VMB key: error %d", err);
		goto bad_seal_payload;
	}
	sflc_log_debug("Successfully encrypted payload with VMB key");

	// No prob
	err = 0;


bad_seal_payload:
bad_seal_prologue:
	return err;
}


/*****************************************************
 *          PRIVATE FUNCTIONS PROTOTYPES             *
 *****************************************************/

/* AES-GCM-encrypt the VMB key with the KDF-generated key */
static int _sealPrologue(char *pwd, size_t pwd_len, char *vmb_key, char *disk_block)
{
	// Pointers inside the block
	char *salt = disk_block;
	char *iv = salt + SFLC_SCRYPT_SALTLEN;
	char *enc_vmb_key = iv + SFLC_PADDED_IVLEN;
	char *mac = enc_vmb_key + SFLC_CRYPTO_KEYLEN;
	// Key-encryption-key derived from KDF
	char kek[SFLC_CRYPTO_KEYLEN];
	// Error code
	int err;

	/* Sample KDF salt */
	err = sflc_rand_getWeakBytes(salt, SFLC_SCRYPT_SALTLEN);
	if (err) {
		sflc_log_error("Could not sample KDF salt: error %d", err);
		goto bad_sample_salt;
	}
	sflc_log_debug("Successfully sampled KDF salt");

	/* Derive KEK */
	err = sflc_scrypt_derive(pwd, pwd_len, salt, kek);
	if (err) {
		sflc_log_error("Could not perform KDF: error %d", err);
		goto bad_kdf;
	}
	sflc_log_debug("Successfully derived key-encryption-key with KDF");

	/* Sample prologue IV */
	err = sflc_rand_getWeakBytes(iv, SFLC_PADDED_IVLEN);
	if (err) {
		sflc_log_error("Could not sample prologue IV: error %d", err);
		goto bad_sample_iv;
	}
	sflc_log_debug("Successfully sampled prologue IV");

	/* Encrypt the VMB key */
	err = sflc_aes256gcm_encrypt(kek, vmb_key, SFLC_CRYPTO_KEYLEN, iv, enc_vmb_key, mac);
	if (err) {
		sflc_log_error("Could not encrypt the VMB key: error %d", err);
		goto bad_encrypt;
	}
	sflc_log_debug("Successfully encrypted VMB key with key-encryption-key");

	// No prob
	err = 0;


bad_encrypt:
bad_sample_iv:
bad_kdf:
bad_sample_salt:
	/* Always wipe the key from memory, even on success */
	memset(kek, 0, SFLC_CRYPTO_KEYLEN);
	return err;
}


/* AES-GCM-encrypt the payload with the VMB key */
static int _sealPayload(sflc_VolumeMasterBlock *vmb, char *disk_block)
{
	// Pointers inside the block
	char *iv = disk_block + SFLC_SCRYPT_SALTLEN + SFLC_PADDED_IVLEN +
			SFLC_CRYPTO_KEYLEN + SFLC_AESGCM_TAGLEN;
	char *mac = iv + SFLC_PADDED_IVLEN;
	char *enc_payload = mac + SFLC_AESGCM_TAGLEN;
	// Serialised payload (dynamically allocated), to be encrypted
	char *payload;
	// Error code
	int err;

	/* Allocate large buffer on the heap */
	payload = malloc(SFLC_VMB_PAYLOAD_LEN);
	if (!payload) {
		sflc_log_error("Could not allocate %d bytes for VMB payload", SFLC_VMB_PAYLOAD_LEN);
		err = ENOMEM;
		goto bad_payload_alloc;
	}
	sflc_log_debug("Successfully allocated %d bytes for VMB payload", SFLC_VMB_PAYLOAD_LEN);

	/* Serialise the struct */
	_serialisePayload(vmb, payload);
	sflc_log_debug("Serialised VMB struct");

	/* Sample payload IV */
	err = sflc_rand_getWeakBytes(iv, SFLC_PADDED_IVLEN);
	if (err) {
		sflc_log_error("Could not sample payload IV: error %d", err);
		goto bad_sample_iv;
	}
	sflc_log_debug("Successfully sampled payload IV");

	/* Encrypt the payload */
	err = sflc_aes256gcm_encrypt(vmb->vmb_key, payload, SFLC_VMB_PAYLOAD_LEN, iv, enc_payload, mac);
	if (err) {
		sflc_log_error("Could not encrypt payload: error %d", err);
		goto bad_encrypt;
	}
	sflc_log_debug("Successfully encrypted payload");

	// No prob
	err = 0;


bad_encrypt:
bad_sample_iv:
	/* Always wipe and free the payload, even on success */
	memset(payload, 0, SFLC_VMB_PAYLOAD_LEN);
	free(payload);
bad_payload_alloc:
	return err;
}


/* Serialise the payload before encrypting it */
static void _serialisePayload(sflc_VolumeMasterBlock *vmb, char *payload)
{
	// Pointers inside the payload
	char *p_ver_major = payload;
	char *p_ver_minor = p_ver_major + SFLC_VMBVER_MAJOR_WIDTH;
	char *p_vol_key = p_ver_minor + SFLC_VMBVER_MINOR_WIDTH;
	char *p_prev_vmb_key = p_vol_key + SFLC_CRYPTO_KEYLEN;
	char *p_nr_slices = p_prev_vmb_key + SFLC_CRYPTO_KEYLEN;

	/* Write version numbers (network byte order) */
	*((uint32_t *) p_ver_major) = htonl(SFLC_VMBVER_MAJOR);
	*((uint32_t *) p_ver_minor) = htonl(SFLC_VMBVER_MINOR);

	/* Copy the volume key */
	memcpy(p_vol_key, vmb->volume_key, SFLC_CRYPTO_KEYLEN);

	/* Copy the previous volume's VMB key */
	memcpy(p_prev_vmb_key, vmb->prev_vmb_key, SFLC_CRYPTO_KEYLEN);

	/* Write the number of slices (network byte order) */
	*((uint32_t *) p_nr_slices) = htonl(vmb->nr_slices);

	// Leave the rest uninitialised

	return;
}

