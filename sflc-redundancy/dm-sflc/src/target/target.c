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
 * Methods of our DM target
 */

/*****************************************************
 *                  INCLUDE SECTION                  *
 *****************************************************/

#include "target.h"
#include "device/device.h"
#include "volume/volume.h"
#include "utils/bio.h"
#include "utils/string.h"
#include "log/log.h"
#include "utils/pools.h"

/*****************************************************
 *                    CONSTANTS                      *
 *****************************************************/

/*****************************************************
 *                       TYPES                       *
 *****************************************************/


// sflc-raid START
struct sflc_slice_corr_s
{
	sflc_Volume *donor_volume;
	sflc_Volume *receiver_volume;
	u32 slice_index;
};
typedef struct sflc_slice_corr_s sflc_Slice_Corr;
// sflc-raid END

/*****************************************************
 *           PRIVATE FUNCTIONS PROTOTYPES            *
 *****************************************************/

static int sflc_tgt_ctr(struct dm_target *ti, unsigned int argc, char **argv);
static void sflc_tgt_dtr(struct dm_target *ti);
static int sflc_tgt_map(struct dm_target *ti, struct bio *bio);
static void sflc_tgt_ioHints(struct dm_target *ti, struct queue_limits *limits);
static int sflc_tgt_iterateDevices(struct dm_target *ti, iterate_devices_callout_fn fn,
								   void *data);

// sflc-raid START
int slice_transfusion(sflc_Device *dev, sflc_Volume *donor_volume, sflc_Volume *receiver_volume, u32 donoe_slice, u32 receiver_slice);
// sflc-raid END

/*****************************************************
 *           PUBLIC VARIABLES DEFINITIONS            *
 *****************************************************/

struct target_type sflc_target = {
	.name = "shufflecake",
	.version = {1, 0, 0},
	.module = THIS_MODULE,
	.ctr = sflc_tgt_ctr,
	.dtr = sflc_tgt_dtr,
	.map = sflc_tgt_map,
	.status = NULL,
	.io_hints = sflc_tgt_ioHints,
	.iterate_devices = sflc_tgt_iterateDevices,
};

// sflc-raid START
sflc_Volume *volume_links[SFLC_DEV_MAX_VOLUMES];
char redundancy;
// sflc-raid END

/*****************************************************
 *           PRIVATE FUNCTIONS DEFINITIONS           *
 *****************************************************/

