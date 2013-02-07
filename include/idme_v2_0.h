
#ifndef __IDME_V2_0_H__
#define __IDME_V2_0_H__

#ifdef WITH_ENABLE_IDME

#define IDME_VERSION_2P0 "2.0"
#define IDME_VERSION_2P1 "2.1"

int idme_init_var_v2p0(void *data);
int idme_get_var_v2p0(const char *name, char *buf, unsigned int length, void *data);
int idme_update_var_v2p0(const char *name, const char *value, unsigned int length, void *data);
int idme_print_var_v2p0(void *data);
int idme_init_atag_v2p0(void *atag_buffer, void *data, unsigned int size);
int idme_init_dev_tree_v2p1(void *fdt, void *data);
struct idme_desc *idme_get_item_desc_v2p0(void *data, char *item_name);
#endif // WITH_ENABLE_IDME

#endif
