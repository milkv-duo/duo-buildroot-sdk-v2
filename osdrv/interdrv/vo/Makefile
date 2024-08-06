include $(PWD)/../Makefile.interdrv.param

ifneq ($(KERNELRELEASE),)
include $(PWD)/chip/$(CVIARCH_L)/chip.mk

$(CVIARCH_L)_vo-objs += common/vo_core.o
$(CVIARCH_L)_vo-objs += common/vo_rgn_ctrl.o

obj-m += $(CVIARCH_L)_vo.o
obj-m += $(CVIARCH_L)_mipi_tx.o

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

ccflags-y += -I$(PWD)/chip/$(CVIARCH_L)/ -I$(PWD)/common/
ccflags-y += -I$(PWD)/../include/common/uapi/ -I$(PWD)/../include/chip/$(CVIARCH_L)/uapi/
ccflags-y += -I$(PWD)/../include/common/kapi/
ccflags-y += -I$(PWD)/../vpss/chip/$(CVIARCH_L)/
ccflags-y += -I$(PWD)/../vpss/common
ccflags-y += -I$(PWD)/../sys/common/ -I$(PWD)/../sys/common/uapi
ccflags-y += -I$(PWD)/../base/chip/$(CVIARCH_L)/
ccflags-y += -I$(PWD)/../base/
ccflags-y += -I$(srctree)/drivers/staging/android
ccflags-y += -I$(srctree)/drivers/pinctrl/cvitek/

KBUILD_EXTRA_SYMBOLS = $(PWD)/../base/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(PWD)/../vpss/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(PWD)/../sys/Module.symvers
else
all:
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
endif
