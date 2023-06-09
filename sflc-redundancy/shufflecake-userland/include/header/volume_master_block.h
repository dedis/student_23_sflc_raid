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
 * Defines the Volume Master Block (VMB) and related functionalities
 */

#ifndef _HEADER_VOLUME_MASTER_BLOCK_H_
#define _HEADER_VOLUME_MASTER_BLOCK_H_


/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/

#include <stdint.h>
#include <stddef.h>

#include "utils/disk.h"
#include "utils/crypto.h"


/*****************************************************
 *                    CONSTANTS                      *
 *****************************************************/

// The current version number for the master block format
#define SFLC_VMBVER_MAJOR	0
#define SFLC_VMBVER_MINOR	1

// IVs occupy 16 bytes on-disk, but only the *FIRST* 12 are used for AES-GCM
#define SFLC_PADDED_IVLEN	16	/* bytes */

// The VMB payload occupies the last 3984 bytes on-disk, excluding its IV and MAC
#define SFLC_VMB_PAYLOAD_LEN	(SFLC_SECTOR_SIZE		-	\
								SFLC_SCRYPT_SALTLEN		-	\
								2 * SFLC_PADDED_IVLEN	-	\
								SFLC_CRYPTO_KEYLEN		-	\
								2 * SFLC_AESGCM_TAGLEN)

// The VMB format version is made of two 32-bit unsigned integers (network byte order)
#define SFLC_VMBVER_MAJOR_WIDTH	4	/* bytes */
#define SFLC_VMBVER_MINOR_WIDTH	4	/* bytes */



/*****************************************************
 *                     STRUCTS                       *
 *****************************************************/

/**
 * The on-disk master block of a volume contains lots of crypto stuff
 * (a KDF salt, IVs, MACs...) used to properly hide the useful
 * info. This struct only contains the useful info.
 */
typedef struct {
	// The key that encrypts the rest of the fields (payload)
	char vmb_key[SFLC_CRYPTO_KEYLEN];

	// The key that encrypts the volume's data section
	char volume_key[SFLC_CRYPTO_KEYLEN];

	// The key that encrypts the previous volume's master block
	char prev_vmb_key[SFLC_CRYPTO_KEYLEN];

	// The total number of logical slices virtually available to this volume
	uint32_t nr_slices;

} sflc_VolumeMasterBlock;


/*****************************************************
 *           PUBLIC FUNCTIONS PROTOTYPES             *
 *****************************************************/

/* "Encrypt" a master block with a password, so it's ready to be written on-disk */
int sflc_vmb_seal(sflc_VolumeMasterBlock *vmb, char *pwd, size_t pwd_len, char *disk_block);

/* Try to "decrypt" a master block coming from the disk using the password (perform the KDF) */
int sflc_vmb_tryUnsealWithPwd(char *disk_block, char *pwd, size_t pwd_len, bool *match);

/* "Decrypt" a master block coming from the disk, using the password (perform the KDF) */
int sflc_vmb_unsealWithPwd(char *disk_block, char *pwd, size_t pwd_len, sflc_VolumeMasterBlock *vmb);

/* "Decrypt" a master block coming from the disk, directly using its key (bypass the KDF) */
int sflc_vmb_unsealWithKey(char *disk_block, char *vmb_key, sflc_VolumeMasterBlock *vmb);


#endif /* _HEADER_VOLUME_MASTER_BLOCK_H_ */
