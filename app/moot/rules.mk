LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/moot.c \
	$(LOCAL_DIR)/usb.c \


include make/module.mk

