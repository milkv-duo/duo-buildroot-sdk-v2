include $(PWD)/../Makefile.interdrv.param

ccflags-y += -I$(PWD)/../include/common/uapi/ -I$(PWD)/../include/common/kapi/
ccflags-y += -I$(src)/../base/mw/ \
             -I$(src)/../base/ \
             -I$(src)/common/uapi \
             -I$(src)/common/kapi \
             -I$(src)/common \
             -I$(src)/hal/$(CVIARCH_L)/uapi \
             -I$(src)/hal/$(CVIARCH_L) \
             -I$(srctree)/drivers/staging/android \
             -I$(src)/../include/chip/$(CVIARCH_L)/uapi

ccflags-y += $(INTRERDRV_FLAGS)

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

obj-m += $(CVIARCH_L)_sys.o
$(CVIARCH_L)_sys-y += common/sys.o \
                    common/sys_context.o

ifneq ($(INTRERDRV_FLAGS), )
$(CVIARCH_L)_sys-y += common/sys_test.o
endif

all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o* *.ko
	@rm -f Module.symvers modules.order
	@rm -f *.mod.c
	@rm -rf .tmp_versions
	@rm -f .*cmd
	@rm -f $(CHIP)/*.o*
	@rm -f $(CHIP)/.*cmd
	@find . -name \*.o* -type f -delete
