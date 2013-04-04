/*
 * cmd_idme.c
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

#include "idme.h"
#include <stdlib.h>
#include <string.h>
#include <debug.h>
#include "fastboot.h"
#include <mmc.h>

#ifdef WITH_ENABLE_IDME

/* Local variables */
static unsigned char gidme_buff[CONFIG_IDME_SIZE];
static struct idme_t *pidme_data = NULL;

/* Local function prototype */
void idme_dump_memory(unsigned char *dst, int size);
static int idme_null_func(void);
int do_idme ( int flag, int argc, char *argv[]);
static int (*idme_init_var)(void *data);
static int (*idme_get_var)(const char *name, char *buf, unsigned int length, void *data);
static int (*idme_update_var)(const char *name, const char *value, unsigned int length, void *data);
static int (*idme_print_var)(void *data);
static int (*idme_init_atag)(void *atag_buffer, void *data, unsigned int size);
static int (*idme_init_dev_tree)(void *fdt, void *data);
static struct idme_desc* (*idme_get_item_desc)(void *data, char *item_name);
static int idme_read(unsigned char *pbuf);
static int idme_write(const unsigned char *pbuf);

/* List all the versions of idme in turn */
static struct idme_ver_t idme_ver_table[IDME_VER_TAB_LEN] =
{
	{ "1.2", IDME_VER_1P2},
	{ "2.0", IDME_VER_2P0},
	{ "2.1", IDME_VER_2P1},
	{ "3.0", IDME_VER_3P0},
};

/* read entire idme */
static int idme_read(unsigned char *pbuf)
{
	int ret = 0;
	int len = 0;
	unsigned long long read_addr = 0;

    /* switch to boot partition */
    if( switch_to_boot_partition() != 0) {
        dprintf(CRITICAL,
            "ERROR: couldn't switch to boot partition\n");
        return -1;
    }

	while (len < CONFIG_IDME_SIZE){
		ret = mmc_read( read_addr, (unsigned int*)(pbuf + len),
			CONFIG_MMC_BLOCK_SIZE );
		if (ret != 0) {
			dprintf( CRITICAL,
				"%s error in iteration %d! Couldn't read vars from boot partition\n",
				__FUNCTION__, len / CONFIG_MMC_BLOCK_SIZE);
			break;
        }
		len += CONFIG_MMC_BLOCK_SIZE;
		read_addr += CONFIG_MMC_BLOCK_SIZE;
	}

	// Switch out of boot partition.
	if( switch_to_user_partition() != 0 ) {
		dprintf(CRITICAL, "Error, failed to switch out of boot partition\n");
		return -1;
	}
	// if the mmc_read failed, return
	if (ret != 0) {
		return -1;
	}
	return 0;
}


static int idme_write( const unsigned char *pbuf)
{
	int len = 0;
	int ret = 0;
	int block_addr = 0;

	/* switch to boot partition */
	if( switch_to_boot_partition() != 0 ) {
		dprintf(CRITICAL, "Error, could not switch to boot partition\n");
		return -1;
	}

	while( len < CONFIG_IDME_SIZE ) {
		ret = mmc_write( block_addr, CONFIG_MMC_BLOCK_SIZE,
			(void *)(pbuf+len) );
                if( ret != 0 ){
                        dprintf(CRITICAL, "Error, mmc_write failed.\n");
                        break;
                }
                len += CONFIG_MMC_BLOCK_SIZE;
                block_addr += CONFIG_MMC_BLOCK_SIZE;
	}

	// Switch out of boot partition.
	if( switch_to_user_partition() != 0 ) {
		dprintf(CRITICAL, "Error, failed to switch out of boot partition\n");
		return -1;
	}

	if (ret != 0){
		return -1;
	}
	return 0;
}
int idme_clean(void)
{
	if (0 != idme_check_magic_number(pidme_data))
		return -1;

	memset(pidme_data, 0x00, CONFIG_IDME_SIZE);
	idme_write((const unsigned char *)pidme_data) ;

	return 0;
}

static int idme_null_func(void)
{
	/* Set all the functions to NULL */
	idme_init_var = NULL;
	idme_get_var = NULL;
	idme_update_var = NULL;
	idme_print_var = NULL;
	idme_init_atag = NULL;
	idme_get_item_desc = NULL;

	return 0;
}


