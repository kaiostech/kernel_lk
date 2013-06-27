/*
 * cmd_idme_v2_0.c
 */

#include <string.h>
#include <idme.h>
#include <debug.h>
#include <libfdt.h>

#ifdef WITH_ENABLE_IDME

#define MIN(x,y) ((x) > (y)? (y):(x))

#define IDME_ITEM_INIT(pitem, limit, item, max_size, export, item_permission, item_value) \
	if((unsigned long)((char *)pitem + sizeof(struct idme_desc) + max_size) > (unsigned long)limit)\
	{\
		dprintf(CRITICAL, "When init item: %s.\n", item);\
		dprintf(CRITICAL, "The idme size is out of limit(0x%x).\n", CONFIG_IDME_SIZE);\
	}else{\
		memset(&(pitem->data[0]), 0, max_size); \
		memcpy(&(pitem->data[0]), item_value, MIN(max_size - 1, strlen(item_value))); \
		memset(pitem->desc.name, 0, IDME_MAX_NAME_LEN); \
		memcpy(pitem->desc.name, item, MIN(IDME_MAX_NAME_LEN - 1, strlen(item)));\
		pitem->desc.size = max_size;\
		pitem->desc.exportable = export;\
		pitem->desc.permission = item_permission;\
	}

#define IDME_ITEM_NEXT(curr_item) \
	curr_item = (struct item_t *)((char *)curr_item + sizeof(struct idme_desc) + curr_item->desc.size)

const struct idme_init_values idme_default_values[] = {
	{ { "board_id", 16, 1, 0444 },
		/* Default Board ID value */
		"ffffff0000000000"
	},
	{ { "serial", 16, 1, 0444 },
		/* Default DSN value */
		"0"
	},
	{ { "mac_addr", 16, 1, 0444 },
		/* Default MAC address */
		"0"
	},
	{ { "mac_sec", 32, 1, 0444 },
		/* Default MAC secret */
		"0"
	},
	{ { "bt_mac_addr", 16, 1, 0444 },
		/* Default BT MAC address */
		"0"
	},
	{ { "productid", 32, 1, 0444 },
		/* Default Primary Product ID */
		"0"
	},
	{ { "productid2", 32, 1, 0444 },
		/* Default Secondary Product ID */
		"0"
	},
	{ { "bootmode", 4, 1, 0444 },
		/* Default Bootmode */
#if defined(CONFIG_MACH_MSM8974)
		"1" /* Devices that do not go through factory should be 1 */
#else
		"2" /* Anything that begins in a factory should be 2 */
#endif
	},
	{ { "postmode", 4, 1, 0444 },
		/* Default Postmode */
		"0"
	},
	{ { "bootcount", 8, 1, 0444 },
		/* Initial Bootcount */
		"0"
	},
	{ { "panelcal", 160, 1, 0444 },
		/* Panel Calibration Data */
		""
	},
	{ { "manufacturing", 512, 1, 0444 },
		/* Manufacturer-specific data */
		""
	},
	{ { "", 0, 0, 0 }, 0 },
};

int idme_init_var_v2p0(void *data)
{
	struct idme_t *pidme = (struct idme_t *)data;
	char *idme_limit = (char *)pidme + CONFIG_IDME_SIZE;
	struct item_t *pitem = (struct item_t *)(&(pidme->item_data[0]));
	unsigned int items_num = 0;

	memset(data, 0, CONFIG_IDME_SIZE);
	memcpy(pidme->magic, IDME_MAGIC_NUMBER, strlen(IDME_MAGIC_NUMBER));
	memcpy(pidme->version, IDME_VERSION_2P1, strlen(IDME_VERSION_2P1));

	/* use default values to initialize idme data */
	const struct idme_init_values *ptr = &idme_default_values[0];

	while (strlen(ptr->desc.name)) {
		IDME_ITEM_INIT(pitem, idme_limit,
			ptr->desc.name,
			ptr->desc.size,
			ptr->desc.exportable,
			ptr->desc.permission,
			ptr->value);

		items_num++;
		ptr++;

		if (strlen(ptr->desc.name))
			IDME_ITEM_NEXT(pitem);
	}

	pidme->items_num = items_num;

	return 0;
}

