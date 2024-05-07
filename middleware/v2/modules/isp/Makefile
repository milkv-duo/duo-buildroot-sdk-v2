.PHONY: clean all
.SILENT: clean all

include isp.mk

all:
	cp $(shell echo $(CHIP_ARCH) | tr A-Z a-z)/$(SDK_VER)/lib* $(MW_LIB)

clean:
	#
