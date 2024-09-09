.PHONY: clean all
.SILENT: clean all

include isp.mk

all:
	cp $(shell echo $(CHIP_ARCH) | tr A-Z a-z)/$(SDK_VER)/lib* $(MW_LIB)
	if [ "$(wildcard $(isp_chip_dir))" != "" ]; then cd $(isp_chip_dir) ; $(MAKE) all || exit 1; fi;
clean:
	if [ "$(wildcard $(isp_chip_dir))" != "" ]; then cd $(isp_chip_dir) ; $(MAKE) clean || exit 1; fi;