/* Called every time we create a volume with the userland tool */
static int sflc_tgt_ctr(struct dm_target *ti, unsigned int argc, char **argv)
{
	char *real_dev_path;
	char *vol_name;
	int vol_idx;
	bool vol_creation;
	char *enckey_hex;
	u8 enckey[SFLC_SK_KEY_LEN];
	u32 tot_slices;
	sflc_Device *dev;
	sflc_Volume *vol;
	int err;

	// sflc-raid START
	bool redundant_among;
	bool redundant_within;

	/*
	 * Parse arguments.
	 *
	 * argv[0]: real device path
	 * argv[1]: Shufflecake-unique volume name
	 * argv[2]: volume index within the device
	 * argv[3]: 'c' for volume creation, 'o' for volume opening
	 * argv[4]: number of 1 MB slices in the underlying device
	 * argv[5]: 32-byte encryption key (hex-encoded)
	 * argv[6]: redundancy implementation
	 */

	if (argc != 7)
	{
		ti->error = "Invalid argument count";
		return -EINVAL;
	}
	real_dev_path = argv[0];
	vol_name = argv[1];
	sscanf(argv[2], "%d", &vol_idx);
	vol_creation = (argv[3][0] == 'c');
	sscanf(argv[4], "%u", &tot_slices);
	enckey_hex = argv[5];
	redundant_among = (argv[6][0] == 'a');
	redundant_within = (argv[6][0] == 'w');

	/* Decode the encryption key */
	if (strlen(enckey_hex) != 2 * SFLC_SK_KEY_LEN)
	{
		pr_err("Hexadecimal key (length %lu): %s\n", strlen(enckey_hex), enckey_hex);
		ti->error = "Invalid key length";
		return -EINVAL;
	}
	err = sflc_str_hexDecode(enckey_hex, enckey);
	if (err)
	{
		ti->error = "Could not decode hexadecimal encryption key";
		return err;
	}

	/* Acquire the big device lock */
	if (down_interruptible(&sflc_dev_mutex))
	{
		ti->error = "Interrupted while waiting for access to Device";
		return -EINTR;
	}

	/* Check if we already have a device for this real device */
	dev = sflc_dev_lookupByPath(real_dev_path);
	if (IS_ERR(dev))
	{
		ti->error = "Could not look up device by path (interrupted)";
		up(&sflc_dev_mutex);
		return PTR_ERR(dev);
	}

	/* Otherwise create it (also adds it to the device list) */
	if (!dev)
	{
		pr_notice("Device on %s didn't exist before, going to create it\n", real_dev_path);
		dev = sflc_dev_getDevice(ti, real_dev_path, tot_slices);
	}
	else
	{
		pr_notice("Device on %s already existed\n", real_dev_path);
	}

	/* Check for device creation errors */
	if (IS_ERR(dev))
	{
		ti->error = "Could not create device";
		up(&sflc_dev_mutex);
		return PTR_ERR(dev);
	}

	/* Check that the provided volume name is actually unique across Shufflecake */
	vol = sflc_dev_lookupVolumeByName(vol_name);
	if (vol)
	{
		ti->error = "Volume name already exists";
		up(&sflc_dev_mutex);
		return -EINVAL;
	}

	/* Create the volume (also adds it to the device) */
	vol = sflc_vol_getVolume(ti, vol_name, dev, vol_idx, enckey, vol_creation);
	if (IS_ERR(vol))
	{
		ti->error = "Error creating volume";
		up(&sflc_dev_mutex);
		return PTR_ERR(vol);
	}
	pr_debug("Now %d volumes are linked to device %s\n", dev->vol_cnt, real_dev_path);

	/* Release the big device lock */
	up(&sflc_dev_mutex);

	/* Tell DM we want one SFLC sector at a time */
	ti->max_io_len = SFLC_DEV_SECTOR_SCALE;
	/* Enable REQ_OP_FLUSH bios */
	ti->num_flush_bios = 1;
	/* Disable REQ_OP_WRITE_ZEROES and REQ_OP_SECURE_ERASE (can't be passed through as
	   they would break deniability, and they would be too complicated to handle individually) */
	ti->num_secure_erase_bios = 0;
	ti->num_write_zeroes_bios = 0;
	/* Momentarily disable REQ_OP_DISCARD_BIOS
	   TODO: will need to support them to release slice mappings */
	ti->num_discard_bios = 0;
	/* When we receive a ->map call, we won't need to take the device lock anymore */
	ti->private = vol;

	/* DEBUG : list allocated slices
	u32 i;
	for (i = 0; i < dev->tot_slices; i++)
	{
		if (vol->fmap[i] != SFLC_VOL_FMAP_INVALID_PSI)
		{
			pr_alert("VOLUME:%s:%u->%u\n", vol->vol_name, i, vol->fmap[i]);
		}
	}
	pr_alert("VOLUME:%s:%u/%u\n", vol->vol_name, vol->mapped_slices, dev->tot_slices);
	DEBUG */

	if (redundant_among)
	{
		redundancy = 'a';
	}
	else if (redundant_within)
	{
		redundancy = 'w';

		if (dev->tot_slices % 2 != 0)
		{
			dev->tot_slices -= 1;
			pr_info("Reduced total number of slices to %d to be even\n", dev->tot_slices);
		}
	}
	else
	{
		redundancy = 'n';

		/* No redundancy */
		err = 0;
		return err;
	}

	// Remembering volume
	volume_links[vol->vol_idx] = vol;

	// When last volume was added, check for corruption
	if (!vol_creation && vol->vol_idx == 0)
	{
		int corr_index = 0;
		sflc_Slice_Corr *slice_corr = NULL;

		pr_info("Checking for corrupted slices\n");
		int low_idx;
		int high_idx;
		u32 i;
		u32 j;
		for (low_idx = 0; low_idx < SFLC_DEV_MAX_VOLUMES; low_idx++)
		{
			if (volume_links[low_idx] != NULL)
			{
				for (high_idx = low_idx + 1; high_idx < SFLC_DEV_MAX_VOLUMES; high_idx++)
				{
					if (volume_links[high_idx] != NULL)
					{
						for (i = 0; i < dev->tot_slices; i++)
						{
							if (volume_links[low_idx]->fmap[i] != SFLC_VOL_FMAP_INVALID_PSI)
							{
								for (j = 0; j < dev->tot_slices; j++)
								{
									if (volume_links[high_idx]->fmap[j] != SFLC_VOL_FMAP_INVALID_PSI)
									{
										if (volume_links[low_idx]->fmap[i] == volume_links[high_idx]->fmap[j])
										{
											pr_info("Slice conflict, volume %d : %u -> %u and volume %d : %u -> %u\n", low_idx + 1, i, volume_links[low_idx]->fmap[i], high_idx + 1, j, volume_links[high_idx]->fmap[j]);

											sflc_Volume *donor_volume;
											sflc_Volume *receiver_volume = volume_links[high_idx];

											if (redundancy == 'a')
											{
												if (receiver_volume->vol_idx % 2 == 0)
												{
													donor_volume = volume_links[high_idx - 1];
												}
												else
												{
													donor_volume = volume_links[high_idx + 1];
												}
											}
											else if (redundancy == 'w')
											{
												donor_volume = receiver_volume;
											}
											else
											{
												pr_warn("Unrecognizable redundancy, not supposed to be here, what happened ?\n");
												continue;
											}

											if (donor_volume == NULL)
											{
												pr_warn("Something is wrong, the donor volume is missing, cancelling this transfusion\n");
												continue;
											}

											corr_index += 1;
											slice_corr = krealloc(slice_corr, corr_index * sizeof(sflc_Slice_Corr), GFP_KERNEL);
											slice_corr[corr_index - 1] = (sflc_Slice_Corr){donor_volume, receiver_volume, j};
										}
									}
								}
							}
						}
					}
				}
			}
		}
		pr_info("Detected %d slice inconsistencies\n", corr_index);

		int fixed_slices = 0;

		int k;
		for (k = 0; k < corr_index; k++)
		{
			sflc_Volume *donor_volume = slice_corr[k].donor_volume;
			sflc_Volume *receiver_volume = slice_corr[k].receiver_volume;

			u32 receiver_slice = slice_corr[k].slice_index;
			u32 donor_slice;

			if (redundancy == 'a')
			{
				donor_slice = receiver_slice;
			}
			else if (redundancy == 'w')
			{
				if (receiver_slice % 2 == 0)
				{
					donor_slice = receiver_slice + 1;
				}
				else
				{
					donor_slice = receiver_slice - 1;
				}
			}
			else
			{
				pr_warn("Unrecognizable redundancy, not supposed to be here, what happened ?\n");
				continue;
			}

			if (donor_slice < 0 || donor_slice > dev->tot_slices)
			{
				pr_warn("Something is wrong, the donor slice %d in %d is unreachable, cancelling this transfusion\n", receiver_slice, donor_volume->vol_idx + 1);
				continue;
			}

			bool double_corr = false;

			int l;
			for (l = 0; l < corr_index; l++)
			{
				if (donor_slice == slice_corr[l].slice_index && donor_volume->vol_idx == slice_corr[l].receiver_volume->vol_idx)
				{
					double_corr = true;
					break;
				}
			}

			// Discard slice for receiver volume
			receiver_volume->fmap[receiver_slice] = SFLC_VOL_FMAP_INVALID_PSI;
			// Allocate new slice
			sflc_vol_mapSlice(receiver_volume, receiver_slice, WRITE); // WRITE to force allocation

			if (double_corr)
			{
				pr_warn("Detected double corruption, skipping transfusion from volume %d to %d from slice %d to %d\n", donor_volume->vol_idx + 1, receiver_volume->vol_idx + 1, donor_slice, receiver_slice);
				continue;
			}

			err = slice_transfusion(dev, donor_volume, receiver_volume, donor_slice, receiver_slice);
			if (err)
			{
				pr_err("Slice transfusion failed from volume %d to %d from slice %d to %d\n", donor_volume->vol_idx + 1, receiver_volume->vol_idx + 1, donor_slice, receiver_slice);
				return err;
			}

			fixed_slices += 1;
		}

		pr_info("Redundancy repair finished, fixed %d of %d corrupted slices\n", fixed_slices, corr_index);

		if (slice_corr != NULL)
		{
			kfree(slice_corr);
		}
	}

	/* No error if we made it here */
	err = 0;
	// sflc-raid END

	return err;
}

