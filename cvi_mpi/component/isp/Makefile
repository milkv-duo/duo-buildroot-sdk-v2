SHELL = /bin/bash
#
ifneq ($(BUILD_PATH),)
include $(BUILD_PATH)/.config
endif
#
chip_arch = $(shell echo $(CHIP_ARCH) | tr A-Z a-z)

all:
	@$(MAKE) -C common all || exit 1;
	@$(MAKE) -C sensor/$(chip_arch) all || exit 1;

clean:
	@$(MAKE) -C common clean || exit 1;
	@$(MAKE) -C sensor/$(chip_arch) clean || exit 1;
