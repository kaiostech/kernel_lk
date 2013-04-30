/*
 * idme.h
 *
 * Copyright 2011 Amazon Technologies, Inc. All Rights Reserved.
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
 * @file idme.h
 * @brief This file contains functions for interacting with variables
 *in the userstore partition
 *
 */

#ifndef __IDME_H__
#define __IDME_H__

#ifdef WITH_ENABLE_IDME

int fastboot_idme(const char *cmd);

#define MAGIC_NUMBER_OFFSET 0
#define IDME_MAGIC_NUMBER "beefdeed"
#define IDME_BASE_ADDR 8        /* magic number and size takes 8 bytes */
#define MAX_IDME_VALUE 2560

/* base address of idme location on user partition */
#define CONFIG_USERDATA_IDME_ADDR 0x00
#define CONFIG_MMC_BLOCK_SIZE 512
#define IDME_NUM_OF_EMMC_BLOCKS 10
/* total size of 5120 bytes - 10 NAND native blocks*/
#define CONFIG_IDME_SIZE (CONFIG_MMC_BLOCK_SIZE * IDME_NUM_OF_EMMC_BLOCKS)

//idme size limit
#define IDME_MAX_NAME_LEN 16
#define IDME_MAX_IDME_ITEM_SIZE CONFIG_IDME_SIZE
#define IDME_MAX_PRINT_SIZE 40
#define IDME_ATAG_SIZE 2048

/*
Define idme atag address, which is same as OMAP platform.
Because the addresses for CORE, MEM and CMDLINE are the same
*/
#define ATAG_IDME 0x54410010

#define IDME_VER_TAB_LEN	4
#include <idme_v2_0.h>

#define IDME_DEFAULT_VERSION	IDME_VERSION_2P1

enum {
	IDME_BOOTMODE_NORMAL = 1,
	IDME_BOOTMODE_DIAG = 2,
	IDME_BOOTMODE_RECOVERY = 3,
	IDME_BOOTMODE_EMERGENCY = 4,
	IDME_BOOTMODE_RESERVED2 = 5,
	IDME_BOOTMODE_FASTBOOT = 6,
};

struct idme_desc {
	char name[IDME_MAX_NAME_LEN];
	unsigned int size;
	unsigned int exportable;
	unsigned int permission;
};

struct item_t {
	struct idme_desc desc;
	unsigned char data[0];
};

struct idme_t {
	char magic[8];
	char version[4];
	unsigned int items_num;

	unsigned char item_data[0];
};

struct idme_ver_t {
	char version[4];
	enum version_type {
		IDME_VER_1P2 = 0,
		IDME_VER_2P0,
		IDME_VER_2P1,
		IDME_VER_3P0
	} ver_type;
};

/* Defines the initial values along with the IDME tag */
struct idme_init_values {
	struct idme_desc desc;
	void *value;
};

int idme_initialize(void);
int idme_atag_initialize(void *atag_buffer, unsigned int size);
int idme_check_magic_number(struct idme_t *pidme);
int idme_update_var_ex(const char *name, const char *value, unsigned int length);
int idme_clean(void);
void idme_boot_info(void);
/* For external access to idme fields */
int idme_get_var_external(const char *name, char *buf, unsigned int length);

#endif // WITH_ENABLE_IDME

#endif /* __IDME_H__ */

