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
 * Miscellaneous helper functions
 */

/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/

#include "utils.h"

/*****************************************************
 *           PUBLIC FUNCTIONS DEFINITIONS            *
 *****************************************************/

/* 
 * String utils 
 */

void join_strings(char * dest, char ** srcs, unsigned count)
{
    int i;

    dest[0] = '\0';
    for (i = 0; i < count; ++i) {
        strcat(dest, srcs[i]);

        if (i < count) {
            strcat(dest, " ");
        }
    }

    return;
}

/* Encodes the given buffer of bytes as a hex string. */
unsigned char * hex_encode(unsigned char *in, unsigned len) {

    unsigned i;
    char *out;

    out = malloc((len * 2) + 1);

    for(i = 0; i < len; i++) {
        sprintf(out + (i * 2), "%02x", in[i]);
    }
    out[len*2] = '\0';

    return out;
}

/* Replaces all occurrences of old with new */
void str_replaceAll(char * str, char old, char new)
{
    int i;
    for (i = 0; str[i] != '\0'; i++) {
        if (str[i] == old) {
            str[i] = new;
        }
    }

    return;
}

/* Reads the entire content of a file in a malloc-ed string */
char * read_file_contents(char * path)
{
    int filesize;
    FILE * fp;
    char * content;

    /* Open file */
    fp = fopen(path, "r");
    if (fp == NULL) {
        die("ERR: Could not open file %s", path);
    }

    /* Get size */
    fseek(fp, 0L, SEEK_END);
    filesize = ftell(fp);
    rewind(fp);

    /* Allocate */
    content = malloc(filesize + 1);
    if (content == NULL) {
        die("ERR: Could not malloc");
    }

    /* Read */
    filesize = fread(content, 1, filesize, fp);
    content[filesize] = '\0';

    /* Close */
    fclose(fp);

    return content;
}
