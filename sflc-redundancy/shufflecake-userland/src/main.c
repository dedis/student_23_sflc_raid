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
 * Shufflecake userland tool
 */

/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <errno.h>

#include "utils.h"
#include "sflc.h"
#include "crypto.h"

/*****************************************************
 *                    CONSTANTS                      *
 *****************************************************/

/* Commands */
#define COMMAND_CREATE_VOLS_STR "create_vols"
#define COMMAND_OPEN_VOLS_STR   "open_vols"
#define COMMAND_CLOSE_VOLS_STR  "close_vols"

/* Options */
#define OPTION_CREATE_NO_RANDFILL "--no-randfill"
#define OPTION_REDUNDANT_AMONG "--redundant-among"
#define OPTION_REDUNDANT_WITHIN "--redundant-within"

/* Space for extra arguments to create command */
#define CMD_CREATE_EXTRA_ARGS_MAX_LEN 200

/*****************************************************
 *                       TYPES                       *
 *****************************************************/

enum sflc_cmd
{
    SFLC_CMD_CREATE_VOLS,
    SFLC_CMD_OPEN_VOLS,
    SFLC_CMD_CLOSE_VOLS,
    SFLC_CMD_HELP,
    SFLC_CMD_UNKNOWN
};

struct sflc_create_vols_args
{
    bool		no_randfill;
    bool        redundant_among;
    bool        redundant_within;
    char      * real_dev_path;
    char     ** pwd;
    int         nr_pwd;
};

struct sflc_open_vols_args
{
    bool        redundant_among;
    bool        redundant_within;
    char      * real_dev_path;
    char     ** vol_names;
    int         nr_vols;
    char      * last_pwd;
};

struct sflc_close_vols_args
{
    char      * real_dev_path;
};

struct sflc_args 
{
    enum sflc_cmd cmd;
    union
    {
        struct sflc_create_vols_args create_vols;
        struct sflc_open_vols_args   open_vols;
        struct sflc_close_vols_args  close_vols;
    };
};

/*****************************************************
 *           PRIVATE FUNCTIONS PROTOTYPES            *
 *****************************************************/

static int parse_args(int argc, char ** argv, struct sflc_args * args);
static int parse_create_vols_args(int argc, char ** argv, struct sflc_create_vols_args * args);
static int parse_open_vols_args(int argc, char ** argv, struct sflc_open_vols_args * args);
static int parse_close_vols_args(int argc, char ** argv, struct sflc_close_vols_args * args);
static void show_help(char * bin_name);

/*****************************************************
 *                       MAIN                        *
 *****************************************************/

int main(int argc, char ** argv) 
{
    struct sflc_args args;

    init_crypto();

    /* Parse arguments. */
    if (parse_args(argc - 1, argv + 1, &args) != 0) {
        die("ERR: Error while parsing arguments\n");
    }

    /* Command dispatcher. */
    switch (args.cmd)
    {

    case SFLC_CMD_CREATE_VOLS:
        print_green("Creating %d volumes on real device %s\n", 
                        args.create_vols.nr_pwd, args.create_vols.real_dev_path);
        sflc_create_vols(args.create_vols.no_randfill, args.create_vols.real_dev_path,
        					args.create_vols.pwd, args.create_vols.nr_pwd,
                            args.create_vols.redundant_among, args.create_vols.redundant_within);
        break;
    
    case SFLC_CMD_OPEN_VOLS:
        print_green("Opening %d volumes from real device %s, last password %s\n", 
                        args.open_vols.nr_vols, args.open_vols.real_dev_path, args.open_vols.last_pwd);
        sflc_open_vols(args.open_vols.real_dev_path, args.open_vols.vol_names,
        				args.open_vols.nr_vols, args.open_vols.last_pwd, false,
                        args.open_vols.redundant_among, args.open_vols.redundant_within);
        break;

    case SFLC_CMD_CLOSE_VOLS:
        print_green("Closing all volumes on real device %s\n", args.close_vols.real_dev_path);
        sflc_close_vols(args.close_vols.real_dev_path);
        break;
    
    case SFLC_CMD_HELP:
        show_help(argv[0]);
        break;
    
    default:
        die("ERR: Unknown command, type %s for help\n", argv[0]);
        break;
    }

    cleanup_crypto();

    return 0;
}

/*****************************************************
 *           PRIVATE FUNCTIONS DEFINITIONS           *
 *****************************************************/

