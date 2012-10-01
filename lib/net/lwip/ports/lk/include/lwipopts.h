#ifndef LK_LWIPOPTS_H
#define LK_LWIPOPTS_H
/*
 * LK Specific LWIP Options (overrides src/include/lwip/opt.h)
 */

/* Turn on support for IPv6 */
#define LWIP_IPV6 1

/* Enable built-in mappings for standard UNIX ERRNOs */
#define LWIP_PROVIDE_ERRNO 1

#endif
