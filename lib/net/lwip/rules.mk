#
# Whack together the files for lwip into an lk-style module makefile.
#
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/src/core/def.c \
	$(LOCAL_DIR)/src/core/dhcp.c \
	$(LOCAL_DIR)/src/core/dns.c \
	$(LOCAL_DIR)/src/core/init.c \
	$(LOCAL_DIR)/src/core/mem.c \
	$(LOCAL_DIR)/src/core/memp.c \
	$(LOCAL_DIR)/src/core/netif.c \
	$(LOCAL_DIR)/src/core/pbuf.c \
	$(LOCAL_DIR)/src/core/raw.c \
	$(LOCAL_DIR)/src/core/stats.c \
	$(LOCAL_DIR)/src/core/sys.c \
	$(LOCAL_DIR)/src/core/tcp.c \
	$(LOCAL_DIR)/src/core/tcp_in.c \
	$(LOCAL_DIR)/src/core/tcp_out.c \
	$(LOCAL_DIR)/src/core/timers.c \
	$(LOCAL_DIR)/src/core/udp.c \
	$(LOCAL_DIR)/src/core/ipv4/autoip.c \
	$(LOCAL_DIR)/src/core/ipv4/icmp.c \
	$(LOCAL_DIR)/src/core/ipv4/igmp.c \
	$(LOCAL_DIR)/src/core/ipv4/inet.c \
	$(LOCAL_DIR)/src/core/ipv4/inet_chksum.c \
	$(LOCAL_DIR)/src/core/ipv4/ip.c \
	$(LOCAL_DIR)/src/core/ipv4/ip_addr.c \
	$(LOCAL_DIR)/src/core/ipv4/ip_frag.c \
	$(LOCAL_DIR)/src/api/api_lib.c \
	$(LOCAL_DIR)/src/api/api_msg.c \
	$(LOCAL_DIR)/src/api/err.c \
	$(LOCAL_DIR)/src/api/netbuf.c \
	$(LOCAL_DIR)/src/api/netdb.c \
	$(LOCAL_DIR)/src/api/netifapi.c \
	$(LOCAL_DIR)/src/api/sockets.c \
	$(LOCAL_DIR)/src/api/tcpip.c \
	$(LOCAL_DIR)/src/netif/etharp.c \
	$(LOCAL_DIR)/src/netif/ethernetif.c


INCLUDES += \
	-I$(LOCAL_DIR)/src/include \
	-I$(LOCAL_DIR)/src/include/ipv4 \
	-I$(LOCAL_DIR)/ports/lk/include \

include make/module.mk
