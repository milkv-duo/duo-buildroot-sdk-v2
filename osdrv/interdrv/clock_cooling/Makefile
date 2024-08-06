include $(PWD)/../Makefile.interdrv.param

PWD := $(shell pwd)

ccflags-y += -I$(src) -I$(src)/$(CVIARCH_L)

ccflags-y +=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-sign-compare

$(CVIARCH_L)_clock_cooling-objs = $(CVIARCH_L)/clock_cooling.o

obj-m += $(CVIARCH_L)_clock_cooling.o

all:
	$(MAKE) ARCH=${ARCH} -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd modules.order Module.symvers *.ko *.mod.c
	@rm -rf .tmp_versions
	@find . -name \*.o* -type f -delete
