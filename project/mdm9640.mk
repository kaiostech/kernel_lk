# top level project rules for the mdm9640 project
#
LOCAL_DIR := $(GET_LOCAL_DIR)

TARGET := mdm9640

ifeq ($(TARGET_BUILD_VARIANT),user)
DEBUG := 0
else
DEBUG := 1
endif

ifeq ($(TARGET_BOOTIMG_SIGNED),true)
  CFLAGS += -D_SIGNED_KERNEL=1
endif

ENABLE_USB30_SUPPORT := 1
ENABLE_SDHCI_SUPPORT := 1
ENABLE_BOOT_CONFIG_SUPPORT := 1

MODULES += app/aboot

#DEFINES += WITH_DEBUG_DCC=1
DEFINES += WITH_DEBUG_UART=1
#DEFINES += WITH_DEBUG_FBCON=1
DEFINES += DEVICE_TREE=1
DEFINES += SPMI_CORE_V2=1
DEFINES += BAM_V170=1
DEFINES += USE_BOOTDEV_CMDLINE=1
DEFINES += USE_MDM_BOOT_CFG=1
DEFINES += USE_TARGET_HS200_CAPS=1

ifeq ($(ENABLE_USB30_SUPPORT),1)
DEFINES += USB30_SUPPORT=1
endif

ifeq ($(ENABLE_SDHCI_SUPPORT),1)
DEFINES += MMC_SDHCI_SUPPORT=1
endif

#disable Thumb mode for the codesourcery/arm-2011.03 toolchain
ENABLE_THUMB := false

#Override linker for mdm targets
LD := $(TOOLCHAIN_PREFIX)ld.bfd

ENABLE_SMD_SUPPORT := 1
ifeq ($(ENABLE_SMD_SUPPORT),1)
DEFINES += SMD_SUPPORT=1
endif

# Reset USB clock from target code
DEFINES += USB_RESET_FROM_CLK=1

# Turn on Werror
CFLAGS += -Werror
DEFINES += USE_TARGET_QMP_SETTINGS=1
