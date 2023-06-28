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
 * Shufflecake core
 */

/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/

#include <stdio.h>
#include <stdbool.h>

#include "sflc.h"
#include "dmtask.h"
#include "disk.h"
#include "crypto.h"

/*****************************************************
 *                    CONSTANTS                      *
 *****************************************************/

#define PREVIOUS_PWD_FIELD_LEN (SFLC_SECTOR_SIZE - SFLC_USR_SALT_LEN - 2*SFLC_USR_IV_LEN - 2*SFLC_USR_MAC_LEN - 2*SFLC_USR_KEY_LEN)

/*****************************************************
 *           PRIVATE FUNCTIONS PROTOTYPES            *
 *****************************************************/

/* Fills the 4096-byte block with the given information. 
 * Can be used to re-randomise a userland block
 * while preserving the information in it. 
 */
static void packUserlandBlock(char * pwd, char * vek, char * previous_pwd, char * block);

/* 
 * Decrypts the contents of the userland block. Returns < 0 if wrong password.
 */
static int unpackUserlandBlock(char * pwd, char * block, char * vek, char * previous_pwd);

/* Reads the volumes file in the device's sysfs entry, and returns a NULL-terminated
   array of strings, each containing the handle of one of the device's volumes. */
static char ** readDeviceVolumes(char * real_dev_path);

/*****************************************************
 *           PUBLIC FUNCTIONS DEFINITIONS            *
 *****************************************************/


/* Format the headers and fill disk with random data.
   First fill all the userland blocks (from first to last), 
   then fill disk with random data, 
   then open all the volumes for creation, then close them */
void sflc_create_vols(bool no_randfill, char * real_dev_path, char ** pwd, int nr_pwd, bool redundant_among, bool redundant_within)
{
    char block[SFLC_SECTOR_SIZE];
    char vek[SFLC_USR_KEY_LEN];   // Volume encryption key
    int fd;
    ssize_t bytes_written;
    int64_t size;
    char * buf;
    unsigned bufsize = SFLC_SECTOR_SIZE * 64;
    
    if (nr_pwd > SFLC_DEV_MAX_VOLUMES) {
	    die("ERR: Too many passwords\n");
    }

    /* fill disk with random data, unless --no-randfill */
    if (!no_randfill) {
		/* Allocate buffer for random data */
		buf = malloc(bufsize);
		if (!buf) {
			die("ERR: Could not allocate random buffer");
		}

		/* Get size in 4096-byte sectors */
		size = disk_getSize(real_dev_path);
		if (size < 0) {
			die("ERR: Could not get disk size");
		}
		printf("Disk size is %lld blocks\n", size);

		/* Open file */
		fd = open(real_dev_path, O_WRONLY);
		if (fd < 0) {
			perror("ERR: Could not open file:");
			die();
		}

		for (; size > 0; size -= (bytes_written / SFLC_SECTOR_SIZE)) {
			/* Fill random buffer */
			randombytes_buf(buf, bufsize);

			/* Write */
			bytes_written = write(fd, buf, bufsize);
			if (bytes_written < 0) {
				perror("ERR: Could not write:");
				die();
			}
			if (bytes_written != bufsize) {
				print_red("ERR: Only wrote %ld bytes!\n", bytes_written);
			}
		}

        close(fd);
        free(buf);
    }

    /* Format the first N userland blocks */
    int vol_idx;
    for (vol_idx = 0; vol_idx < nr_pwd; vol_idx++) {
        char * previous_pwd = vol_idx == 0 ? NULL : pwd[vol_idx - 1];

        /* Sample vek */
        randombytes_buf(vek, SFLC_USR_KEY_LEN);

        /* Compose the userland block */
        packUserlandBlock(pwd[vol_idx], vek, previous_pwd, block);

        /* Write it at the appropriate position on the disk */
        uint64_t block_pos = vol_idx * SFLC_VOL_HEADER_SIZE;
        int err = disk_writeSector(real_dev_path, block_pos, block);
        if (err) {
            die("ERR: Could not write userland block to disk");
        }
    }
    /* Erase the other ones */
    for (; vol_idx < SFLC_DEV_MAX_VOLUMES; vol_idx++) {
        /* Fill with random bytes */
        randombytes_buf(block, SFLC_SECTOR_SIZE);

        /* Write it at the appropriate position on the disk */
        uint64_t block_pos = vol_idx * SFLC_VOL_HEADER_SIZE;
        int err = disk_writeSector(real_dev_path, block_pos, block);
        if (err) {
            die("ERR: Could not write userland block to disk");
        }
    }
    
    
    /* Open all the volumes and then close them (dirty hack to initialize 
    position maps */

    /* Create temporary volume names */
    char *vol_names[SFLC_DEV_MAX_VOLUMES];
    for (vol_idx = 0; vol_idx < nr_pwd; vol_idx++) {
        vol_names[vol_idx] = malloc(4 * sizeof(char));
        if (vol_names[vol_idx] == NULL) {
            die("ERR: Could not allocate 4 chars");
        }
        sprintf(vol_names[vol_idx], "%d", vol_idx);
    }

    // sflc-raid START
    /* Open volumes */
    sflc_open_vols(real_dev_path, vol_names, nr_pwd, pwd[nr_pwd - 1], true, redundant_among, redundant_within);
    // sflc-raid END

    /* Close volumes */
    sflc_close_vols(real_dev_path);

    return;
}

