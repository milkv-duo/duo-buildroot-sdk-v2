ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../mpi_param.mk
	include $(PARAM_FILE)
endif

isp_chip_dir := $(shell echo $(CHIP_ARCH) | tr A-Z a-z)


