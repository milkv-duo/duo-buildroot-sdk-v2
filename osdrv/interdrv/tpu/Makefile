include $(PWD)/../Makefile.interdrv.param

PWD := $(shell pwd)

ccflags-y +=	-I$(src)/../include/uapi \
				-I$(src)/common \
				-I$(src)/hal/$(CVIARCH_L) \
				-I$(srctree)/drivers/tee \
				-I$(srctree)/arch/arm64/include \
				-I$(srctree)/drivers/staging/android/ion \
				-I$(src)/../include/chip/$(CVIARCH_L)/uapi/linux

obj-m += $(CVIARCH_L)_tpu.o
$(CVIARCH_L)_tpu-y += common/cvi_tpu_interface.o
$(CVIARCH_L)_tpu-y += hal/$(CVIARCH_L)/tpu_platform.o
$(CVIARCH_L)_tpu-y += hal/$(CVIARCH_L)/tpu_pmu.o

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