/* Open the last volume, then call recursively */
void sflc_open_vols(char * real_dev_path, char ** vol_names, int nr_vols, char * last_pwd, bool vol_creation, bool redundant_among, bool redundant_within)
{
    char block[SFLC_SECTOR_SIZE];
    char vek[SFLC_USR_KEY_LEN];   // Volume encryption key
    char previous_pwd[SFLC_USR_PWD_MAX_LEN + 1]; // Previous volume's password
    char * vek_hex;
    char virt_dev_name[64];
    uint64_t tot_slices;
    char param[512];
    int err;

    /* Manage the userland block */

    /* Read it from the disk at the appropriate position */
    int vol_idx = nr_vols - 1;
    uint64_t block_pos = vol_idx * SFLC_VOL_HEADER_SIZE;
    err = disk_readSector(real_dev_path, block_pos, block);

    /* Interpret it to get the vek and the previous volume's password */
    err = unpackUserlandBlock(last_pwd, block, vek, previous_pwd);
    if (err) {
        die("ERR: Wrong password %s for volume %d", last_pwd, vol_idx);
    }

    /* Get the vek's hex representation */
    vek_hex = hex_encode(vek, SFLC_USR_KEY_LEN);


    /* Send the create command to dm_sflc */

    /* Compute the number of slices */
    int64_t dev_size = disk_getSize(real_dev_path);
    if (dev_size < 0) {
        die("ERR: Could not get device size");
    }
    if (dev_size < SFLC_DEV_HEADER_SIZE) {
        die("ERR: Disk not big enough to host device header");
    }
    tot_slices = (dev_size - SFLC_DEV_HEADER_SIZE) / SFLC_PHYS_SLICE_SIZE;
    if (tot_slices > SFLC_VOL_MAX_SLICES) {
        tot_slices = SFLC_VOL_MAX_SLICES;
    }

    /* Construct virtual device name (as it will appear under /dev/mapper) */
    char * handle = vol_names[vol_idx];
    sprintf(virt_dev_name, "sflc-%s", handle);

    /* Construct parameter list to pass to dm-sflc kernel module */
    char creation_flag = vol_creation ? 'c' : 'o';
    // sflc-raid START
    char redundant = 'n';
    redundant = redundant_among ? 'a' : redundant;
    redundant = redundant_within ? 'w' : redundant;
    sprintf(param, "%s %s %d %c %llu %s %c", real_dev_path, handle, vol_idx, creation_flag, tot_slices, vek_hex, redundant);
    // sflc-raid END

    if (!sflc_dmt_create(virt_dev_name, tot_slices * SFLC_LOG_SLICE_SIZE * SFLC_SECTOR_SCALE, param)){
        die("ERR: Error in dmt_create");
    }

    /* Call recursively */

    /* Only if there are more volumes to open */
    if (nr_vols > 1) {
        sflc_open_vols(real_dev_path, vol_names, nr_vols - 1, previous_pwd, vol_creation, redundant_among, redundant_within);
    }

    return;
}