int idme_initialize(void)
{
	int iTmp;
	unsigned int i = 0;

	if (pidme_data != NULL) {
		dprintf(INFO, "idme: pidme_data is not NULL, initialization already completed.\n");
		return 0;
	}

	memset(gidme_buff, 0x00, CONFIG_IDME_SIZE);

	/* load the idme data from boot area
	   Please make sure the MMC driver has been initialized before calling this function */
	if (idme_read(gidme_buff) != 0) {
		dprintf(CRITICAL, "Error, failed to read idme from boot area.\n");
		return -1;
	}

	pidme_data = (struct idme_t*)&gidme_buff[0];

	iTmp = idme_check_magic_number(pidme_data);

	if (-2 == iTmp) {
		dprintf(INFO, "WARNING: Failed to find IDME in boot area. Generating default idme ...\n");
		memcpy(pidme_data->magic, IDME_MAGIC_NUMBER, strlen(IDME_MAGIC_NUMBER));
		memcpy(pidme_data->version, IDME_DEFAULT_VERSION, strlen(IDME_DEFAULT_VERSION));
	}

	for ( ; i < sizeof(idme_ver_table)/sizeof(struct idme_ver_t); i++ ) {
		if (strncmp(pidme_data->version, idme_ver_table[i].version, strlen(idme_ver_table[i].version)) == 0) {
			if (idme_ver_table[i].ver_type == IDME_VER_2P0 ||
					idme_ver_table[i].ver_type == IDME_VER_2P1) {
				dprintf(INFO, "Idme version is 2.x and set related function to V2.x\n");
				idme_init_var = idme_init_var_v2p0;
				idme_get_var = idme_get_var_v2p0;
				idme_update_var = idme_update_var_v2p0;
				idme_print_var = idme_print_var_v2p0;
				idme_init_atag = idme_init_atag_v2p0;
				idme_get_item_desc = idme_get_item_desc_v2p0;
				idme_init_dev_tree = idme_init_dev_tree_v2p1;
			}
			else if (idme_ver_table[i].ver_type < IDME_VER_2P0) {
				dprintf(INFO, "Warning, version %s is an old version\n", pidme_data->version);
				idme_null_func();
			}
			else {
				//TODO
				dprintf(INFO, "Warning, we will implement a newer version %s here\n", pidme_data->version);
				idme_null_func();
			}
			break;
		}
	}

	/* Generating default idme ... */
	if ((-2 == iTmp) && (idme_init_var)) {
		idme_init_var((void*)pidme_data);
		idme_write((const unsigned char *)pidme_data);
	}

	return 0;
}

int idme_device_tree_initialize(void *fdt)
{
	if (idme_init_dev_tree) {
		return idme_init_dev_tree(fdt, pidme_data);
	} else {
		return 0;
	}
}

int idme_atag_initialize(void *atag_buffer, unsigned int size)
{
	if (NULL == atag_buffer)
		return -1;

	if (NULL == idme_init_atag)
		return -1;

	return idme_init_atag(atag_buffer, (void *)pidme_data, size);
}

int idme_check_magic_number(struct idme_t *pidme)
{
	if(pidme == NULL) {
		dprintf(CRITICAL, "Error, the pointer of pidme_data is NULL.\n");
		return -1;
	}

	if (strncmp(pidme->magic, IDME_MAGIC_NUMBER, strlen(IDME_MAGIC_NUMBER))){
		dprintf(CRITICAL, "Error, idme data is invalid!\n");
		return -2;
	}
	else
		return 0;
}

int idme_update_var_ex(const char *name, const char *value, unsigned int length)
{
	if (NULL == name)
		return -1;

	if (NULL == value) {
		return -1;
	}

	if (idme_update_var) {
		idme_update_var(name, value, length, pidme_data);
		idme_write((const unsigned char *)pidme_data);
		return 0;
	}
	else
		return -1;
}

void idme_boot_info(void)
{
	char boot_count_str[8];

	memset(boot_count_str, 0, sizeof(boot_count_str));

	/* get the bootcount */
	if ((idme_get_var) && (idme_get_var("bootcount", boot_count_str, sizeof(boot_count_str), pidme_data) == 0)) {
		dprintf(INFO, "bootcount = %d\n", atoi(boot_count_str) + 1);
		sprintf(boot_count_str, "%d", atoi(boot_count_str) + 1);
		idme_update_var_ex("bootcount", boot_count_str, IDME_MAX_IDME_ITEM_SIZE);
	}

}

int idme_boot_mode(void)
{
	char bootmode[4];

	if (idme_get_var)
		if (idme_get_var("bootmode", bootmode,
				sizeof(bootmode), pidme_data) == 0) {
			dprintf(INFO, "Boot mode is %d\n", atoi(bootmode));
			return atoi(bootmode);
		}

	return -1;
}

/* Please note: there is a tokenizing glitch here, in the case where 	*/
/* no arguments are passed in ("fastboot oem idme"). Argc is  		*/
/* incorrectly advanced.						*/
int fastboot_idme(const char *cmd)
{
	char *token;
	char ver[32], value[32];
	char *idme[3]  = {"idme", NULL, NULL};
	int i=2;
	idme[1]=ver;
	idme[2]=value;
	dprintf(INFO, "The token is:  \"%s\"\n", cmd);
	token = strtok((char *)cmd,(const char *)" ");
	dprintf(INFO, "Tokenized string is:  \"%s\"\n", token);
	sprintf(ver, "%s", (const char *)token);
	while (token != NULL){
		token = strtok(NULL," ");
		if(token != NULL){
			sprintf(value, "%s", token);
			i++;
		}else{
			break;
		}
	}
	return do_idme ( 0, i, idme);
}

