/* Copyright (c) 2015, The Linux Foundation. All rights reserved.
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

#include <debug.h>
#include <reg.h>
#include <platform/iomap.h>
#include <platform/irqs.h>
#include <platform/clock.h>
#include <qgic.h>
#include <qtimer.h>
#include <arch/arm/mmu.h>
#include <mmu.h>
#include <smem.h>
#include <board.h>
#include <boot_stats.h>
#include <platform.h>
#include <target/display.h>

#define MSM8976_SOC_V11 0x10001
#define MSM_IOMAP_SIZE ((MSM_IOMAP_END - MSM_IOMAP_BASE)/MB)
#define APPS_SS_SIZE   ((APPS_SS_END - APPS_SS_BASE)/MB)
/* LK memory - cacheable, write through */
#define LK_MEMORY         (MMU_MEMORY_TYPE_NORMAL_WRITE_BACK_ALLOCATE | \
				MMU_MEMORY_AP_READ_WRITE)

/* Peripherals - non-shared device */
#define IOMAP_MEMORY      (MMU_MEMORY_TYPE_DEVICE_SHARED | \
				MMU_MEMORY_AP_READ_WRITE | MMU_MEMORY_XN)

/* IMEM memory - cacheable, write through */
#define COMMON_MEMORY     (MMU_MEMORY_TYPE_NORMAL_WRITE_THROUGH | \
				MMU_MEMORY_AP_READ_WRITE | MMU_MEMORY_XN)

#define SCRATCH_MEMORY    (MMU_MEMORY_TYPE_NORMAL_WRITE_BACK_ALLOCATE | \
				MMU_MEMORY_AP_READ_WRITE | MMU_MEMORY_XN)

#if LPAE
static mmu_section_t mmu_section_table[] =
{
/*       Physical addr,          Virtual addr,          Mapping type ,              Size (in MB),            Flags */
    {    MEMBASE,                MEMBASE,               MMU_L2_NS_SECTION_MAPPING,  (MEMSIZE / MB),      LK_MEMORY},
    {    MSM_IOMAP_BASE,         MSM_IOMAP_BASE,        MMU_L2_NS_SECTION_MAPPING,  MSM_IOMAP_SIZE,      IOMAP_MEMORY},
	{    APPS_SS_BASE,           APPS_SS_BASE,          MMU_L2_NS_SECTION_MAPPING,  APPS_SS_SIZE,        IOMAP_MEMORY},
    {    MSM_SHARED_IMEM_BASE,   MSM_SHARED_IMEM_BASE,  MMU_L2_NS_SECTION_MAPPING,  2,                   COMMON_MEMORY},
    {    SCRATCH_ADDR,           SCRATCH_ADDR,          MMU_L2_NS_SECTION_MAPPING,  512,                 SCRATCH_MEMORY},
	{    MIPI_FB_ADDR,           MIPI_FB_ADDR,          MMU_L2_NS_SECTION_MAPPING,  42,                  COMMON_MEMORY},
    {    RPMB_SND_RCV_BUF,       RPMB_SND_RCV_BUF,      MMU_L2_NS_SECTION_MAPPING,  RPMB_SND_RCV_BUF_SZ, IOMAP_MEMORY},
};
#else
static mmu_section_t mmu_section_table[] =
{
/*           Physical addr,         Virtual addr,            Size (in MB),     Flags */
	{    MEMBASE,               MEMBASE,                 (MEMSIZE / MB),         LK_MEMORY},
	{    MSM_IOMAP_BASE,        MSM_IOMAP_BASE,          MSM_IOMAP_SIZE,         IOMAP_MEMORY},
	{    APPS_SS_BASE,          APPS_SS_BASE,            APPS_SS_SIZE,           IOMAP_MEMORY},
	{    MSM_SHARED_IMEM_BASE,  MSM_SHARED_IMEM_BASE,    1,                      COMMON_MEMORY},
	{    SCRATCH_ADDR,          SCRATCH_ADDR,            512,                    SCRATCH_MEMORY},
	{    MIPI_FB_ADDR,          MIPI_FB_ADDR,            42,                     COMMON_MEMORY},
	{    RPMB_SND_RCV_BUF,      RPMB_SND_RCV_BUF,        RPMB_SND_RCV_BUF_SZ,    IOMAP_MEMORY},
};
#endif

void platform_early_init(void)
{
	board_init();
	platform_clock_init();
	qgic_init();
	qtimer_init();
	scm_init();
}

void platform_init(void)
{
	dprintf(INFO, "platform_init()\n");
}

void platform_uninit(void)
{
	qtimer_uninit();
}

uint32_t platform_get_sclk_count(void)
{
	return readl(MPM2_MPM_SLEEP_TIMETICK_COUNT_VAL);
}

addr_t get_bs_info_addr()
{
	return ((addr_t)BS_INFO_ADDR);
}

