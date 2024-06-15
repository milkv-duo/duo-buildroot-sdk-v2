DUO_PINMUX_VERSION = 1.0.0
DUO_PINMUX_SITE = $(call github,milkv-duo,duo-pinmux,$(DUO_PINMUX_VERSION))
DUO_PINMUX_INSTALL_STAGING = YES

ifeq ($(BR2_PACKAGE_DUO_PINMUX_DUO),y)
    DUO_SRC_DIR = duo
else ifeq ($(BR2_PACKAGE_DUO_PINMUX_DUO256M),y)
    DUO_SRC_DIR = duo256m
else ifeq ($(BR2_PACKAGE_DUO_PINMUX_DUOS),y)
    DUO_SRC_DIR = duos
else
    $(error "Please select either CV180X or SG200X")
endif

define DUO_PINMUX_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_LDFLAGS) \
		$(@D)/$(DUO_SRC_DIR)/*.c -o $(@D)/duo-pinmux
endef

define DUO_PINMUX_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/duo-pinmux $(TARGET_DIR)/usr/bin/
endef

$(eval $(generic-package))
