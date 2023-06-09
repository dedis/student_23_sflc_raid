/*
 *  Copyright The Shufflecake Project Authors (2022)
 *  Copyright The Shufflecake Project Contributors (2022)
 *  Copyright Contributors to the The Shufflecake Project.
 *  
 *  See the AUTHORS file at the top-level directory of this distribution and at
 *  <https://www.shufflecake.net/permalinks/shufflecake-userland/AUTHORS>
 *  
 *  This file is part of the program dm-sflc, which is part of the Shufflecake 
 *  Project. Shufflecake is a plausible deniability (hidden storage) layer for 
 *  Linux. See <https://www.shufflecake.net>.
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
 * Implementations for all the string utility functions
 */

/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/

#include <linux/string.h>

#include "string.h"
#include "log/log.h"


/*****************************************************
 *           PUBLIC FUNCTIONS DEFINITIONS            *
 *****************************************************/

int sflc_str_hexDecode(char * hex, u8 * bin)
{
	char buf[3];
	unsigned len;

	buf[2] = '\0';
	len = strlen(hex) / 2;

	unsigned i;
	for (i = 0; i < len; ++i) {
		int err;

		buf[0] = *hex++;
		buf[1] = *hex++;
		err = kstrtou8(buf, 16, &bin[i]);
		if (err) {
			pr_err("Could not decode byte %s; error %d\n", buf, err);
			return err;
		}
	}

	return 0;
}


void sflc_str_replaceAll(char * str, char old, char new)
{
	int i;

	for (i = 0; str[i] != '\0'; i++) {
		if (str[i] == old) {
			str[i] = new;
		}
	}

	return;
}