int idme_get_var_v2p0(const char *name, char *buf, unsigned int length, void *data)
{
	int ret = -1;
	unsigned int i = 0;
	struct idme_t *pdata = (struct idme_t *)data;
	struct item_t *pitem = NULL;

	if (0 != idme_check_magic_number(pdata)){
		dprintf(CRITICAL, "The idme magic number error.\n");
		return -1;
	}

	if (NULL == buf)
		return -1;

	pitem = (struct item_t *)(&(pdata->item_data[0]));
	for (i = 0; i < pdata->items_num; i++) {
		if ( 0 == strcmp(name, pitem->desc.name) ) {
			memcpy( buf, &(pitem->data[0]), MIN( pitem->desc.size, length ) );
			ret = 0;
			break;
		}else{
			IDME_ITEM_NEXT(pitem);
		}
	}



	return ret;
}

int idme_update_var_v2p0(const char *name, const char *value, unsigned int length, void *data)
{
	int ret = -1;
	unsigned int i = 0;
	struct idme_t *pdata = (struct idme_t *)data;
	struct item_t *pitem = NULL;

	if (0 != idme_check_magic_number(pdata)){
		dprintf(CRITICAL, "The idme magic number error.\n");
		return -1;
	}

	if (value == NULL){
		dprintf(INFO, "warning: the idme value is null.\n");
		return -1;
	}

	pitem = (struct item_t *)(&(pdata->item_data[0]));
	for (i = 0; i < pdata->items_num; i++) {
		if ( 0 == strcmp(name, pitem->desc.name) ) {
			memset(&(pitem->data[0]), 0, pitem->desc.size);
			memcpy(&(pitem->data[0]), value, MIN( pitem->desc.size, length ) );
			ret = 0;
			break;
		}else{
			IDME_ITEM_NEXT(pitem);
		}
	}

	return ret;
}

struct idme_desc *idme_get_item_desc_v2p0(void *data, char *item_name)
{
	unsigned int i = 0;
	struct idme_t *pdata = (struct idme_t *)data;
	struct item_t *pitem = NULL;

	if (0 != idme_check_magic_number(pdata)){
		dprintf(CRITICAL, "The idme magic number error.\n");
		return NULL;
	}

	pitem = (struct item_t *)(&(pdata->item_data[0]));
	for (i = 0; i < pdata->items_num; i++) {
		if ( 0 == strcmp(item_name, pitem->desc.name) ) {
			return &(pitem->desc);
		}else{
			IDME_ITEM_NEXT(pitem);
		}
	}

	return NULL;
}

static char *idme_permission_to_string(int permission)
{
	unsigned int i = 0;
	static char buffer[10];

	memset(buffer, 0, sizeof(buffer));
	memset(buffer, '-', sizeof(buffer) - 1);

	for (i = 0; i < sizeof(buffer) - 1; i++) {
		if (permission & (1 << (8 - i))) {
			switch (i) {
			case 0:
			case 3:
			case 6:
				buffer[i] = 'r';
				break;
			case 1:
			case 4:
			case 7:
				buffer[i] = 'w';
				break;
			case 2:
			case 5:
			case 8:
				buffer[i] = 'x';
				break;
			}
		}
	}

	return buffer;
}

int idme_print_var_v2p0(void *data)
{
	unsigned int i;
	char temp[IDME_MAX_PRINT_SIZE + 1];
	struct idme_t *pdata = (struct idme_t *)data;
	struct item_t *pitem = NULL;

	if (0 != idme_check_magic_number(pdata)){
		dprintf(CRITICAL, "The idme magic number error.\n");
		return -1;
	}

	dprintf(INFO, "idme items number:%d\n", pdata->items_num);
	pitem = (struct item_t *)(&(pdata->item_data[0]));

	for(i = 0; i < pdata->items_num;i++){
		memset(temp, 0, sizeof(temp));
		memcpy(temp, &(pitem->data[0]), MIN(pitem->desc.size, IDME_MAX_PRINT_SIZE));
		dprintf(INFO, "name: %s, size: %d, exportable: %d, "
				"permission: %s, data: [%s]\n",
			pitem->desc.name,
			pitem->desc.size,
			pitem->desc.exportable,
			idme_permission_to_string(pitem->desc.permission),
			temp);
		IDME_ITEM_NEXT(pitem);
	}

	return 0;
}

