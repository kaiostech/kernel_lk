/* Copyright (c) 2014, The Linux Foundation. All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of The Linux Foundation. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <string.h>
#include <reg.h>
#include <platform.h>
#include "mmc.h"
#include "partition_parser.h"
#include "bootselect_parser.h"

static bool format_data = false;
static bool factory_not_set = false;

bool read_boot_select_partition()
{
	bool ret = false;
	int index;
	uint32_t page_size = 0;
	uint64_t offset;
	struct boot_selection_info *in = NULL;
	char *buf = NULL;

	index = partition_get_index("bootselect");
	if (index == INVALID_PTN) {
		dprintf(INFO, "Unable to locate /bootselect partition\n");
		return ret;
	}

	offset = partition_get_offset(index);
	if(!offset) {
		dprintf(INFO, "partition /bootselect doesn't exist\n");
		return ret;
	}

	/* Get the page size of the device */
	page_size = mmc_page_size();

	buf = (char *) memalign(CACHE_LINE, ROUNDUP(page_size, CACHE_LINE));
	ASSERT(buf != NULL);
	if (mmc_read(offset, (unsigned int *)buf, page_size)) {
		dprintf(INFO, "mmc read failure /bootselect \n");
		free(buf);
		return ret;
	}

	in = (struct boot_selection_info *) buf;
	if ((in->signature == BOOTSELECT_SIGNATURE) &&
			(in->version == BOOTSELECT_VERSION)) {
		if(!(in->state_info & BOOTSELECT_FACTORY))
			factory_not_set = true;
		if ((in->state_info & BOOTSELECT_FORMAT))
			format_data = true;
		ret = true;
	} else {
		dprintf(CRITICAL, "Signature: 0x%08x or version: 0x%08x mismatched of /bootselect\n", in->signature, in->version);
		ASSERT(0);
	}

	free(buf);
	return ret;
}

bool needs_write_protection()
{
	return(factory_not_set && platform_check_secure_fuses());
}

bool check_format_bit()
{
	return(factory_not_set && format_data);
}

int write_protect_partitions(int enable)
{
	unsigned partition_count;
	struct partition_entry *partition_entries;
	unsigned long long ptn, end_ptn;
	unsigned long long size_wp;
	int index, ret = 0;
	int needs_wp = 0;
	uint32_t wp_grp_size;
	void *dev;

	partition_count = partition_max_count();
	partition_entries = partition_get_entries();

	dev = (struct mmc_device *)target_mmc_device();
	if(!dev) {
		dprintf(CRITICAL, "Error: MMC device uninitialized\n");
		return 1;
	}
	/* Get the WP size from the card */
	wp_grp_size = mmc_get_wp_size(dev);
	dprintf(SPEW, "write protect size in bytes %d\n", wp_grp_size);

	if(!wp_grp_size) {
		dprintf(CRITICAL, "Wrong write protect size\n");
		ASSERT(0);
	}

	/* Write protect GPT */
	ptn = 0;
	needs_wp = 1;
	for (index = 0; index < partition_count; index++)
	{
		/* Determine the start offset from where the wp is needed */
		if((!needs_wp) && partition_entries[index].attribute_wp_flag) {
			ptn = partition_get_offset(index);
			needs_wp = 1;
		}

		/* Determine the start offset until the wp is needed */
		if((!partition_entries[index].attribute_wp_flag) && (needs_wp)) {
			end_ptn = partition_get_offset(index);
		}
		else
			continue;

		/* Effective write protect size */
		size_wp = end_ptn - ptn;
		if (!(size_wp % wp_grp_size)) {
			ret = mmc_set_clr_power_on_wp_user(dev, (ptn / MMC_BLK_SZ), size_wp, enable);
			if(ret) {
				dprintf(CRITICAL, "Failed to write protect: %s\n", partition_entries[index].name);
				return 1;
			}
			needs_wp = 0;
			dprintf(SPEW, "Partitions write protected with start offset %llu and size %llu\n", ptn, size_wp);
		} else {
			dprintf(CRITICAL, "Partitions Not aligned correctly\n");
			ASSERT(0);
		}
	}

	if(index == partition_count && needs_wp) {
		size_wp = mmc_get_device_capacity() - ptn;
		if (size_wp >  wp_grp_size) {
			if(size_wp % wp_grp_size)
				ASSERT(0);
			ret = mmc_set_clr_power_on_wp_user(dev, (ptn / MMC_BLK_SZ), size_wp, enable);
			if(ret) {
				dprintf(CRITICAL, "Failed to write protect: %s\n", partition_entries[index-1].name);
				return 1;
			}
		} else {
			dprintf(CRITICAL, "No sufficient hole for write protecting last partition\n");
			ASSERT(0);
		}
	}
	dprintf(SPEW, "write protection done\n");
	return 0;
}
