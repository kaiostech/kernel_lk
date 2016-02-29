LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/moot.c \
	$(LOCAL_DIR)/cmd.c

include make/module.mk

