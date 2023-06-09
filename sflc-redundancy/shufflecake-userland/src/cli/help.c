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

#include <stdio.h>

#include "cli/cli.h"


/*****************************************************
 *                    CONSTANTS                      *
 *****************************************************/

/* The usage prompt */
#define SFLC_CLI_HELP_FMT																	\
	"Usage: %s <command>\n\n"																\
	"<command> is one of:\n"																\
	"\tinit: format a block device to host Shufflecake volumes\n"							\
	"\topen: open (as virtual block devices) some Shufflecake volumes in a block device\n"	\
	"\tclose: close all open Shufflecake volumes on a block device\n\n"						\
	"All commands will open an interactive prompt to gather user inputs\n"

/*****************************************************
 *          PUBLIC FUNCTIONS DEFINITIONS             *
 *****************************************************/

/**
 * Show usage prompt
 *
 * @param argc The number of command-line arguments supplied to the main
 * @param argv The arguments
 */
void sflc_cli_help(int argc, char **argv)
{
	printf(SFLC_CLI_HELP_FMT, argv[0]);
}