int idme_init_dev_tree_v2p1(void *fdt, void *data)
{
	int ret = 0, offset = 0, node_offset = 0;
	char buffer[IDME_MAX_NAME_LEN + 6 + 1];
	char temp[IDME_MAX_PRINT_SIZE + 1];
	struct idme_t *pidme = (struct idme_t *)data;
	struct item_t *pitem = (struct item_t *)&(pidme->item_data[0]);
	int count = pidme->items_num;

	ret = fdt_path_offset(fdt, "/idme");

	if (ret & FDT_ERR_NOTFOUND) {
		/* Create the IDME root node in FDT */
		if ((ret = fdt_path_offset(fdt, "/")) < 0) {
			dprintf(CRITICAL, "Unable to find root offset\n");
			goto done;
		}

		if ((ret = fdt_add_subnode(fdt, ret, "idme")) < 0) {
			dprintf(CRITICAL, "Unable to add IDME root node\n");
			goto done;
		}

		/* Get the offset of the new IDME root node */
		if ((ret = fdt_path_offset(fdt, "/idme")) < 0) {
			dprintf(CRITICAL,
				"Unable to find IDME root node offset\n");
			goto done;
		}

		offset = ret;

		while (count--) {
			if (!pitem->desc.exportable) {
				IDME_ITEM_NEXT(pitem);
				continue;
			}

			/* Copy the value string */
			memcpy(temp, &(pitem->data[0]),
				MIN(pitem->desc.size, IDME_MAX_PRINT_SIZE));

			/* Create the sub-node, and set the value */
			if ((ret = fdt_add_subnode(fdt,
					offset, pitem->desc.name)) < 0) {
				dprintf(CRITICAL,
					"Unable to add new IDME node: %s\n",
					pitem->desc.name);
				goto done;
			}

			sprintf(buffer, "/idme/%s", pitem->desc.name);

			if ((ret = fdt_path_offset(fdt, buffer)) < 0) {
				dprintf(CRITICAL,
					"Unable to locate find new IDME node offset: %s\n",
					pitem->desc.name);
				goto done;
			}

			node_offset = ret;

			if ((ret = fdt_setprop_string(fdt,
					node_offset, "value", temp)) < 0) {
				dprintf(CRITICAL,
					"Unable to set IDME node (value): %s\n",
					pitem->desc.name);
				goto done;
			}

			if ((ret = fdt_setprop_u32(fdt,
					node_offset, "permission",
					pitem->desc.permission)) < 0)  {
				dprintf(CRITICAL,
					"Unable to set IDME node (permission): %s\n",
					pitem->desc.name);
				goto done;
			}

			IDME_ITEM_NEXT(pitem);
		}
		ret = 0;
	} else if (ret != 0) {
		dprintf(CRITICAL, "Error in searching IDME node in FDT\n");
	}
done:
	return ret;
}

int idme_init_atag_v2p0(void *atag_buffer, void *data, unsigned int size)
{
	struct idme_t *pidme = (struct idme_t *)data;
	struct idme_t *patag_idme = (struct idme_t *)atag_buffer;

	struct item_t *pitem = (struct item_t *)&(pidme->item_data[0]);
	int count = pidme->items_num;
	unsigned int item_size = 0;
	char *atag_limit = atag_buffer + size;

	if (0 != idme_check_magic_number(pidme)){
		dprintf(CRITICAL, "The idme magic number error.\n");
		return -1;
	}
	//init atag idme.
	memset(atag_buffer, 0, size);
	memcpy(patag_idme->magic, IDME_MAGIC_NUMBER, strlen(IDME_MAGIC_NUMBER));
	memcpy(patag_idme->version, IDME_VERSION_2P0, strlen(IDME_VERSION_2P0));
	patag_idme->items_num = 0;

	atag_buffer = (struct item_t *)&(patag_idme->item_data[0]);
	/* fill proc items */
	while(count--)
	{
		item_size = sizeof(struct idme_desc) + pitem->desc.size;
		if( (unsigned long)(atag_buffer + item_size) > (unsigned long)atag_limit ){
			dprintf(CRITICAL, "When init atag of idme item: %s.\n", pitem->desc.name);
			dprintf(CRITICAL, "The atag idme size is out of limit(0x%x).\n", size);
			return -1;
		}
		if(pitem->desc.exportable){
			memcpy(atag_buffer, (char *)pitem, item_size);
			patag_idme->items_num++;
			dprintf(INFO, "name: %s, size: %d, exportable: %d, permission: %d, data: %s\n", pitem->desc.name, pitem->desc.size, pitem->desc.exportable, pitem->desc.permission, &pitem->data[0]);
		}
		atag_buffer += item_size;
		IDME_ITEM_NEXT(pitem);
	}
	dprintf(INFO, "The atag idme items number:%d\n", patag_idme->items_num);
	return 0;
}

#endif
