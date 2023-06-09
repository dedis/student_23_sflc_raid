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

/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/

#include <linux/module.h>
#include <linux/device-mapper.h>

#include "sysfs/sysfs.h"
#include "target/target.h"
#include "crypto/symkey/symkey.h"
#include "crypto/rand/rand.h"
#include "utils/pools.h"
#include "utils/workqueues.h"
#include "log/log.h"

/*****************************************************
 *            MODULE FUNCTION PROTOTYPES             *
 *****************************************************/

static int sflc_init(void);
static void sflc_exit(void);

/*****************************************************
 *            MODULE FUNCTIONS DEFINITIONS           *
 *****************************************************/

/* Module entry point, called just once, at module-load time */
static int sflc_init(void)
{
	int ret;

	ret = sflc_rand_init();
	if (ret) {
		pr_err("Could not init rand; error %d\n", ret);
		goto err_rand_init;
	}

	/* Run crypto symkey self test */
	ret = sflc_sk_selftest();
	if (ret) {
		pr_err("Error in crypto symkey self test: %d\n", ret);
		goto err_sk;
	}
	/* Run crypto rand self test */
	ret = sflc_rand_selftest();
	if (ret) {
		pr_err("Error in crypto rand self test: %d\n", ret);
		goto err_rand_selftest;
	}

	/* Create the first sysfs entries */
	ret = sflc_sysfs_init();
	if (ret) {
		pr_err("Could not init sysfs; error %d\n", ret);
		goto err_sysfs;
	}

	/* Init the memory pools */
	ret = sflc_pools_init();
	if (ret) {
		pr_err("Could not init memory pools; error %d\n", ret);
		goto err_pools;
	}

	/* Init the workqueues */
	ret = sflc_queues_init();
	if (ret) {
		pr_err("Could not init workqueues; error %d\n", ret);
		goto err_queues;
	}

	/* Register the DM callbacks */
	ret = dm_register_target(&sflc_target);
	if (ret < 0) {
		pr_err("dm_register failed: %d", ret);
		goto err_dm;
	}

	pr_info("Shufflecake loaded");
	return 0;


err_dm:
	sflc_queues_exit();
err_queues:
	sflc_pools_exit();
err_pools:
	sflc_sysfs_exit();
err_sysfs:
err_rand_selftest:
err_sk:
	sflc_rand_exit();
err_rand_init:
	return ret;
}

/* Module exit point, called just once, at module-unload time */
static void sflc_exit(void)
{
	dm_unregister_target(&sflc_target);
	sflc_queues_exit();
	sflc_pools_exit();
	sflc_sysfs_exit();
	sflc_rand_exit();

	pr_info("Shufflecake unloaded");
	return;
}

/* Declare them as such to the kernel */
module_init(sflc_init);
module_exit(sflc_exit);

/*****************************************************
 *                    MODULE INFO                    *
 *****************************************************/

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Toninov");
