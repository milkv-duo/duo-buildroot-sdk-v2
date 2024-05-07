include $(PWD)/../Makefile.interdrv.param

SARADC_PATH = $(CVIARCH_L)

$(CVIARCH_L)_saradc-objs = $(SARADC_PATH)/cvi_saradc.o

obj-m += $(CVIARCH_L)_saradc.o

ccflags-y += -I$(PWD)/common/

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
