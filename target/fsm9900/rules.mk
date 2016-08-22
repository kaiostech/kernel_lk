LOCAL_DIR := $(GET_LOCAL_DIR)

INCLUDES += -I$(LOCAL_DIR)/include -I$(LK_TOP_DIR)/platform/msm_shared

PLATFORM := fsm9900

MEMBASE := 0x08a00000 # SDRAM
MEMSIZE := 0x00100000 # 1MB

BASE_ADDR        := 0x08000000

SCRATCH_ADDR     := 0x08b00000

MODULES += \
	dev/keys \
	dev/pmic/pm8x41 \
    lib/ptable \
    lib/libfdt

DEFINES += \
	MEMSIZE=$(MEMSIZE) \
	MEMBASE=$(MEMBASE) \
	SCRATCH_ADDR=$(SCRATCH_ADDR)


OBJS += \
    $(LOCAL_DIR)/init.o \
    $(LOCAL_DIR)/meminfo.o
