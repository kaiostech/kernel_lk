LOCAL_DIR := $(GET_LOCAL_DIR)

INCLUDES += -I$(LK_TOP_DIR)/platform/msm_shared/include

OBJS += \
	$(LOCAL_DIR)/aboot.o \
	$(LOCAL_DIR)/fastboot.o \
	$(LOCAL_DIR)/recovery.o
# ACOS_MOD_BEGIN
OBJS += \
	$(LOCAL_DIR)/cmd_idme.o \
	$(LOCAL_DIR)/cmd_idme_v2_0.o
# ACOS_MOD_END
