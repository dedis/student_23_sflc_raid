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

#include <string.h>
#include <stdint.h>

#include "utils/crypto.h"
#include "test_aes256gcm.h"
#include "minunit.h"
#include "utils/log.h"


/*****************************************************
 *                 CONSTANT VARIABLES                *
 *****************************************************/

static const char KEY[] = AES256GCM_TEST_KEY;
static const char IV[] = AES256GCM_TEST_IV;
static const char PT[] = AES256GCM_TEST_PT;
static const char CT[] = AES256GCM_TEST_CT;
static const char TAG[] = AES256GCM_TEST_TAG;


/*****************************************************
 *          PUBLIC FUNCTIONS DEFINITIONS            *
 *****************************************************/

char *test_aes256gcm_encrypt()
{
	char pt[] = AES256GCM_TEST_PT;
	size_t pt_len = sizeof(pt);
	char key[] = AES256GCM_TEST_KEY;
	char iv[] = AES256GCM_TEST_IV;
	char ct[sizeof(pt)];
	char tag[SFLC_AESGCM_TAGLEN];
	int err;

	sflc_log_blue("Testing AES256-GCM encryption");

	// Encrypt
	err = sflc_aes256gcm_encrypt(key, pt, pt_len, iv, ct, tag);

	// Check error
	mu_assert("Error while encrypting", !err);

	// Check outcome
	mu_assert("Ciphertext mismatch", memcmp(ct, CT, pt_len) == 0);
	mu_assert("MAC mismatch", memcmp(tag, TAG, SFLC_AESGCM_TAGLEN) == 0);

	// Check key untouched
	mu_assert("Key changed", memcmp(key, KEY, sizeof(key)) == 0);

	// Check IV untouched
	mu_assert("IV changed", memcmp(iv, IV, sizeof(iv)) == 0);

	sflc_log_green("OK");

	return NULL;
}

char *test_aes256gcm_decrypt_good()
{
	char ct[] = AES256GCM_TEST_CT;
	size_t ct_len = sizeof(ct);
	char tag[] = AES256GCM_TEST_TAG;
	char key[] = AES256GCM_TEST_KEY;
	char iv[] = AES256GCM_TEST_IV;
	bool match;
	char pt[sizeof(ct)];
	int err;

	sflc_log_blue("Testing AES256-GCM decryption with the proper MAC");

	// Decrypt
	err = sflc_aes256gcm_decrypt(key, ct, ct_len, tag, iv, pt, &match);

	// Check error
	mu_assert("Error while decrypting", !err);

	// Check outcome
	mu_assert("MAC verification failed", match);
	mu_assert("Plaintext mismatch", memcmp(pt, PT, ct_len) == 0);

	// Check key untouched
	mu_assert("Key changed", memcmp(key, KEY, sizeof(key)) == 0);

	// Check IV untouched
	mu_assert("IV changed", memcmp(iv, IV, sizeof(iv)) == 0);

	// Check MAC untouched
	mu_assert("MAC changed", memcmp(tag, TAG, sizeof(tag)) == 0);

	sflc_log_green("OK");

	return NULL;
}

char *test_aes256gcm_decrypt_fail()
{
	char ct[] = AES256GCM_TEST_CT;
	size_t ct_len = sizeof(ct);
	char tag[] = AES256GCM_TEST_TAG;
	char key[] = AES256GCM_TEST_KEY;
	char iv[] = AES256GCM_TEST_IV;
	bool match;
	char pt[sizeof(ct)];
	int err;

	sflc_log_blue("Testing AES256-GCM decryption without the proper MAC");

	// Corrupt the MAC
	tag[0] += 1;

	// Decrypt
	err = sflc_aes256gcm_decrypt(key, ct, ct_len, tag, iv, pt, &match);

	// Check error
	mu_assert("Error while decrypting", !err);

	// Check outcome
	mu_assert("MAC verification succeeded", !match);

	// Check key untouched
	mu_assert("Key changed", memcmp(key, KEY, sizeof(key)) == 0);

	// Check IV untouched
	mu_assert("IV changed", memcmp(iv, IV, sizeof(iv)) == 0);

	// Check MAC untouched
	mu_assert("Tail of MAC changed", memcmp(tag+1, TAG+1, sizeof(tag)-1) == 0);
	mu_assert("Head of MAC changed", tag[0] == TAG[0]+1);

	sflc_log_green("OK");

	return NULL;
}
