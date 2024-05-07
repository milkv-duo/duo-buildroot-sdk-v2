include $(PWD)/../Makefile.interdrv.param

PWM_PATH = $(CVIARCH_L)

$(CVIARCH_L)_pwm-objs = $(PWM_PATH)/cvi_pwm.o

obj-m += $(CVIARCH_L)_pwm.o

all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