void sflc_close_vols(char * real_dev_path)
{
    char ** handles;
    char virt_dev_name[64];

    /* Read all volume names in the device */
    handles = readDeviceVolumes(real_dev_path);

    /* Close all volumes, from first to last */
    int i;
    for (i = 0; handles[i] != NULL; i++) {
        /* Construct virtual device name (as it appears under /dev/mapper) */
        sprintf(virt_dev_name, "sflc-%s", handles[i]);

        /* Close this volume */
        if (!sflc_dmt_destroy(virt_dev_name)) {
            die("ERR: Could not close volume%s", handles[i]);
        }
    }

    return;
}

/*****************************************************
 *           PRIVATE FUNCTIONS PROTOTYPES            *
 *****************************************************/

/* Fills the 4096-byte block with the given information. 
 * Can be used to re-randomise a userland block
 * while preserving the information in it. 
 */
static void packUserlandBlock(char * pwd, char * vek, char * previous_pwd, char * block)
{
    /* Stuff to be encrypted */
    char keys[2*SFLC_USR_KEY_LEN];
    char * ppek = keys + SFLC_USR_KEY_LEN; // Previous password encryption key
    char padded_pp[PREVIOUS_PWD_FIELD_LEN] = {'\0'};

    /* Copy the vek */
    memcpy(keys, vek, SFLC_USR_KEY_LEN);
    /* Sample the ppek */
    randombytes_buf(ppek, SFLC_USR_KEY_LEN);

    /* Copy the previous password */
    if (previous_pwd != NULL) {
        strncpy(padded_pp, previous_pwd, PREVIOUS_PWD_FIELD_LEN-1);
    }

    /* Pointers inside the block */
    char * salt = block;
    char * keys_iv = salt + SFLC_USR_SALT_LEN;
    char * encrypted_keys = keys_iv + SFLC_USR_IV_LEN;
    char * keys_mac = encrypted_keys + 2*SFLC_USR_KEY_LEN;
    char * pp_iv = keys_mac + SFLC_USR_MAC_LEN;
    char * encrypted_pp = pp_iv + SFLC_USR_IV_LEN;
    char * pp_mac = encrypted_pp + PREVIOUS_PWD_FIELD_LEN;
    /* Key encryption key */
    char kek[SFLC_USR_KEY_LEN];

    /* Sample salt */
    randombytes_buf(salt, SFLC_USR_SALT_LEN);
    /* Sample IVs */
    randombytes_buf(keys_iv, SFLC_USR_IV_LEN);
    randombytes_buf(pp_iv, SFLC_USR_IV_LEN);

    /* Derive kek */
    kdf_pwhash(pwd, SFLC_USR_KEY_LEN, kek, salt);

    /* Encrypt the keys */
    ae_encrypt(keys, 2*SFLC_USR_KEY_LEN, encrypted_keys, keys_mac, kek, keys_iv);

    /* Encrypt the previous password */
    ae_encrypt(padded_pp, PREVIOUS_PWD_FIELD_LEN, encrypted_pp, pp_mac, ppek, pp_iv);

    return;
}

/* 
 * Decrypts the contents of the userland block. Returns < 0 if wrong password.
 */
