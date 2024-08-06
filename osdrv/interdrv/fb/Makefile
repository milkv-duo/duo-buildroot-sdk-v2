include $(PWD)/../Makefile.interdrv.param

obj-m += cvi_fb.o
cvi_fb-y += cvifb.o

#cvi_fb-objs := cvifb.o
#obj-$(CONFIG_FB_CVITEK) += cvi_fb.o
ccflags-y += -I$(srctree)/drivers/staging/android/
ccflags-y += -I$(PWD)/../base/
ccflags-y += -I$(PWD)/../base/chip/$(CVIARCH_L)/
ccflags-y += -I$(PWD)/../vpss/chip/$(CVIARCH_L)/
ccflags-y += -I$(PWD)/../include/common/uapi/
ccflags-y += -I$(PWD)/../include/chip/$(CVIARCH_L)/uapi/
ccflags-y += -I$(PWD)/../include/common/kapi/

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

KBUILD_EXTRA_SYMBOLS += $(PWD)/../base/Module.symvers
KBUILD_EXTRA_SYMBOLS += $(PWD)/../vpss/Module.symvers

all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c
	rm -rf .tmp_versions