// sflc-raid START
int slice_transfusion(sflc_Device *dev, sflc_Volume *donor_volume, sflc_Volume *receiver_volume, u32 donor_slice, u32 receiver_slice)
{
	sector_t data_start_sector;
	sector_t donor_sector;
	sector_t receiver_sector;
	struct page *iv_donor_page;
	struct page *iv_receiver_page;
	u8 *iv_donor_ptr;
	u8 *iv_receiver_ptr;
	struct page *data_page;
	u8 *data_ptr;
	int err;

	/* Allocate pages */
	iv_donor_page = mempool_alloc(sflc_pools_pagePool, GFP_NOIO);
	if (!iv_donor_page)
	{
		pr_err("Could not allocate IV donor page\n");
		return -ENOMEM;
	}
	iv_receiver_page = mempool_alloc(sflc_pools_pagePool, GFP_NOIO);
	if (!iv_receiver_page)
	{
		pr_err("Could not allocate IV receiver page\n");
		return -ENOMEM;
	}
	data_page = mempool_alloc(sflc_pools_pagePool, GFP_NOIO);
	if (!data_page)
	{
		pr_err("Could not allocate data page\n");
		return -ENOMEM;
	}
	/* Kmap them */
	iv_donor_ptr = kmap(iv_donor_page);
	iv_receiver_ptr = kmap(iv_receiver_page);
	data_ptr = kmap(data_page);

	/* Lock both the forward and the reverse position maps */
	if (mutex_lock_interruptible(&donor_volume->fmap_lock))
	{
		pr_err("Interrupted while waiting to lock fmap\n");
		return -EINTR;
	}
	if (mutex_lock_interruptible(&receiver_volume->fmap_lock))
	{
		mutex_unlock(&donor_volume->fmap_lock);
		pr_err("Interrupted while waiting to lock fmap\n");
		return -EINTR;
	}
	if (mutex_lock_interruptible(&dev->rmap_lock))
	{
		pr_err("Interrupted while waiting to lock rmap\n");
		mutex_unlock(&donor_volume->fmap_lock);
		mutex_unlock(&receiver_volume->fmap_lock);
		return -EINTR;
	}

	/* Starting sector of data section */
	data_start_sector = SFLC_DEV_HEADER_SIZE;

	/* Starting sector of the physical slices */
	donor_sector = data_start_sector + (donor_volume->fmap[donor_slice] * SFLC_DEV_PHYS_SLICE_SIZE);
	receiver_sector = data_start_sector + (receiver_volume->fmap[receiver_slice] * SFLC_DEV_PHYS_SLICE_SIZE);

	/* Read IV-data of donor slice */
	err = sflc_dev_rwSector(dev, iv_donor_page, donor_sector, READ);
	if (err)
	{
		pr_err("Could not read IV donor block %d at sector %llu; error %d\n", donor_volume->vol_idx + 1, donor_sector, err);
		goto out;
	}
	donor_sector += 1;

	/* Read IV-data of receiver slice */
	err = sflc_dev_rwSector(dev, iv_receiver_page, receiver_sector, READ);
	if (err)
	{
		pr_err("Could not read IV receiver block %d at sector %llu; error %d\n", receiver_volume->vol_idx + 1, receiver_sector, err);
		goto out;
	}
	receiver_sector += 1;

	int k;
	for (k = 0; k < SFLC_DEV_SECTOR_TO_IV_RATIO; k++)
	{
		/* Load the data block from donor */
		err = sflc_dev_rwSector(dev, data_page, donor_sector, READ);
		if (err)
		{
			pr_err("Could not read data block from donor %d at sector %llu; error %d\n", donor_volume->vol_idx + 1, donor_sector, err);
			goto out;
		}
		donor_sector += 1;

		/* Decrypt it in place with donor crypto */
		err = sflc_sk_decrypt(donor_volume->skctx, data_ptr, data_ptr, SFLC_DEV_SECTOR_SIZE, (iv_donor_ptr + donor_slice * SFLC_SK_IV_LEN));
		if (err)
		{
			pr_err("Could not decrypt data block from donor %d at sector %llu; error %d\n", donor_volume->vol_idx + 1, donor_sector, err);
			goto out;
		}

		/* Encrypt it in place with receiver crypto */
		err = sflc_sk_encrypt(receiver_volume->skctx, data_ptr, data_ptr, SFLC_DEV_SECTOR_SIZE, (iv_receiver_ptr + receiver_slice * SFLC_SK_IV_LEN));
		if (err)
		{
			pr_err("Could not encrypt data block to receiver %d at sector %llu; error %d\n", receiver_volume->vol_idx + 1, receiver_sector, err);
			goto out;
		}

		/* Store the data block to receiver */
		err = sflc_dev_rwSector(dev, data_page, receiver_sector, WRITE);
		if (err)
		{
			pr_err("Could not write data block to receiver %d at sector %llu; error %d\n", receiver_volume->vol_idx + 1, receiver_sector, err);
			goto out;
		}
		receiver_sector += 1;
	}

	/* No error if we made it here */
	err = 0;

out:
	/* Unlock all maps */
	mutex_unlock(&dev->rmap_lock);
	mutex_unlock(&receiver_volume->fmap_lock);
	mutex_unlock(&donor_volume->fmap_lock);
	/* Kunmap pages */
	kunmap(iv_donor_page);
	kunmap(iv_receiver_page);
	kunmap(data_page);
	/* Free them */
	mempool_free(iv_donor_page, sflc_pools_pagePool);
	mempool_free(iv_receiver_page, sflc_pools_pagePool);
	mempool_free(data_page, sflc_pools_pagePool);

	return err;
}
// sflc-raid END