static int unpackUserlandBlock(char * pwd, char * block, char * vek, char * previous_pwd)
{
    /* Stuff to be decrypted */
    char keys[2*SFLC_USR_KEY_LEN];
    char * ppek = keys + SFLC_USR_KEY_LEN; // Previous password encryption key
    char padded_pp[PREVIOUS_PWD_FIELD_LEN] = {'\0'};

    /* Pointers inside the block */
    char * salt = block;
    char * keys_iv = salt + SFLC_USR_SALT_LEN;
    char * encrypted_keys = keys_iv + SFLC_USR_IV_LEN;
    char * keys_mac = encrypted_keys + 2*SFLC_USR_KEY_LEN;
    char * pp_iv = keys_mac + SFLC_USR_MAC_LEN;
    char * encrypted_pp = pp_iv + SFLC_USR_IV_LEN;
    char * pp_mac = encrypted_pp + PREVIOUS_PWD_FIELD_LEN;
    /* Key encryption key */
    char kek[SFLC_USR_KEY_LEN];

    /* Derive kek */
    kdf_pwhash(pwd, SFLC_USR_KEY_LEN, kek, salt);

    /* Decrypt the keys */
    if (ae_decrypt(encrypted_keys, 2*SFLC_USR_KEY_LEN, keys_mac, keys, kek, keys_iv) < 0) {
        print_red("ERR: Failed keys decryption: wrong password\n");
        return -1;
    }

    /* Copy the vek */
    memcpy(vek, keys, SFLC_USR_KEY_LEN);

    /* Decrypt the previous password */
    if (ae_decrypt(encrypted_pp, PREVIOUS_PWD_FIELD_LEN, pp_mac, padded_pp, ppek, pp_iv) < 0) {
        /* Should never happen */
        die("ERR: WTF? Previous password decryption failed!");
    }

    /* Copy it into the output parameter */
    if (padded_pp[PREVIOUS_PWD_FIELD_LEN - 1] != '\0') {
        /* Should never happen */
        die("ERR: WTF? Decrypted padded_pp does not end with \\0!");
    }
    strcpy(previous_pwd, padded_pp);

    return 0;
}

/* Reads the volumes file in the device's sysfs entry, and returns a NULL-terminated
   array of strings, each containing the handle of one of the device's volumes. */
static char ** readDeviceVolumes(char * real_dev_path)
{
    char ** handles;
    char * filecontent;
    char * modified_dev_path;
    char filepath[256];

    /* Get rid of slashes in the real_dev_path */
    modified_dev_path = strdup(real_dev_path);
    str_replaceAll(modified_dev_path, '/', '_');

    /* Construct filepath */
    sprintf(filepath, "/sys/module/dm_sflc/realdevs/%s/volumes", modified_dev_path);

    /* Read file */
    filecontent = read_file_contents(filepath);
    print_green("File content:\n%s\nEnd of file content\n", filecontent);

    /* Parse it */

    /* Get number of volumes */
    int nr_vols;
    sscanf(filecontent, "%d", &nr_vols);

    /* Allocate */
    handles = malloc((nr_vols + 1) * sizeof(char *));
    if (handles == NULL) {
        die("ERR: Could not allocate %d handle pointers", nr_vols + 1);
    }

    /* Fill */
    int i;
    int vol_idx = 0;
    /* The file ends with a newline */
    for (i = 0; filecontent[i] != '\n'; i++) {
        if (filecontent[i] == ' ') {
            /* New handle starts after */
            filecontent[i] = '\0';
            handles[vol_idx++] = filecontent + (i+1);
        }
    }
    /* End the last string */
    filecontent[i] = '\0';
    /* NULL-terminate the array of strings */
    handles[nr_vols] = NULL;
    
    print_green("Volume handles\n");
    int j;
    for (j = 0; handles[j] != NULL; j++) {
		print_green("%s ", handles[j]);
	}
	print_green("\nEnd of volume handles\n");

    return handles;
}
