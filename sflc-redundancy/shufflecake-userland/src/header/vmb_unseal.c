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

/* AES-GCM-decrypt the VMB key with the KDF-generated key */
static int _unsealPrologue(char *disk_block, char *pwd, size_t pwd_len, char *vmb_key, bool *match);

/* AES-GCM-decrypt the payload with the VMB key */
static int _unsealPayload(char *disk_block, sflc_VolumeMasterBlock *vmb, bool *match);

/* Deserialise the payload after decrypting it */
static int _deserialisePayload(char *payload, sflc_VolumeMasterBlock *vmb);


/*****************************************************
 *          PUBLIC FUNCTIONS DEFINITIONS             *
 *****************************************************/

/**
 * Only check if the password is correct for this volume
 *
 * @param disk_block The content of the on-disk encrypted VMB
 * @param pwd The proposed password to unseal it
 * @param pwd_len The length of the proposed password
 * @param match A pointer to an output boolean, indicating success or failure
 *
 * @return An error code, 0 on success
 */
int sflc_vmb_tryUnsealWithPwd(char *disk_block, char *pwd, size_t pwd_len, bool *match)
{
	char vmb_key[SFLC_CRYPTO_KEYLEN];	// Unused
	int err;

	// Try to unseal the prologue with this password
	err = _unsealPrologue(disk_block, pwd, pwd_len, vmb_key, match);
	if (err) {
		sflc_log_error("Error while unsealing prologue: error %d", err);
		goto bad_unseal;
	}
	sflc_log_debug("Checked for password match, result: %d", *match);

	// No prob
	err = 0;


bad_unseal:
	// Always wipe the key, even on success
	memset(vmb_key, 0, SFLC_CRYPTO_KEYLEN);
	return err;
}


/**
 * Decrypt the whole VMB using this password
 *
 * @param disk_block The content of the on-disk encrypted VMB
 * @param pwd The proposed password to unseal it
 * @param pwd_len The length of the proposed password
 * @param vmb A pointer to the output struct that will contain all the VMB fields
 *
 * @return An error code, 0 on success
 */
int sflc_vmb_unsealWithPwd(char *disk_block, char *pwd, size_t pwd_len, sflc_VolumeMasterBlock *vmb)
{
	bool match;
	int err;

	/* Unseal the VMB key with the password */
	err = _unsealPrologue(disk_block, pwd, pwd_len, vmb->vmb_key, &match);
	if (err) {
		sflc_log_error("Error while unsealing prologue: error %d", err);
		goto bad_unseal_prologue;
	}
	if (!match) {
		sflc_log_error("Wrong password supplied!");
		err = EINVAL;
		goto bad_pwd;
	}
	sflc_log_debug("Successfully decrypted prologue");

	/* Unseal the other fields using the VMB key */
	err = _unsealPayload(disk_block, vmb, &match);
	if (err) {
		sflc_log_error("Error while unsealing payload: error %d", err);
		goto bad_unseal_payload;
	}
	if (!match) {
		sflc_log_error("Password was right but VMB key is not, something is seriously wrong!");
		err = ENOTRECOVERABLE;
		goto bad_key;
	}
	sflc_log_debug("Successfully decrypted payload");

	// No prob
	err = 0;


bad_key:
bad_unseal_payload:
bad_pwd:
bad_unseal_prologue:
	return err;
}


/**
 * Decrypt the VMB payload using this VMB key. Sets the VMB key inside
 * the struct.
 *
 * @param disk_block The content of the on-disk encrypted VMB
 * @param vmb_key The proposed VMB key to unseal its payload
 * @param vmb A pointer to the output struct that will contain all the VMB fields
 *
 * @return An error code, 0 on success
 */
int sflc_vmb_unsealWithKey(char *disk_block, char *vmb_key, sflc_VolumeMasterBlock *vmb)
{
	bool match;
	int err;

	/* Set the VMB key */
	memcpy(vmb->vmb_key, vmb_key, SFLC_CRYPTO_KEYLEN);

	/* Unseal the payload using that key */
	err = _unsealPayload(disk_block, vmb, &match);
	if (err) {
		sflc_log_error("Error while unsealing payload");
		goto bad_unseal;
	}
	if (!match) {
		sflc_log_error("Wrong VMB key supplied!");
		err = EINVAL;
		goto bad_key;
	}
	sflc_log_debug("Successfully decrypted payload");

	// No prob
	err = 0;


bad_key:
bad_unseal:
	return err;
}


/*****************************************************
 *          PRIVATE FUNCTIONS PROTOTYPES             *
 *****************************************************/