/* Called every time we destroy a volume with the userland tool */
static void sflc_tgt_dtr(struct dm_target *ti)
{
	sflc_Volume *vol = ti->private;
	sflc_Device *dev = vol->dev;

	pr_debug("Destroying volume \"%s\"\n", vol->vol_name);

	/* We do need to take the device lock here, as we'll be modifying the device */
	if (down_interruptible(&sflc_dev_mutex))
	{
		pr_err("Interrupted while waiting to destroy volume\n");
		return;
	}

	/* Destroy volume (also decreases refcount in device) */
	sflc_vol_putVolume(ti, vol);

	/* Destroy the device */
	if (dev->vol_cnt == 0)
	{
		pr_notice("Removed the last volume from device\n");
		sflc_dev_putDevice(ti, dev);
	}

	/* End of critical section */
	up(&sflc_dev_mutex);

	// sflc-raid START
	if (redundancy != 'n')
	{
		// Forgetting volume
		volume_links[vol->vol_idx] = NULL;
	}
	// sflc-raid END

	return;
}

/* Callback for every bio submitted to our virtual block device */
static int sflc_tgt_map(struct dm_target *ti, struct bio *bio)
{
	int err;
	sflc_Volume *vol = ti->private;

	/* If no data, just quickly remap the sector and the block device (no crypto) */
	/* TODO: this is dangerous for deniability, will need more filtering */
	if (unlikely(!bio_has_data(bio)))
	{
		pr_debug("No-data bio: bio_op = %d", bio_op(bio));

		err = sflc_vol_remapBioFast(vol, bio);
		if (err)
		{
			pr_err("Could not remap bio; error %d\n", err);
			return DM_MAPIO_KILL;
		}

		return DM_MAPIO_REMAPPED;
	}

	/* At this point, the bio has data. Do a few sanity checks */
	/* TODO: I think we can get rid of all of them */

	/* Check that it is properly aligned and it doesn't cross vector boundaries */
	if (unlikely(!sflc_bio_isAligned(bio)))
	{
		pr_err("Unaligned bio!\n");
		return DM_MAPIO_KILL;
	}
	/* If it contains more than one SFLC sector, complain with the DM layer and continue */
	if (unlikely(bio->bi_iter.bi_size > SFLC_DEV_SECTOR_SIZE))
	{
		pr_notice("Large bio of size %u\n", bio->bi_iter.bi_size);
		dm_accept_partial_bio(bio, SFLC_DEV_SECTOR_SCALE);
	}
	/* Check that it contains exactly one SFLC sector */
	if (unlikely(bio->bi_iter.bi_size != SFLC_DEV_SECTOR_SIZE))
	{
		pr_err("Wrong length (%u) of bio\n", bio->bi_iter.bi_size);
		return DM_MAPIO_KILL;
	}

	// sflc-raid START
	if (redundancy != 'n' && vol->vol_idx != 0)
	{
		if (redundancy == 'a')
		{
			if (vol->vol_idx % 2 == 0)
			{
				err = sflc_vol_processBioRedundantlyAmong(vol, volume_links[vol->vol_idx - 1], bio);
				if (err)
				{
					pr_err("Could not enqueue bio\n");
					return DM_MAPIO_KILL;
				}
			}
			else
			{
				err = sflc_vol_processBioRedundantlyAmong(vol, volume_links[vol->vol_idx + 1], bio);
				if (err)
				{
					pr_err("Could not enqueue bio\n");
					return DM_MAPIO_KILL;
				}
			}
		}
		else if (redundancy == 'w')
		{
			err = sflc_vol_processBioRedundantlyWithin(vol, bio);
			if (err)
			{
				pr_err("Could not enqueue bio\n");
				return DM_MAPIO_KILL;
			}
		}
		else
		{
			pr_warn("Unrecognizable redundancy, not supposed to be here, what happened ?\n");
			return DM_MAPIO_SUBMITTED;
		}
	}
	else
	{
		/* Now it is safe, process it */
		err = sflc_vol_processBio(vol, bio);
		if (err)
		{
			pr_err("Could not enqueue bio\n");
			return DM_MAPIO_KILL;
		}
	}
	// sflc-raid END

	return DM_MAPIO_SUBMITTED;
}

