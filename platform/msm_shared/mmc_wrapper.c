/* Copyright (c) 2013, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
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
#include <stdint.h>
#include <mmc_sdhci.h>
#include <mmc_wrapper.h>
#include <sdhci.h>
#include <target.h>
#include <string.h>

/*
 * Function: get mmc card
 * Arg     : None
 * Return  : Pointer to mmc card structure
 * Flow    : Get the card pointer from the device structure
 */
static struct mmc_card *get_mmc_card()
{
	struct mmc_device *dev;
	struct mmc_card *card;

	dev = target_mmc_device();
	card = &dev->card;

	return card;
}

/*
 * Function: mmc_write
 * Arg     : Data address on card, data length, i/p buffer
 * Return  : 0 on Success, non zero on failure
 * Flow    : Write the data from in to the card
 */
uint32_t mmc_write(uint64_t data_addr, uint32_t data_len, void *in)
{
	uint32_t val = 0;
	uint32_t write_size = SDHCI_ADMA_MAX_TRANS_SZ;
	uint8_t *sptr = (uint8_t *)in;
	struct mmc_device *dev;

	dev = target_mmc_device();

	ASSERT(!(data_addr % MMC_BLK_SZ));

	if (data_len % MMC_BLK_SZ)
		data_len = ROUNDUP(data_len, MMC_BLK_SZ);

	/* TODO: This function is aware of max data that can be
	 * tranferred using sdhci adma mode, need to have a cleaner
	 * implementation to keep this function independent of sdhci
	 * limitations
	 */
	while (data_len > write_size) {
		val = mmc_sdhci_write(dev, (void *)sptr, (data_addr / MMC_BLK_SZ), (write_size / MMC_BLK_SZ));
		if (val)
		{
			dprintf(CRITICAL, "Failed Writing block @ %x\n", (data_addr / MMC_BLK_SZ));
			return val;
		}
		sptr += write_size;
		data_addr += write_size;
		data_len -= write_size;
	}

	if (data_len)
		val = mmc_sdhci_write(dev, (void *)sptr, (data_addr / MMC_BLK_SZ), (data_len / MMC_BLK_SZ));

	if (val)
		dprintf(CRITICAL, "Failed Writing block @ %x\n", (data_addr / MMC_BLK_SZ));

	return val;
}

/*
 * Function: mmc_read
 * Arg     : Data address on card, o/p buffer & data length
 * Return  : 0 on Success, non zero on failure
 * Flow    : Read data from the card to out
 */
uint32_t mmc_read(uint64_t data_addr, uint32_t *out, uint32_t data_len)
{
	uint32_t ret = 0;
	uint32_t read_size = SDHCI_ADMA_MAX_TRANS_SZ;
	struct mmc_device *dev;
	uint8_t *sptr = (uint8_t *)out;

	ASSERT(!(data_addr % MMC_BLK_SZ));
	ASSERT(!(data_len % MMC_BLK_SZ));

	dev = target_mmc_device();

	/* TODO: This function is aware of max data that can be
	 * tranferred using sdhci adma mode, need to have a cleaner
	 * implementation to keep this function independent of sdhci
	 * limitations
	 */
	while (data_len > read_size) {
		ret = mmc_sdhci_read(dev, (void *)sptr, (data_addr / MMC_BLK_SZ), (read_size / MMC_BLK_SZ));
		if (ret)
		{
			dprintf(CRITICAL, "Failed Reading block @ %x\n", (data_addr / MMC_BLK_SZ));
			return ret;
		}
		sptr += read_size;
		data_addr += read_size;
		data_len -= read_size;
	}

	if (data_len)
		ret = mmc_sdhci_read(dev, (void *)sptr, (data_addr / MMC_BLK_SZ), (data_len / MMC_BLK_SZ));

	if (ret)
		dprintf(CRITICAL, "Failed Reading block @ %x\n", (data_addr / MMC_BLK_SZ));

	return ret;
}

/*
 * Function: mmc get erase unit size
 * Arg     : None
 * Return  : Returns the erase unit size of the storage
 * Flow    : Get the erase unit size from the card
 */

uint32_t mmc_get_eraseunit_size()
{
	uint32_t erase_unit_sz = 0;
	struct mmc_device *dev;
	struct mmc_card *card;

	dev = target_mmc_device();
	card = &dev->card;
	/*
	 * Calculate the erase unit size,
	 * 1. Based on emmc 4.5 spec for emmc card
	 * 2. Use SD Card Status info for SD cards
	 */
	if (MMC_CARD_MMC(card))
	{
		/*
		 * Calculate the erase unit size as per the emmc specification v4.5
		 */
		if (dev->card.ext_csd[MMC_ERASE_GRP_DEF])
			erase_unit_sz = (MMC_HC_ERASE_MULT * dev->card.ext_csd[MMC_HC_ERASE_GRP_SIZE]) / MMC_BLK_SZ;
		else
			erase_unit_sz = (dev->card.csd.erase_grp_size + 1) * (dev->card.csd.erase_grp_mult + 1);
	}
	else
		erase_unit_sz = dev->card.ssr.au_size * dev->card.ssr.num_aus;

	return erase_unit_sz;
}

