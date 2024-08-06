include $(PWD)/../Makefile.interdrv.param

ifneq ($(KERNELRELEASE),)
include $(PWD)/chip/$(CVIARCH_L)/chip.mk
$(CVIARCH_L)_vpss-objs += common/cvi_vip_vpss_proc.o
$(CVIARCH_L)_vpss-objs += common/reg.o
$(CVIARCH_L)_vpss-objs += common/vpss.o
$(CVIARCH_L)_vpss-objs += common/vpss_rgn_ctrl.o
$(CVIARCH_L)_vpss-objs += common/vpss_ioctl.o

obj-m += $(CVIARCH_L)_vpss.o

ccflags-y += -I$(PWD)/chip/$(CVIARCH_L)/ -I$(PWD)/common/
ccflags-y += -I$(PWD)/../include/common/uapi/ -I$(PWD)/../include/chip/$(CVIARCH_L)/uapi/
ccflags-y += -I$(PWD)/../include/common/kapi/
ccflags-y += -I$(PWD)/../include/
ccflags-y += -I$(PWD)/../base/
ccflags-y += -I$(PWD)/../base/chip/$(CVIARCH_L)
ccflags-y += -I$(PWD)/../sys/common/ -I$(PWD)/../sys/common/uapi
ccflags-y += -I$(srctree)/drivers/tee
ccflags-y += -I$(srctree)/drivers/staging/android

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

KBUILD_EXTRA_SYMBOLS = $(PWD)/../base/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(PWD)/../sys/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(PWD)/../Module.symvers
else
all:
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
endif
