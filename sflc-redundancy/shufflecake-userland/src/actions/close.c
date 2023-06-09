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

/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "header/volume_master_block.h"
#include "actions/volume.h"
#include "utils/sflc.h"
#include "utils/crypto.h"
#include "utils/file.h"
#include "utils/string.h"
#include "utils/dm.h"
#include "utils/log.h"


/*****************************************************
 *          PUBLIC FUNCTIONS DEFINITIONS             *
 *****************************************************/

/**
 * Close the volume by issuing the appropriate ioctl to the DM.
 *
 * @param label The only needed parameter: the ID of the volume.
 *
 * @return Error code, 0 on success
 */
int sflc_act_closeVolume(char *label)
{
	/* Issue ioctl */
	return sflc_dm_destroy(label);
}
