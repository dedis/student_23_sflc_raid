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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "utils/string.h"
#include "utils/log.h"


/*****************************************************
 *          PUBLIC FUNCTIONS DEFINITIONS            *
 *****************************************************/

/* Malloc's the buffer for the hex string */
char *sflc_toHex(char *buf, size_t len)
{
	unsigned char *u = (unsigned char *) buf;
	char *hex;

	/* Allocate buffer */
	hex = malloc((len * 2) + 1);
	if (!hex) {
		sflc_log_error("Could not allocate buffer for hex string");
		return NULL;
	}

	/* To hex */
	int i;
	for(i = 0; i < len; i++) {
		sprintf(hex + (i * 2), "%02x", u[i]);
	}
	hex[len*2] = '\0';

	return hex;
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
