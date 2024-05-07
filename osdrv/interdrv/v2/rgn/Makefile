include $(PWD)/../Makefile.interdrv.param

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

ifneq ($(KERNELRELEASE),)
include $(PWD)/chip/$(CVIARCH_L)/chip.mk

$(CVIARCH_L)_rgn-objs += common/rgn_core.o

obj-m += $(CVIARCH_L)_rgn.o

ccflags-y += -I$(PWD)/common/ \
             -I$(PWD)/../base/ \
             -I$(PWD)/../base/chip/$(CVIARCH_L) \
             -I$(PWD)/../sys/common/ \
             -I$(PWD)/../sys/common/uapi \
             -I$(srctree)/drivers/staging/android \
             -I$(PWD)/../include/common/uapi/ \
             -I$(PWD)/../include/chip/$(CVIARCH_L)/uapi/ \
             -I$(PWD)/../include/common/kapi/ \
             -I$(PWD)/chip/$(CVIARCH_L) \
             -I$(PWD)/../rtos_cmdqu/

KBUILD_EXTRA_SYMBOLS = $(PWD)/../base/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(PWD)/../sys/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(PWD)/../vpss/Module.symvers
else
all:
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
endif
