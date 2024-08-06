CHIP_ID := $(patsubst "%",%,$(CHIP_ARCH_L))
#ms41929 motor depend on linux spi hw control,if u want to build success
#u need to configure it as follows
#CONFIG_SPI = y
#CONFIG_SPI_MASTER = y
#CONFIG_SPI_SPIDEV = y
#CONFIG_SPI_DESIGNWARE = y
ifneq ($(KERNELRELEASE),)

obj-m += $(CHIP_ID)_motor.o

$(CHIP_ID)_motor-y += hal/relmon/ms41929/ms41929.o
$(CHIP_ID)_motor-y += hal/relmon/ms41929/ms41929_driver.o
$(CHIP_ID)_motor-y += common/motor_driver.o

MW = $(PWD)/../../interdrv/v2
ccflags-y += -I$(PWD)/common/ -I$(PWD)/hal/relmon/ms41929/ -I$(PWD)/common/
ccflags-y += -I$(srctree)/drivers/pinctrl/cvitek/
ccflags-y += -I$(MW)/include/chip/$(CHIP_ID)/uapi/linux/
ccflags-y += -I$(MW)/include/common/uapi/linux/
ccflags-y += -I$(MW)/base/chip/$(CHIP_ID)/

test = $(ccflags-y)

KBUILD_EXTRA_SYMBOLS = $(MW)/base/Module.symvers
else
all:
	$(info $(test))
	$(MAKE) ARCH=$(ARCH) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c *.mod
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
endif
