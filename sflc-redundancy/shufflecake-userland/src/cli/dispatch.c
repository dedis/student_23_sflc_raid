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

#include <string.h>

#include "cli/cli.h"
#include "utils/sflc.h"
#include "utils/log.h"


/*****************************************************
 *          PUBLIC FUNCTIONS DEFINITIONS             *
 *****************************************************/

/**
 * Called by the main to parse the arguments and dispatch to the right command.
 * Just a big if-else.
 *
 * @param argc The number of command-line arguments supplied to the main
 * @param argv The arguments
 *
 * @return Error code, 0 on success
 */
int sflc_cli_dispatch(int argc, char **argv)
{
	/* We expect only one argument, indicating the subcommand */
	if (argc != 2) {
		sflc_cli_help(argc, argv);
		return 0;
	}

	/* Big switch */
	if (strcmp(argv[1], SFLC_CLI_INITCMD) == 0) {
		return sflc_cli_init();
	}
	if (strcmp(argv[1], SFLC_CLI_OPENCMD) == 0) {
		return sflc_cli_open();
	}
	if (strcmp(argv[1], SFLC_CLI_CLOSECMD) == 0) {
		return sflc_cli_close();
	}

	// REDUNDANCY MITIGATION
	
	if (strcmp(argv[1], SFLC_CLI_REDINITCMD) == 0) {
		return sflc_cli_redinit();
	}

	/* Default case */
	sflc_cli_help(argc, argv);
	return 0;
}

