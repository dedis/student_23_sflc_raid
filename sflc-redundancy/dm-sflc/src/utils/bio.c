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
 * Implementations for all the bio utility functions
 */

/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/

#include "bio.h"
#include "log/log.h"


/*****************************************************
 *           PUBLIC FUNCTIONS DEFINITIONS            *
 *****************************************************/

/**
 * Checks whether each of the bio's segments contains a whole
 * number of 4096-byte sectors.
 */
bool sflc_bio_isAligned(struct bio * bio)
{
	bool ret = true;

	/* Unlikely because we'll probably catch it before sending it here */
	if (unlikely(bio->bi_iter.bi_size) == 0) {
		pr_err("bi_size = 0\n");
		return false;
	}
	/* Unlikely because we tell the DM layer about our sector size */
	if (unlikely(bio->bi_iter.bi_size % SFLC_DEV_SECTOR_SIZE != 0)) {
		pr_err("Abnormal bi_size = %u\n", bio->bi_iter.bi_size);
		return false;
	}
	/* Unlikely because we tell the DM layer about our sector size */
	if (unlikely(bio->bi_iter.bi_sector % SFLC_DEV_SECTOR_SCALE != 0)) {
		pr_err("Abnormal bi_sector = %llu\n", bio->bi_iter.bi_sector);
		return false;
	}

	struct bio_vec bvl;
	struct bvec_iter iter;
	bio_for_each_segment(bvl, bio, iter) {
		if ((bvl.bv_len == 0) || (bvl.bv_len % SFLC_DEV_SECTOR_SIZE != 0)) {
			pr_err("Abnormal vector: bv_len = %u\n", bvl.bv_len);
			ret = false;
		}
	}

	return ret;
}
