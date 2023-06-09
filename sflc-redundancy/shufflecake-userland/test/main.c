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


#include "crypto/test_aes256ctr.h"
#include "crypto/test_aes256gcm.h"
#include "actions/test_actions.h"
#include "minunit.h"
#include "utils/log.h"


/*****************************************************
 *           PRIVATE FUNCTIONS PROTOTYPES            *
 *****************************************************/

static char *all_tests();


/*****************************************************
 *                        MAIN                       *
 *****************************************************/

int main()
{
	char *result = all_tests();

	if (result != NULL) {
		sflc_log_red("\nTEST FAILED: %s", result);
	}
	else {
		sflc_log_green("\nALL TESTS PASSED");
	}

	return result != NULL;
}


/*****************************************************
 *          PRIVATE FUNCTIONS DEFINITIONS            *
 *****************************************************/

static char *all_tests()
{
	sflc_log_yellow("Running crypto tests");
	mu_run_test(test_aes256ctr_encrypt_inplace);
	mu_run_test(test_aes256ctr_encrypt_outofplace);
	mu_run_test(test_aes256ctr_decrypt_inplace);
	mu_run_test(test_aes256ctr_decrypt_outofplace);
	mu_run_test(test_aes256gcm_encrypt);
	mu_run_test(test_aes256gcm_decrypt_good);
	mu_run_test(test_aes256gcm_decrypt_fail);

	sflc_log_yellow("\nRunning action tests");
	mu_run_test(test_vol_create);
	mu_run_test(test_vol_allActions);

	return 0;
}