/*
 * Function: Zero out blk_len blocks at the blk_addr by writing zeros. The
 *           function can be used when we want to erase the blocks not
 *           aligned with the mmc erase group.
 * Arg     : Block address & length
 * Return  : Returns 0
 * Flow    : Erase the card from specified addr
 */

static uint32_t mmc_zero_out(struct mmc_device* dev, uint32_t blk_addr, uint32_t num_blks)
{
	uint32_t *out;
	uint32_t block_size;
	int i;

	dprintf(INFO, "erasing 0x%x:0x%x\n", blk_addr, num_blks);
	block_size = mmc_get_device_blocksize();

	/* Assume there are at least block_size bytes available in the heap */
	out = memalign(CACHE_LINE, ROUNDUP(block_size, CACHE_LINE));

	if (!out)
	{
		dprintf(CRITICAL, "Error allocating memory\n");
		return 1;
	}
	memset((void *)out, 0, ROUNDUP(block_size, CACHE_LINE));

	for (i = 0; i < num_blks; i++)
	{
		if (mmc_sdhci_write(dev, out, blk_addr + i, 1))
		{
			dprintf(CRITICAL, "failed to erase the partition: %x\n", blk_addr);
			free(out);
			return 1;
		}
	}
	free(out);
	return 0;
}

/*
 * Function: mmc erase card
 * Arg     : Block address & length
 * Return  : Returns 0
 * Flow    : Erase the card from specified addr
 */
uint32_t mmc_erase_card(uint64_t addr, uint64_t len)
{
	struct mmc_device *dev;
	uint32_t block_size;
	uint32_t unaligned_blks;
	uint32_t head_unit;
	uint32_t tail_unit;
	uint32_t erase_unit_sz;
	uint32_t blk_addr;
	uint32_t blk_count;
	uint64_t blks_to_erase;

	block_size = mmc_get_device_blocksize();
	erase_unit_sz = mmc_get_eraseunit_size();
	dprintf(SPEW, "erase_unit_sz:0x%x\n", erase_unit_sz);

	ASSERT(!(addr % MMC_BLK_SZ));
	ASSERT(!(len % MMC_BLK_SZ));

	blk_addr = addr / block_size;
	blk_count = len / block_size;

	dev = target_mmc_device();

	dprintf(INFO, "Erasing card: 0x%x:0x%x\n", blk_addr, blk_count);

	head_unit = blk_addr / erase_unit_sz;
	tail_unit = (blk_addr + blk_count - 1) / erase_unit_sz;

	if (tail_unit - head_unit <= 1)
	{
		dprintf(INFO, "SDHCI unit erase not required\n");
		return mmc_zero_out(dev, blk_addr, blk_count);
	}

	unaligned_blks = erase_unit_sz - (blk_addr % erase_unit_sz);

	if (unaligned_blks < erase_unit_sz)
	{
		dprintf(SPEW, "Handling unaligned head blocks\n");
		if (mmc_zero_out(dev, blk_addr, unaligned_blks))
			return 1;

		blk_addr += unaligned_blks;
		blk_count -= unaligned_blks;
	}

	unaligned_blks = blk_count % erase_unit_sz;
	blks_to_erase = blk_count - unaligned_blks;

	dprintf(SPEW, "Performing SDHCI erase: 0x%x:0x%x\n", blk_addr, blks_to_erase);
	if (mmc_sdhci_erase((struct mmc_device *)dev, blk_addr, blks_to_erase * block_size))
	{
		dprintf(CRITICAL, "MMC erase failed\n");
		return 1;
	}

	blk_addr += blks_to_erase;

	if (unaligned_blks)
	{
		dprintf(SPEW, "Handling unaligned tail blocks\n");
		if (mmc_zero_out(dev, blk_addr, unaligned_blks))
			return 1;
	}

	return 0;
}

/*
 * Function: mmc get psn
 * Arg     : None
 * Return  : Returns the product serial number
 * Flow    : Get the PSN from card
 */
uint32_t mmc_get_psn(void)
{
	struct mmc_card *card;

	card = get_mmc_card();

	return card->cid.psn;
}

/*
 * Function: mmc get capacity
 * Arg     : None
 * Return  : Returns the density of the emmc card
 * Flow    : Get the density from card
 */
uint64_t mmc_get_device_capacity()
{
	struct mmc_card *card;

	card = get_mmc_card();

	return card->capacity;
}

/*
 * Function: mmc get pagesize
 * Arg     : None
 * Return  : Returns the density of the emmc card
 * Flow    : Get the density from card
 */
uint32_t mmc_get_device_blocksize()
{
	struct mmc_card *card;

	card = get_mmc_card();

	return card->block_size;
}

/*
 * Function: storage page size
 * Arg     : None
 * Return  : Returns the page size for the card
 * Flow    : Get the page size for storage
 */
uint32_t mmc_page_size()
{
	return BOARD_KERNEL_PAGESIZE;
}
