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
 * Interface to the device mapper
 */

#ifndef _UTILS_DM_H_
#define _UTILS_DM_H_


/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/

#include <stddef.h>

#include "utils/sflc.h"


/*****************************************************
 *                    CONSTANTS                      *
 *****************************************************/


/*****************************************************
 *            PUBLIC FUNCTIONS PROTOTYPES            *
 *****************************************************/

/* Create a new Shufflecake virtual device (volume) under /dev/mapper */
int sflc_dm_create(char * virt_dev_name, uint64_t num_sectors, char * params);
/* Destroy the virtual device under /dev/mapper */
int sflc_dm_destroy(char * virt_dev_name);


#endif /* _UTILS_DM_H_ */
