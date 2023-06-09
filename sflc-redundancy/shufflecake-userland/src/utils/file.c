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

#include "utils/file.h"
#include "utils/log.h"


/*****************************************************
 *          PUBLIC FUNCTIONS DEFINITIONS            *
 *****************************************************/

/* Reads the entire content of a file in a malloc-ed string */
char *sflc_readFile(char *path)
{
    int filesize;
    FILE *fp;
    char *content;

    /* Open file */
    fp = fopen(path, "r");
    if (fp == NULL) {
        sflc_log_error("Could not open file %s", path);
        perror("Reason: ");
        goto bad_fopen;
    }

    /* Get size (overestimated) */
    fseek(fp, 0L, SEEK_END);
    filesize = ftell(fp);
    rewind(fp);

    /* Allocate */
    content = malloc(filesize + 1);
    if (content == NULL) {
        sflc_log_error("Could not malloc %d bytes for file content", filesize);
        goto bad_malloc;
    }

    /* Read (adjust filesize) */
    filesize = fread(content, 1, filesize, fp);
    content[filesize] = '\0';

    /* Close */
    fclose(fp);

    return content;


bad_malloc:
	fclose(fp);
bad_fopen:
	return NULL;
}
