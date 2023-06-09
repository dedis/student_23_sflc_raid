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
 * This file only implements the rmap-related functions.
 */

/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/

#include "device.h"
#include "crypto/rand/rand.h"
#include "log/log.h"

/*****************************************************
 *                     CONSTANTS                     *
 *****************************************************/

/*****************************************************
 *           PUBLIC FUNCTIONS DEFINITIONS            *
 *****************************************************/

/* Sets the PSI as owned by the given volume. Returns < 0 if already taken. */
int sflc_dev_setRmap(sflc_Device * dev, u32 psi, u8 vol_idx)
{
	u8 prev_vol_idx;

	/* Bounds check */
	if (psi >= dev->tot_slices) {
		pr_err("Requested to set ownership for invalid PSI\n");
		return -EINVAL;
	}

	/* Check that it's free */
	prev_vol_idx = dev->rmap[psi];
	if (prev_vol_idx != SFLC_DEV_RMAP_INVALID_VOL) {
		pr_err("Requested to set ownership for already-owned PSI\n");
		return -EINVAL;
	}

	/* Just set it */
	dev->rmap[psi] = vol_idx;
	dev->free_slices -= 1;

	return 0;
}

/* Sets the PSI as free. */
void sflc_dev_unsetRmap(sflc_Device * dev, u32 psi)
{
	/* Bounds check */
	if (psi >= dev->tot_slices) {
		pr_err("Requested to unset ownership for invalid PSI\n");
	}

	/* Just unset it */
	dev->rmap[psi] = SFLC_DEV_RMAP_INVALID_VOL;
	dev->free_slices += 1;

	return;
}

/* Returns a random free physical slice, or < 0 if error */
s32 sflc_dev_getRandomFreePsi(sflc_Device * dev)
{
	s32 psi;

	/* Check that there are free slices */
	if (!dev->free_slices) {
		pr_crit("Whoah! No free PSIs on the device! Catastrophe!");
		return -ENOSPC;
	}

	/* Repeatedly sample until you find a free one */
	do {
		psi = sflc_rand_uniform(dev->tot_slices);
		if (psi < 0) {
			pr_err("Could not sample random PSI\n");
			return -EINVAL;
		}
	} while (dev->rmap[psi] != SFLC_DEV_RMAP_INVALID_VOL);

	return psi;
}
