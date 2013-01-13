LOCAL_DIR := $(GET_LOCAL_DIR)

#MODULE := $(LOCAL_DIR)

INCLUDES += \
	-I$(LOCAL_DIR)/include

PLATFORM := bcm2835

MODULES += \

DEFINES += \

#include make/module.mk

