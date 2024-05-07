include $(PWD)/../Makefile.interdrv.param

ifneq ($(KERNELRELEASE),)

obj-m += $(CVIARCH_L)_ive.o
$(CVIARCH_L)_ive-y += common/cvi_ive_interface.o
$(CVIARCH_L)_ive-y += hal/$(CVIARCH_L)/cvi_reg.o
$(CVIARCH_L)_ive-y += hal/$(CVIARCH_L)/cvi_ive_platform.o

ccflags-y += -I$(PWD)/../include/common/uapi -I$(PWD)/../include/chip/$(CVIARCH_L)/uapi/
ccflags-y += -I$(PWD)/../include/common/kapi -I$(PWD)/../sys/common/uapi/
ccflags-y += -I$(src)/common -I$(src)/hal/$(CVIARCH_L) -I$(src)/hal/$(CVIARCH_L)/include
ccflags-y += -I$(PWD)/../base/ -I$(srctree)/drivers/staging/android
ccflags-y += -I$(PWD)/../base/chip/$(CVIARCH_L)

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

KBUILD_EXTRA_SYMBOLS += $(PWD)/../sys/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(PWD)/../base/Module.symvers

else
all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
endif
