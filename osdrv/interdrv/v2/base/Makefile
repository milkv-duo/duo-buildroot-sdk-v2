include $(PWD)/../Makefile.interdrv.param

$(CVIARCH_L)_base-objs = base.o \
				chip/$(CVIARCH_L)/vip_sys.o \
				reg.o \
				cvi_vb_proc.o \
				cvi_log_proc.o \
				cvi_sys_proc.o \
				base_context.o \
				vb.o

ifneq ($(INTRERDRV_FLAGS), )
$(CVIARCH_L)_base-objs += vb_test.o
endif

obj-m += $(CVIARCH_L)_base.o

ccflags-y += -I$(srctree)/drivers/tee \
             -I$(PWD)/../include/common/uapi/ \
             -I$(PWD)/../include/common/kapi/ \
             -I$(PWD)/../base/ \
             -I$(PWD)/../include/chip/$(CVIARCH_L)/uapi/ \
             -I$(PWD)/../sys/common/ \
             -I$(PWD)/../sys/common/uapi \
             -I$(srctree)/drivers/staging/android \
             -I$(PWD)/chip/$(CVIARCH_L)


ifeq ($(SUBTYPE), fpga)
ccflags-y += -DDRV_FPGA_PORTING
endif

ccflags-y += $(INTRERDRV_FLAGS)

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

KBUILD_EXTRA_SYMBOLS += $(PWD)/../sys/Module.symvers

all:
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
