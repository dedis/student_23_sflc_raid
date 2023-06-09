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

#ifndef _UTILS_INPUT_H_
#define _UTILS_INPUT_H_


/*****************************************************
 *                      MACROS                       *
 *****************************************************/

/* Clear a line from stdin, to use after a failed scanf (it didn't actually read input) */
#define sflc_ignoreLine()	scanf("%*[^\n]")


/*****************************************************
 *           PUBLIC FUNCTIONS PROTOTYPES             *
 *****************************************************/

/* Reads a line (discarding the newline) from stdin. No buffer overflow */
int sflc_safeReadLine(char *buf, size_t bufsize);


#endif /* _UTILS_FILE_H_ */
