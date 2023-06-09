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

#ifndef _CLI_CLI_H_
#define _CLI_CLI_H_


/*****************************************************
 *                     CONSTANTS                     *
 *****************************************************/

/* Subcommand to create volumes */
#define SFLC_CLI_INITCMD	"init"
/* Subcommand to open volumes */
#define SFLC_CLI_OPENCMD	"open"
/* Subcommand to close volumes */
#define SFLC_CLI_CLOSECMD	"close"

// REDUNDANCY MITIGATION

/* Subcommand to create redundant volumes */
#define SFLC_CLI_REDINITCMD "redinit"


/*****************************************************
 *           PUBLIC FUNCTIONS PROTOTYPES             *
 *****************************************************/

/* Called by the main to parse the arguments and dispatch to the right command */
int sflc_cli_dispatch(int argc, char **argv);

/* Gather user inputs interactively to create volumes */
int sflc_cli_init(void);
/* Gather user inputs interactively to create redundant volumes */
int sflc_cli_redinit(void);
/* Gather user inputs interactively to open volumes */
int sflc_cli_open(void);
/* Gather user inputs interactively to close volumes */
int sflc_cli_close(void);
/* Show usage prompt */
void sflc_cli_help(int argc, char **argv);


#endif /* _CLI_CLI_H_ */
