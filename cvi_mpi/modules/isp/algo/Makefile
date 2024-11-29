SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../../mpi_param.mk
	include $(PARAM_FILE)
endif

ifeq ($(ISP_3A_CUSTOMIZE_AE),1)
AE_LIB = custom_ae
else
AE_LIB = ae
endif

ifeq ($(ISP_3A_CUSTOMIZE_AF),1)
AF_LIB = custom_af
else
AF_LIB = af
endif

ifeq ($(ISP_3A_CUSTOMIZE_AWB),1)
AWB_LIB = custom_awb
else
AWB_LIB = awb
endif

.PHONY : clean all algo_ae algo_awb algo_af
.SILENT: clean all algo_ae algo_awb algo_af

all : algo_ae algo_awb algo_af

algo_ae:
	cd $(AE_LIB); make;

algo_awb:
	cd $(AWB_LIB); make;

algo_af:
	cd $(AF_LIB); make;

clean:
	make -C $(AE_LIB) clean
	make -C $(AWB_LIB) clean
	make -C $(AF_LIB) clean

clean_all:
	make -C $(AE_LIB) clean_all
	make -C $(AWB_LIB) clean_all
	make -C $(AF_LIB) clean_all
