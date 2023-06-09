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
 * A set of workqueues
 */

#ifndef _SFLC_UTILS_QUEUES_H_
#define _SFLC_UTILS_QUEUES_H_

/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/

#include <linux/workqueue.h>

/*****************************************************
 *           PUBLIC VARIABLES DECLARATIONS           *
 *****************************************************/

extern struct workqueue_struct * sflc_queues_writeQueue;
extern struct workqueue_struct * sflc_queues_decryptQueue;

/*****************************************************
 *            PUBLIC FUNCTIONS PROTOTYPES            *
 *****************************************************/

int sflc_queues_init(void);
void sflc_queues_exit(void);


#endif /* _SFLC_UTILS_QUEUES_H_ */