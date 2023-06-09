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
#include "test_aes256ctr.h"
#include "minunit.h"
#include "utils/log.h"


/*****************************************************
 *                 CONSTANT VARIABLES                *
 *****************************************************/

static const char KEY[] = AES256CTR_TEST_KEY;
static const char IV[] = AES256CTR_TEST_IV;
static const char PT[] = AES256CTR_TEST_PT;
static const char CT[] = AES256CTR_TEST_CT;


/*****************************************************
 *          PUBLIC FUNCTIONS DEFINITIONS            *
 *****************************************************/

char *test_aes256ctr_encrypt_inplace()
{
	char msg[] = AES256CTR_TEST_PT;
	size_t msg_len = sizeof(msg);
	char key[] = AES256CTR_TEST_KEY;
	char iv[] = AES256CTR_TEST_IV;
	int err;

	sflc_log_blue("Testing AES256-CTR encryption in-place");

	// Encrypt in-place
	err = sflc_aes256ctr_encrypt(key, msg, msg_len, iv, NULL);

	// Check error
	mu_assert("Error while encrypting", !err);

	// Check outcome
	mu_assert("Ciphertext mismatch", memcmp(msg, CT, msg_len) == 0);

	// Check key untouched
	mu_assert("Key changed", memcmp(key, KEY, sizeof(key)) == 0);

	// Check IV untouched
	mu_assert("IV changed", memcmp(iv, IV, sizeof(iv)) == 0);

	sflc_log_green("OK");

	return NULL;
}

char *test_aes256ctr_encrypt_outofplace()
{
	char msg[] = AES256CTR_TEST_PT;
	char ct[sizeof(msg)];
	size_t msg_len = sizeof(msg);
	char key[] = AES256CTR_TEST_KEY;
	char iv[] = AES256CTR_TEST_IV;
	int err;

	sflc_log_blue("Testing AES256-CTR encryption out-of-place");

	// Encrypt out-of-place
	err = sflc_aes256ctr_encrypt(key, msg, msg_len, iv, ct);

	// Check error
	mu_assert("Error while encrypting", !err);

	// Check outcome
	mu_assert("Ciphertext mismatch", memcmp(ct, CT, msg_len) == 0);

	// Check msg untouched
	mu_assert("Plaintext changed", memcmp(msg, PT, sizeof(key)) == 0);

	// Check key untouched
	mu_assert("Key changed", memcmp(key, KEY, sizeof(key)) == 0);

	// Check IV untouched
	mu_assert("IV changed", memcmp(iv, IV, sizeof(iv)) == 0);

	sflc_log_green("OK");

	return NULL;
}

char *test_aes256ctr_decrypt_inplace()
{
	char msg[] = AES256CTR_TEST_CT;
	size_t msg_len = sizeof(msg);
	char key[] = AES256CTR_TEST_KEY;
	char iv[] = AES256CTR_TEST_IV;
	int err;

	sflc_log_blue("Testing AES256-CTR decryption in-place");

	// Decrypt in-place
	err = sflc_aes256ctr_decrypt(key, msg, msg_len, iv, NULL);

	// Check error
	mu_assert("Error while decrypting", !err);

	// Check outcome
	mu_assert("Plaintext mismatch", memcmp(msg, PT, msg_len) == 0);

	// Check key untouched
	mu_assert("Key changed", memcmp(key, KEY, sizeof(key)) == 0);

	// Check IV untouched
	mu_assert("IV changed", memcmp(iv, IV, sizeof(iv)) == 0);

	sflc_log_green("OK");

	return NULL;
}

char *test_aes256ctr_decrypt_outofplace()
{
	char msg[] = AES256CTR_TEST_CT;
	char pt[sizeof(msg)];
	size_t msg_len = sizeof(msg);
	char key[] = AES256CTR_TEST_KEY;
	char iv[] = AES256CTR_TEST_IV;
	int err;

	sflc_log_blue("Testing AES256-CTR decryption out-of-place");

	// Decrypt out-of-place
	err = sflc_aes256ctr_decrypt(key, msg, msg_len, iv, pt);

	// Check error
	mu_assert("Error while decrypting", !err);

	// Check outcome
	mu_assert("Plaintext mismatch", memcmp(pt, PT, msg_len) == 0);

	// Check msg untouched
	mu_assert("Ciphertext changed", memcmp(msg, CT, sizeof(key)) == 0);

	// Check key untouched
	mu_assert("Key changed", memcmp(key, KEY, sizeof(key)) == 0);

	// Check IV untouched
	mu_assert("IV changed", memcmp(iv, IV, sizeof(iv)) == 0);

	sflc_log_green("OK");

	return NULL;
}

