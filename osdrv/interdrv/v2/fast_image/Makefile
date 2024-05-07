include $(PWD)/../Makefile.interdrv.param

$(CVIARCH_L)_fast_image-objs = fast_image.o
obj-m += $(CVIARCH_L)_fast_image.o

ccflags-y += -I$(src)
ccflags-y += -I$(PWD)/../rtos_cmdqu/
ccflags-y += -I$(srctree)/drivers/staging/android/

ccflags-y += -I$(PWD)/../include/common/uapi/ -I$(PWD)/../include/chip/$(CVIARCH_L)/uapi/
ccflags-y += -I$(PWD)/../include/common/kapi/
ccflags-y += -I$(PWD)/../include/
ccflags-y += -I$(PWD)/../base/
ccflags-y += -I$(PWD)/../base/chip/$(CVIARCH_L)

ccflags-y += -I$(PWD)/../sys/common/ -I$(PWD)/../sys/common/uapi

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

KBUILD_EXTRA_SYMBOLS = $(PWD)/../rtos_cmdqu/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(PWD)/../sys/Module.symvers

all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c
	rm -rf .tmp_versions

