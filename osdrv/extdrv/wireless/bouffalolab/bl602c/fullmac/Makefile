# Enable A-MSDU support (need FW support)
## Select this if FW is compiled with AMSDU support
CONFIG_BL_SPLIT_TX_BUF ?= n
## Select this TO send AMSDU
CONFIG_BL_AMSDUS_TX ?= n
## mp cmds
CONFIG_BL_MP ?= y
## use cfg80211 wext standard ioctl to support wext mode
CONFIG_SUPPORT_WEXT_MODE ?= n

# Enable HW queue for Broadcast/Multicast traffic (need FW support)
CONFIG_BL_BCMC ?= y

# extra DEBUG config
CONFIG_BL_SW_PROFILING ?= n
CONFIG_BL_DBG ?= y
# If dump_fw is y, then dump whole fw. If no, only print epc registers.
CONFIG_BL_DUMP_FW ?= n
CONFIG_AUTO_DNLD ?= y
CONFIG_BL_TRACE ?= n
CONFIG_BL_DNLD_FWBIN ?=n
CONFIG_BL_RX_REORDER ?=y
CONFIG_BL_FW_RETRY ?=y
CONFIG_BL_INT_WRITE_CLEAR ?=n
CONFIG_BL_RX_DEFRAG ?=y
obj-m := bl_fdrv.o
bl_fdrv-y := bl_cfgfile.o           \
               bl_iwpriv.o          \
               bl_main.o            \
               bl_mod_params.o      \
               bl_platform.o	    \
	           bl_sdio.o            \
               bl_msg_tx.o          \
               bl_msg_rx.o          \
               bl_utils.o           \
               bl_cmds.o            \
               bl_irqs.o            \
               ipc_host.o           \
               bl_txq.o             \
               bl_strs.o            \
               bl_tx.o              \
               bl_rx.o              \
               bl_v7.o              \
               bl_ftrace.o          \
               bl_bootrom.o

ifeq ($(CONFIG_BL_DNLD_FWBIN), n)               
        bl_fdrv-y += bl_fwbin.o
        bl_fdrv-y += bl_caldata.o
ifeq ($(CONFIG_BL_MP), y)
        bl_fdrv-y += bl_mpfwbin.o
endif
endif

bl_fdrv-$(CONFIG_DEBUG_FS)         += bl_debugfs.o

ccflags-y := -DCONFIG_BL_FULLMAC
ccflags-y += -I$(src)/..
ccflags-$(CONFIG_BL_SPLIT_TX_BUF) += -DCONFIG_BL_SPLIT_TX_BUF
ifeq ($(CONFIG_BL_SPLIT_TX_BUF), y)
ccflags-$(CONFIG_BL_AMSDUS_TX) += -DCONFIG_BL_AMSDUS_TX
endif
ccflags-$(CONFIG_BL_DBG) += -DCONFIG_BL_DBG
ccflags-$(CONFIG_BL_MOD_LEV_DBG) += -DCONFIG_BL_MOD_LEV_DBG
ccflags-$(CONFIG_BL_DNLD_FWBIN) += -DCONFIG_BL_DNLD_FWBIN
ccflags-$(CONFIG_BL_RX_REORDER) += -DBL_RX_REORDER
ccflags-$(CONFIG_BL_FW_RETRY) += -DBL_FW_RETRY
ccflags-$(CONFIG_BL_INT_WRITE_CLEAR) += -DBL_INT_WRITE_CLEAR
ccflags-$(CONFIG_BL_RX_DEFRAG) += -DBL_RX_DEFRAG
ccflags-y += -DCONFIG_USER_MAX=1
ccflags-$(CONFIG_BL_MP) += -DCONFIG_BL_MP
ccflags-$(CONFIG_SUPPORT_WEXT_MODE) += -DCONFIG_SUPPORT_WEXT_MODE
ccflags-$(CONFIG_BL_DUMP_FW) += -DCONFIG_BL_DUMP_FW

ifeq ($(CONFIG_BL_BCMC), y)
ccflags-y += -DNX_TXQ_CNT=5
else
ccflags-y += -DNX_TXQ_CNT=4
endif

ifeq ($(CONFIG_64BIT), y)
	ccflags-y += -DBL64BIT_FLAG
endif

#quiet_cmd_genvers = GENVERSION $@
#      cmd_genvers = ($(if $(KBUILD_EXTMOD),,$(srctree)/)$(src)/../mkvers.sh $@)

#$(obj)/bl_main.o: $(obj)/bl_version_gen.h

#$(obj)/bl_version_gen.h: FORCE
#	$(call cmd,genvers)

clean-files := bl_version_gen.h
