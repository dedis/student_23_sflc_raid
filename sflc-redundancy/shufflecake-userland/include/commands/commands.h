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

#ifndef _COMMANDS_COMMANDS_H_
#define _COMMANDS_COMMANDS_H_


/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>


/*****************************************************
 *                    CONSTANTS                      *
 *****************************************************/


/*****************************************************
 *                     STRUCTS                       *
 *****************************************************/

/* Parameters for the init command */
typedef struct
{
	/* Underlying block device */
	char	*bdev_path;

	/* Number of volumes */
	size_t	nr_vols;
	/* Volumes' passwords */
	char	**pwds;
	size_t	*pwd_lens;

	/* Option to skip random filling */
	bool	no_randfill;

} sflc_cmd_InitArgs;


/* Parameters for the open command */
typedef struct
{
	/* Underlying block device */
	char 	*bdev_path;

	/* The only password provided */
	char	*pwd;
	size_t	pwd_len;

} sflc_cmd_OpenArgs;


/*****************************************************
 *           PUBLIC FUNCTIONS PROTOTYPES             *
 *****************************************************/

/* Create N volumes (only formats the device header, does not open the volumes) */
int sflc_cmd_initVolumes(sflc_cmd_InitArgs *args);

/* Create N redundant volumes plus a decoy (only formats the device header, does not open the volumes) */
int sflc_cmd_redinitVolumes(sflc_cmd_InitArgs *args);

/* Open M volumes, from the first down to the one whose pwd is provided */
int sflc_cmd_openVolumes(sflc_cmd_OpenArgs *args);

/* Close all volumes on the device (reads the list from sysfs) */
int sflc_cmd_closeVolumes(char *bdev_path);


#endif /* _COMMANDS_COMMANDS_H_ */
