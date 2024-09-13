BL_VERS_NUM=5.5.0.0

CONFIG_BL_FULLMAC ?= m


KERNELVERSION := 	$(shell uname -r)

CONFIG_DEBUG_FS ?= n

ifeq ($(CONFIG_DEBUG_FS),y)
CONFIG_BL_DEBUGFS ?= y
export CONFIG_BL_DEBUGFS ?= y
# Support of P2P DebugFS for enabling/disabling NoA and OppPS
CONFIG_RWNX_P2P_DEBUGFS ?= y
endif


subdir-ccflags-$(CONFIG_RWNX_P2P_DEBUGFS) += -DCONFIG_RWNX_P2P_DEBUGFS
subdir-ccflags-$(CONFIG_BL_DEBUGFS) += -DCONFIG_BL_DEBUGFS
subdir-ccflags-y += -I$(src)/fullmac

# FW VARS
subdir-ccflags-y += -DNX_VIRT_DEV_MAX=4
subdir-ccflags-y += -DNX_REMOTE_STA_MAX=10
subdir-ccflags-y += -DNX_CHAN_CTXT_CNT=3

ARCH := riscv
CROSS_COMPILE = riscv64-unknown-linux-musl-

KBUILD_CFLAGS += -w
obj-$(CONFIG_BL_FULLMAC) += fullmac/


KERNELDIR ?= /data/liyuan.si/code/v4.1.0.cv1811c.wifi/linux_5.10/build/cv1811c_wevb_0006a_spinor/

all: modules

modules clean:
	@$(PWD)/mklink.sh
	# $(MAKE) -C $(KERNELDIR) O=$(KBUILDDIR) M=$(PWD) $@
	$(MAKE) -C $(KERNELDIR) O=$(KBUILDDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) M=$(PWD) $@
	@$(PWD)/mklink.sh clean

