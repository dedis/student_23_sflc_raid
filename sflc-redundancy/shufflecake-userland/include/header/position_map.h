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
 * Helpers to encrypt an empty position map
 */

#ifndef _HEADER_POSITION_MAP_H_
#define _HEADER_POSITION_MAP_H_


/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/

#include <stdint.h>
#include <stddef.h>


/*****************************************************
 *                     STRUCTS                       *
 *****************************************************/

/**
 * This struct represents an encrypted empty position map.
 * There are as many IV blocks as there are PosMapBlock arrays.
 * The m-th IV of the n-th IV block encrypts the m-th block of the n-th array.
 * The PosMapBlocks in an array are contiguous, so a PosMapBlock array is just
 * a char array of length multiple of 4096.
 * All the arrays are full (256 PosMapBlocks, 1 MiB) except for the last one,
 * which may hold fewer blocks.
 */
typedef struct {
	// The number of PosMapBlock arrays (and of IV blocks)
	size_t nr_arrays;

	// The sequence of IV blocks
	char **iv_blocks;
	// The sequence of (encrypted) PosMapBlock arrays
	char **pmb_arrays;

	// The number of PosMapBlocks in the last array
	size_t nr_last_pmbs;

} sflc_EncPosMap;


/*****************************************************
 *           PUBLIC FUNCTIONS PROTOTYPES             *
 *****************************************************/

/* Create an encrypted empty position map for the given number of slices (allocates memory) */
int sflc_epm_create(size_t nr_slices, char *volume_key, sflc_EncPosMap *epm);


#endif /* _HEADER_POSITION_MAP_H_ */
