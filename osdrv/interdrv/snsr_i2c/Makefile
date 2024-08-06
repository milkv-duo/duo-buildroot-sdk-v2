include $(PWD)/../Makefile.interdrv.param

ifneq ($(KERNELRELEASE),)
snsr_i2c-objs += common/snsr_i2c.o

obj-m += snsr_i2c.o

ccflags-y += -I$(PWD)/chip/$(CVIARCH_L)/ -I$(PWD)/common/ -I$(PWD)/../include/chip/$(CVIARCH_L)/uapi
ccflags-y += -I$(PWD)/../include/common/kapi/

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

KBUILD_EXTRA_SYMBOLS = $(PWD)/../base/Module.symvers
else
all:
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
endif