int platform_use_identity_mmu_mappings(void)
{
	/* Use only the mappings specified in this file. */
	return 0;
}

#if LPAE
/* Setup memory for this platform */
void platform_init_mmu_mappings(void)
{
	int i;
	int table_sz = ARRAY_SIZE(mmu_section_table);
	uint32_t ddr_start = get_ddr_start();
	uint32_t smem_addr = platform_get_smem_base_addr();
	mmu_section_t entry;

	/*Mapping the ddr start address for loading the kernel about 90 MB*/
	entry.paddress = entry.vaddress = ddr_start;
	entry.type = MMU_L2_NS_SECTION_MAPPING;
	entry.size = 90;
	entry.flags = SCRATCH_MEMORY;
	arm_mmu_map_entry(&entry);

	entry.paddress = entry.vaddress = smem_addr;
	entry.size = 2;
	entry.flags = COMMON_MEMORY;
	arm_mmu_map_entry(&entry);

	/* Map default memory needed for lk , scratch, rpmb & iomap */
	for (i = 0 ; i < table_sz; i++)
		arm_mmu_map_entry(&mmu_section_table[i]);

}
#else
/* Setup MMU mapping for this platform */
void platform_init_mmu_mappings(void)
{
	uint32_t i;
	uint32_t sections;
	uint32_t table_size = ARRAY_SIZE(mmu_section_table);
	uint32_t ddr_start = get_ddr_start();
	uint32_t smem_addr = platform_get_smem_base_addr();

	/*Mapping the ddr start address for loading the kernel about 90 MB*/
	sections = 90;
	while(sections--)
	{
		arm_mmu_map_section(ddr_start + sections * MB, ddr_start + sections* MB, COMMON_MEMORY);
	}


	/* Mapping the SMEM addr */
	arm_mmu_map_section(smem_addr, smem_addr, COMMON_MEMORY);

	/* Configure the MMU page entries for memory read from the
	   mmu_section_table */
	for (i = 0; i < table_size; i++)
	{
		sections = mmu_section_table[i].num_of_sections;

		while (sections--)
		{
			arm_mmu_map_section(mmu_section_table[i].paddress +
								sections * MB,
								mmu_section_table[i].vaddress +
								sections * MB,
								mmu_section_table[i].flags);
		}
	}
}
#endif
addr_t platform_get_virt_to_phys_mapping(addr_t virt_addr)
{
#if LPAE
	return virtual_to_physical_mapping(virt_addr);
#else
	/* Using 1-1 mapping on this platform. */
	return virt_addr;
#endif
}

addr_t platform_get_phys_to_virt_mapping(addr_t phys_addr)
{
#if LPAE
	return physical_to_virtual_mapping(phys_addr);
#else
	/* Using 1-1 mapping on this platform. */
	return phys_addr;
#endif
}

/* DYNAMIC SMEM REGION feature enables LK to dynamically
 * read the SMEM addr info from TCSR_TZ_WONCE register.
 * The first word read, if indicates a MAGIC number, then
 * Dynamic SMEM is assumed to be enabled. Read the remaining
 * SMEM info for SMEM Size and Phy_addr from the other bytes.
 */
uint32_t platform_get_smem_base_addr()
{
	struct smem_addr_info *smem_info = NULL;

	smem_info = (struct smem_addr_info *)readl(TCSR_TZ_WONCE);
	if(smem_info && (smem_info->identifier == SMEM_TARGET_INFO_IDENTIFIER))
		return smem_info->phy_addr;
	else
		return MSM_SHARED_BASE;
}

uint32_t platform_get_max_periph()
{
	return 256;
}

int platform_is_msm8956()
{
	uint32_t platform = board_platform_id();
	uint32_t ret = 0;

	switch(platform)
	{
		case MSM8956:
		case APQ8056:
		case APQ8076:
		case MSM8976:
			ret = 1;
			break;
		default:
			ret = 0;
	};

	return ret;
}

uint32_t platform_read_pte_reg()
{
       uint32_t reg = 0;

	if(platform_is_msm8956()) {
		reg = readl(QFPROM_PTE_PART_ADDR);
		/*
		 * The PTE register bits 28 to 31 have the partial goods
		 * info, extract the partial goods value before using
		 */
		reg = (reg & 0xf0000000) >> 28;
		dprintf(CRITICAL, " platform_read_pte_reg %d\n", reg);
	}

	return reg;
}

uint32_t platform_check_pte_reg(uint32_t index, uint32_t reg)
{
	uint32_t ret = 0;

	if((reg >> index) & 1)
		ret = 1;
	return ret;
}

uint32_t platform_is_msm8976_v_1_1()
{
	uint32_t soc_ver = board_soc_version();
	uint32_t ret = 0;

	if(soc_ver == MSM8976_SOC_V11)
		ret = 1;
	else
		ret = 0;

	return ret;
}
