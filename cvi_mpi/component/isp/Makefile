SHELL = /bin/bash
include $(BUILD_PATH)/.config
include sensor.mk

chip_arch = $(shell echo $(CHIP_ARCH) | tr A-Z a-z)

all:
	@cd common; $(MAKE) all || exit 1; cd ../;
	pushd sensor/$(chip_arch) && \
	$(MAKE) all && \
	popd;

clean:
	@cd common; $(MAKE) clean; cd ../;
	pushd sensor/$(chip_arch) && \
	$(MAKE) clean && \
	popd;