/* Callback executed to inform the DM about our 4096-byte sector size */
static void sflc_tgt_ioHints(struct dm_target *ti, struct queue_limits *limits)
{
	sflc_Volume *vol = ti->private;

	pr_info("Called io_hints on volume \"%s\"\n", vol->vol_name);

	limits->logical_block_size = SFLC_DEV_SECTOR_SIZE;
	limits->physical_block_size = SFLC_DEV_SECTOR_SIZE;

	limits->io_min = SFLC_DEV_SECTOR_SIZE;
	limits->io_opt = SFLC_DEV_SECTOR_SIZE;

	return;
}

/* Callback needed for God knows what, otherwise io_hints never gets called */
static int sflc_tgt_iterateDevices(struct dm_target *ti, iterate_devices_callout_fn fn,
								   void *data)
{
	sflc_Volume *vol = ti->private;
	sflc_Device *dev = vol->dev;

	pr_info("Called iterate_devices on volume \"%s\"\n", vol->vol_name);

	if (!fn)
	{
		return -EINVAL;
	}
	return fn(ti, vol->dev->real_dev, 0,
			  (SFLC_DEV_HEADER_SIZE + dev->tot_slices * SFLC_DEV_PHYS_SLICE_SIZE) * SFLC_DEV_SECTOR_SCALE,
			  data);
}
