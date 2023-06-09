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
 * Disk helper functions
 */

#ifndef _DISK_H_
#define _DISK_H_

/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/

#include <stdint.h>

/*****************************************************
 *                       MACROS                      *
 *****************************************************/

/*****************************************************
 *            PUBLIC FUNCTIONS PROTOTYPES            *
 *****************************************************/

/* Returns the size in 4096-byte sectors (or < 0 if error) */
int64_t disk_getSize(char * real_dev_path);

/* Reads a single 4096-byte sector from the disk */
int disk_readSector(char * real_dev_path, uint64_t sector, char * buf);

/* Writes a single 4096-byte sector to the disk */
int disk_writeSector(char * real_dev_path, uint64_t sector, char * buf);


#endif /* _DISK_H_ */