/* AES-GCM-decrypt the VMB key with the KDF-generated key */
static int _unsealPrologue(char *disk_block, char *pwd, size_t pwd_len, char *vmb_key, bool *match)
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

	/* Derive KEK */
	err = sflc_scrypt_derive(pwd, pwd_len, salt, kek);
	if (err) {
		sflc_log_error("Could not perform KDF: error %d", err);
		goto bad_kdf;
	}
	sflc_log_debug("Successfully derived key-encryption-key with KDF");

	/* Decrypt the VMB key */
	err = sflc_aes256gcm_decrypt(kek, enc_vmb_key, SFLC_CRYPTO_KEYLEN, mac, iv, vmb_key, match);
	if (err) {
		sflc_log_error("Error while decrypting VMB key: error %d", err);
		goto bad_decrypt;
	}
	sflc_log_debug("Decrypted VMB key: match = %d", *match);


bad_decrypt:
bad_kdf:
	/* Always wipe the key from memory, even on success */
	memset(kek, 0, SFLC_CRYPTO_KEYLEN);
	return err;
}

/* AES-GCM-decrypt the payload with the VMB key */
static int _unsealPayload(char *disk_block, sflc_VolumeMasterBlock *vmb, bool *match)
{
	// Pointers inside the block
	char *iv = disk_block + SFLC_SCRYPT_SALTLEN + SFLC_PADDED_IVLEN +
			SFLC_CRYPTO_KEYLEN + SFLC_AESGCM_TAGLEN;
	char *mac = iv + SFLC_PADDED_IVLEN;
	char *enc_payload = mac + SFLC_AESGCM_TAGLEN;
	// Decrypted payload (dynamically allocated), to be deserialised
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

	/* Decrypt the payload */
	err = sflc_aes256gcm_decrypt(vmb->vmb_key, enc_payload, SFLC_VMB_PAYLOAD_LEN, mac, iv, payload, match);
	if (err) {
		sflc_log_error("Error while decrypting payload: error %d", err);
		goto bad_decrypt;
	}
	if (!*match) {	// Pointless to continue
		sflc_log_error("Wrong VMB key supplied!");
		goto bad_key;
	}
	sflc_log_debug("Successfully decrypted payload");

	/* Deserialise the struct */
	err = _deserialisePayload(payload, vmb);
	if (err) {
		sflc_log_error("Error while deserialising payload: error %d", err);
		goto bad_deserialise;
	}
	sflc_log_debug("Deserialised VMB struct");

	// No prob
	err = 0;


bad_deserialise:
bad_key:
bad_decrypt:
	/* Always wipe and free the payload, even on success */
	memset(payload, 0, SFLC_VMB_PAYLOAD_LEN);
	free(payload);
bad_payload_alloc:
	return err;
}


/* Deserialise the payload after decrypting it */
static int _deserialisePayload(char *payload, sflc_VolumeMasterBlock *vmb)
{
	uint32_t ver_major;
	uint32_t ver_minor;

	// Pointers inside the payload
	char *p_ver_major = payload;
	char *p_ver_minor = p_ver_major + SFLC_VMBVER_MAJOR_WIDTH;
	char *p_vol_key = p_ver_minor + SFLC_VMBVER_MINOR_WIDTH;
	char *p_prev_vmb_key = p_vol_key + SFLC_CRYPTO_KEYLEN;
	char *p_nr_slices = p_prev_vmb_key + SFLC_CRYPTO_KEYLEN;

	/* Read version numbers (network byte order) */
	ver_major = ntohl( *((uint32_t *) p_ver_major) );
	ver_minor = ntohl( *((uint32_t *) p_ver_minor) );
	if (ver_major != SFLC_VMBVER_MAJOR ||
	    ver_minor > SFLC_VMBVER_MINOR) {
		sflc_log_error("On disk VMB format version %u.%u is incompatible with current"
				"version %u.%u", ver_major, ver_minor, SFLC_VMBVER_MAJOR, SFLC_VMBVER_MINOR);
		return EINVAL;
	}

	/* Copy the volume key */
	memcpy(vmb->volume_key, p_vol_key, SFLC_CRYPTO_KEYLEN);

	/* Copy the previous volume's VMB key */
	memcpy(vmb->prev_vmb_key, p_prev_vmb_key, SFLC_CRYPTO_KEYLEN);

	/* Read number of slices (network byte order) */
	vmb->nr_slices = ntohl( *((uint32_t *) p_nr_slices) );

	// Ignore the rest

	return 0;
}