/* Report on or modify IDME variables. cmdline is 'idme xyz abc' */
int do_idme ( int flag, int argc, char *argv[])
{
	for (int i = 0; i < argc; i++) {
		dprintf(INFO, "arg %d is %s\n", i, argv[i]);
	}

	if (argc == 1 || strncmp(argv[1], "?", 1) == 0) {
		if (argc == 3) {
			char buf[1024];
			int len = sprintf(buf, "%s: ", argv[2]);
			if (idme_get_var)
				idme_get_var(argv[2], buf+len, sizeof(buf)-len, pidme_data);
			fastboot_info(buf);
			return 0;
		} else
		if(idme_print_var)
			return idme_print_var(pidme_data);
		else
			return -1;
	}
	else if (argc <= 3) {
		char *value = NULL;
		int len = 0;
		int i = 0;
		int ret = -1;

		if (argc == 3) {
			if(strncmp(argv[2], "clean", strlen("clean")) == 0) {
				value = NULL;
				dprintf(INFO, "clean '%s'\n", argv[1]);
				len = 0;
			} else if(strncmp(argv[1], "version", strlen("version")) == 0) {
				for( ; i < IDME_VER_TAB_LEN; i++) {
					if(memcmp(idme_ver_table[i].version, argv[2],
							strlen(argv[2])) == 0) {
						value = argv[2];
						ret = 1;
						break;
					}
				}
				if(ret == -1) {
					dprintf(CRITICAL, "Error version, try again!\n");
					return ret;
				}
				memcpy(pidme_data->version, value, strlen(value));
				dprintf(INFO, "Modifying version to '%s'\n", argv[2]);
				idme_write((const unsigned char *)pidme_data);
				return 0;
			} else {
				value = argv[2];
				dprintf(INFO, "setting '%s' to '%s'\n", argv[1], argv[2]);
				len = strlen(value);
			}
		}
		else if (argc == 2) {
			if (strncmp(argv[1], "clean", strlen("clean")) == 0)
				return idme_clean();
			else {
				return 0;
			}
		}

		if (idme_update_var) {
			idme_update_var(argv[1], value, len, pidme_data);
			idme_write((const unsigned char *)pidme_data);
			return 0;
		}
		else
			return -1;
	}

	return 0;
}

unsigned int simple_atoi(const char *s, const char *e)
{
        unsigned int result = 0, value = 0;
        /* first remove all leading 0s */
        while(*s == '0' && s < e){
                s++;
        }

        while(s < e){
                value = *s - '0';
                result = result * 10 + value;
                s++;
        }
        return result;
}

int idme_read_first_block(unsigned char * buf)
{
	int ret = 0;
	if ( switch_to_boot_partition() != 0) {
		dprintf(CRITICAL, "ERROR: couldn't switch to boot partition\n");
		return -1;
	}

	ret = mmc_read( 0, (unsigned int*)buf, CONFIG_MMC_BLOCK_SIZE );
	if (ret != 0) {
		dprintf(CRITICAL,
			"%s error! Couldn't read vars from partition\n",
			__FUNCTION__);
	}

	/* ey: switching out of boot partition by clearing PARTITION_ACCESS bits */
	if( switch_to_user_partition() != 0) {
		dprintf(CRITICAL,
			"ERROR: couldn't switch back to user partition\n");
		return -1;
	}

	return ret;     /* may be -1 from the mmc_read */
}

/* idme_get_var_external
 *
 * Use this function from areas outside of idme who wish to access
 * idme fields without having access to the pidme_data struct
 */
int idme_get_var_external(const char *name, char *buf, unsigned int length)
{
	if(!idme_get_var)
		return -2; // Something different from idme_get_var
	return idme_get_var(name, buf, length, pidme_data);
}

/* NOT USED but interesting - */
void idme_dump_memory(unsigned char *dst, int size){
	int i;
	for( i = 0; i < size; i++){
		if(i%512 == 0){
			dprintf(INFO, "\n");
		}
		if( i%16 == 0){
			dprintf(INFO, "\n");
		}else if(i%4 == 0){
			dprintf(INFO, " ");
		}
		dprintf(INFO, "%2x ", dst[i]);
	}
	dprintf(INFO, "\n");
}

#if 0
/* The write_protect calls are completely untested. */
test_mmc_write_protect()
{
	unsigned int mmc_ret = MMC_BOOT_E_SUCCESS;
	unsigned int status;
	struct mmc_boot_card * card = get_mmc_card();

	mmc_ret = mmc_boot_switch_cmd(card, MMC_BOOT_ACCESS_WRITE, MMC_BOOT_EXT_BOOT_WP, (MMC_BOOT_B_PERM_WP_DIS | MMC_BOOT_B_PWR_WP_EN));
	// request permissions report, to test further

	return wait_for_ready();
}

test_mmc_no_write_protect()
{
	unsigned int mmc_ret = MMC_BOOT_E_SUCCESS;
	unsigned int status;
	struct mmc_boot_card * card = get_mmc_card();

	mmc_ret = mmc_boot_switch_cmd(card, MMC_BOOT_ACCESS_WRITE, MMC_BOOT_EXT_BOOT_WP, (MMC_BOOT_B_PERM_WP_DIS | MMC_BOOT_B_PWR_WP_DIS));
	// request permissions report, to test further

	return wait_for_ready();
}
#endif

#endif
