CVI_PINMUX_VERSION = 1.0.0
CVI_PINMUX_SITE = $(call github,milkv-duo,cvi-pinmux,$(CVI_PINMUX_VERSION))
CVI_PINMUX_INSTALL_STAGING = YES

ifeq ($(BR2_PACKAGE_CVI_PINMUX_CV180X),y)
    CVI_SRC_DIR = cv180x
else ifeq ($(BR2_PACKAGE_CVI_PINMUX_SG200X),y)
    CVI_SRC_DIR = sg200x
else
    $(error "Please select either CV180X or SG200X")
endif

define CVI_PINMUX_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_LDFLAGS) \
		$(@D)/$(CVI_SRC_DIR)/*.c -o $(@D)/cvi-pinmux
endef

define CVI_PINMUX_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/cvi-pinmux $(TARGET_DIR)/usr/bin/
endef

$(eval $(generic-package))