static int parse_args(int argc, char **argv, struct sflc_args * args)
{
    int ret;

    /* If there's no command, show help */
    if (argc < 1) {
        args->cmd = SFLC_CMD_HELP;
        return 0;
    }

    /* Else, detect command and dispatch to the right sub-parser */
    if (strcmp(argv[0], COMMAND_CREATE_VOLS_STR) == 0) {
        args->cmd = SFLC_CMD_CREATE_VOLS;
        ret = parse_create_vols_args(argc - 1, argv + 1, &args->create_vols);
    }
    else if (strcmp(argv[0], COMMAND_OPEN_VOLS_STR) == 0) {
        args->cmd = SFLC_CMD_OPEN_VOLS;
        ret = parse_open_vols_args(argc - 1, argv + 1, &args->open_vols);
    }
    else if (strcmp(argv[0], COMMAND_CLOSE_VOLS_STR) == 0) {
        args->cmd = SFLC_CMD_CLOSE_VOLS;
        ret = parse_close_vols_args(argc - 1, argv + 1, &args->close_vols);
    }  
    else {
        print_red("ERR: Unknown command %s\n", argv[0]);
        args->cmd = SFLC_CMD_UNKNOWN;
        ret = EINVAL;
    }

    return ret;
}

static int parse_create_vols_args(int argc, char **argv, struct sflc_create_vols_args * args)
{
	/* If no arguments supplied, fail */
	if (argc == 0) {
		print_red("ERR: no arguments supplied to create command\n");
		return EINVAL;
	}

	/* Check if argument is --no-randfill option */
	if (strcmp(argv[0], OPTION_CREATE_NO_RANDFILL) == 0) {
		// Save it in the struct, and advance the args
		args->no_randfill = true;
		argv += 1;
		argc -= 1;
	}
	else {
		args->no_randfill = false;
	}

    /* Check if argument is --redundant-* option */
	if (strcmp(argv[0], OPTION_REDUNDANT_AMONG) == 0) {
		// Save it in the struct, and advance the args
		args->redundant_among = true;
        args->redundant_within = false;
		argv += 1;
		argc -= 1;
	} else if (strcmp(argv[0], OPTION_REDUNDANT_WITHIN) == 0) {
        // Save it in the struct, and advance the args
        args->redundant_among = false;
		args->redundant_within = true;
		argv += 1;
		argc -= 1;
    }
	else {
		args->redundant_among = false;
        args->redundant_within = false;
	}

    /* Check that there are at least 2 remaining arguments (device and at least one password) */
    if (argc < 2) {
        print_red("ERR: Command %s needs device path and at least one password\n", COMMAND_CREATE_VOLS_STR);
        return EINVAL;
    }

    /* Just assign the real_dev_path */
    args->real_dev_path = argv[0];

    /* Assign the password list */
    args->pwd = argv + 1;
    /* And the password count */
    args->nr_pwd = argc - 1;

    return 0;
}

static int parse_open_vols_args(int argc, char **argv, struct sflc_open_vols_args * args)
{
    /* Check if first argument is --redundant-* option */
	if (strcmp(argv[0], OPTION_REDUNDANT_AMONG) == 0) {
		// Save it in the struct, and advance the args
		args->redundant_among = true;
        args->redundant_within = false;
		argv += 1;
		argc -= 1;
	} else if (strcmp(argv[0], OPTION_REDUNDANT_WITHIN) == 0) {
        // Save it in the struct, and advance the args
        args->redundant_among = false;
		args->redundant_within = true;
		argv += 1;
		argc -= 1;
    }
	else {
		args->redundant_among = false;
        args->redundant_within = false;
	}

    /* Check that there are at least 3 arguments */
    if (argc < 3) {
        print_red("ERR: Command %s accepts at least 3 arguments\n", COMMAND_OPEN_VOLS_STR);
        return EINVAL;
    }

    /* Just assign the real_dev_path */
    args->real_dev_path = argv[0];

    /* Assign the handle list */
    args->vol_names = argv + 1;
    /* And the handle count */
    args->nr_vols = argc - 2;

    /* Assign the last password */
    args->last_pwd = argv[argc - 1];

    return 0;
}

static int parse_close_vols_args(int argc, char **argv, struct sflc_close_vols_args * args)
{
    /* Check that there is exactly 1 argument */
    if (argc != 1) {
        print_red("ERR: Command %s accepts exactly 1 parameter\n", COMMAND_CLOSE_VOLS_STR);
        return EINVAL;
    }

    /* Just assign */
    args->real_dev_path = argv[0];

    return 0;
}

static void show_help(char * bin_name)
{
    printf("Usage:\n\n");

    printf("\t%s %s [--no-randfill] [--redundand-among|redundant-within] <device>  [<pwd1>, ... <pwdN>]\n", bin_name, COMMAND_CREATE_VOLS_STR);
    printf("\t\tCreates N volumes with the given passwords on the given device. Erases pre-existing ones.\n\n");
    
    printf("\t%s %s [--redundand-among|redundant-within] <device> [<volname1>, ... <volnameN>] <last_pwd>\n", bin_name, COMMAND_OPEN_VOLS_STR);
    printf("\t\tOpens N volumes with the given names from the given device, using the provided password for the last volume.\n");
    printf("\t\tNames can't be numbers (reserved)\n\n");
    
    printf("\t%s %s <device>\n", bin_name, COMMAND_CLOSE_VOLS_STR);
    printf("\t\tCloses all volumes on a device.\n\n");
}
