SHELL = /bin/bash

.PHONY: prepare clean

prepare :
	@mkdir -p include/linux
	@mkdir -p include/isp
	@cp -pf $(OSDRV_PATH)/interdrv/include/common/uapi/linux/*.h include/linux/
	@cp -pf $(OSDRV_PATH)/interdrv/include/chip/$(CHIP_ARCH_L)/uapi/linux/*.h include/linux/
	@cp -pf $(KERNEL_PATH)/drivers/staging/android/uapi/ion.h include/linux/
	@cp -pf $(KERNEL_PATH)/drivers/staging/android/uapi/ion_cvitek.h include/linux/
	@cp -rpf modules/isp/include/* include/isp
ifeq ($(CHIP_ARCH), $(filter $(CHIP_ARCH), CV181X CV180X))
	@cp -rpf modules/isp/cv181x/isp-daemon2/inc/cvi_ispd2.h include/isp/cv181x/
endif

clean :
	@rm -rf self_test/dl_daemon/build
	@rm -rf include/linux
	@rm -rf include/isp