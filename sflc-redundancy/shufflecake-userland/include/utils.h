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

#ifndef _UTILS_H_
#define _UTILS_H_

/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

/*****************************************************
 *                       MACROS                      *
 *****************************************************/

#define NORMAL "\x1B[0m"
#define RED "\x1B[31m"
#define GREEN "\x1B[32m"

/* Print in color. */
#define print_color(color, fmt, ...)                \
    do {                                            \
        printf("[%s:%d]@%s\t" color fmt NORMAL, __FILE__, __LINE__, __func__, ##__VA_ARGS__);    \
        fflush(stdout);                             \
    } while (0)

/* Print in red. */
#define print_red(fmt, ...)                     \
    print_color(RED, fmt, ##__VA_ARGS__)

/* Print in green. */
#define print_green(fmt, ...)                   \
    print_color(GREEN, fmt, ##__VA_ARGS__)

/* Print error message and exit. */
#define die(fmt, ...)                               \
    do {                                            \
        print_red(fmt "\n" NORMAL, ##__VA_ARGS__);       \
        exit(1);                                    \
    } while (0)


/*****************************************************
 *            PUBLIC FUNCTIONS PROTOTYPES            *
 *****************************************************/

/* String utils */
void join_strings(char * dest, char ** srcs, unsigned count);
unsigned char * hex_encode(unsigned char *in, unsigned len);
void str_replaceAll(char * str, char old, char new);

/* File utils */
char * read_file_contents(char * path);

#endif /* _UTILS_H_ */
