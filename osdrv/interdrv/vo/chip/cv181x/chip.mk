CONFIG_CVI_LOG = 1

$(CVIARCH_L)_vo-objs += chip/$(CVIARCH_L)/vo.o
$(CVIARCH_L)_vo-objs += chip/$(CVIARCH_L)/vo_sdk_layer.o
$(CVIARCH_L)_vo-objs += chip/$(CVIARCH_L)/proc/vo_disp_proc.o
$(CVIARCH_L)_vo-objs += chip/$(CVIARCH_L)/proc/vo_proc.o

$(CVIARCH_L)_mipi_tx-objs := chip/$(CVIARCH_L)/vo_mipi_tx.o \
					chip/$(CVIARCH_L)/proc/vo_mipi_tx_proc.o

ifeq ($(CONFIG_CVI_LOG), 1)
ccflags-y += -DCONFIG_CVI_LOG
endif