SOPHGO_LIBRARY_VERSION = 1.0.0
SOPHGO_LIBRARY_SITE = $(call github,milkv-duo,sophgo-library,$(SOPHGO_LIBRARY_VERSION))
SOPHGO_LIBRARY_INSTALL_STAGING = YES

ifeq ($(BR2_aarch64),y)
ifeq ($(BR2_TOOLCHAIN_USES_GLIBC),y)
SOPHGO_LIB_DIR = glibc_arm64
else ifeq ($(BR2_TOOLCHAIN_USES_MUSL),y)
SOPHGO_LIB_DIR = musl_arm64
endif
else ifeq ($(BR2_RISCV_64),y)
ifeq ($(BR2_TOOLCHAIN_USES_GLIBC),y)
SOPHGO_LIB_DIR = glibc_riscv64
else ifeq ($(BR2_TOOLCHAIN_USES_MUSL),y)
SOPHGO_LIB_DIR = musl_riscv64
endif
else
SOPHGO_LIB_DIR = $(BR2_ARCH)
endif

define SOPHGO_LIBRARY_INSTALL_STAGING_CMDS
	cp -a $(@D)/$(SOPHGO_LIB_DIR)/lib/* $(STAGING_DIR)/usr/lib/
endef

define SOPHGO_LIBRARY_INSTALL_TARGET_CMDS
	$(Q)mkdir -p $(TARGET_DIR)/mnt/system/lib
	cp -a $(@D)/$(SOPHGO_LIB_DIR)/lib/* $(TARGET_DIR)/mnt/system/lib/
endef

$(eval $(generic-package))
