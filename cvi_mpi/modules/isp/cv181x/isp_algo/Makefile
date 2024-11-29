SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../../../mpi_param.mk
	include $(PARAM_FILE)
endif

#
SDIR = $(CURDIR)/src
SRCS_C += $(SDIR)/dpcm_api.c
SRCS_C += $(SDIR)/isp_algo_blc.c
SRCS_C += $(SDIR)/isp_algo_bnr.c
SRCS_C += $(SDIR)/isp_algo_ca.c
SRCS_C += $(SDIR)/isp_algo_ca2.c
SRCS_C += $(SDIR)/isp_algo_cac.c
SRCS_C += $(SDIR)/isp_algo_ccm.c
SRCS_C += $(SDIR)/isp_algo_clut.c
SRCS_C += $(SDIR)/isp_algo_cnr.c
SRCS_C += $(SDIR)/isp_algo_crosstalk.c
SRCS_C += $(SDIR)/isp_algo_csc.c
SRCS_C += $(SDIR)/isp_algo_dci.c
SRCS_C += $(SDIR)/isp_algo_debug.c
SRCS_C += $(SDIR)/isp_algo_dehaze.c
SRCS_C += $(SDIR)/isp_algo_demosaic.c
SRCS_C += $(SDIR)/isp_algo_dis.c
SRCS_C += $(SDIR)/isp_algo_dpc.c
SRCS_C += $(SDIR)/isp_algo_drc.c
SRCS_C += $(SDIR)/isp_algo_fswdr.c
SRCS_C += $(SDIR)/isp_algo_gamma.c
SRCS_C += $(SDIR)/isp_algo_lcac.c
SRCS_C += $(SDIR)/isp_algo_ldci.c
SRCS_C += $(SDIR)/isp_algo_mlsc.c
SRCS_C += $(SDIR)/isp_algo_mono.c
SRCS_C += $(SDIR)/isp_algo_motion.c
SRCS_C += $(SDIR)/isp_algo_presharpen.c
SRCS_C += $(SDIR)/isp_algo_rgbcac.c
SRCS_C += $(SDIR)/isp_algo_sharpen.c
SRCS_C += $(SDIR)/isp_algo_tnr.c
SRCS_C += $(SDIR)/isp_algo_utility.c
SRCS_C += $(SDIR)/isp_algo_wb.c
SRCS_C += $(SDIR)/isp_algo_ycontrast.c
SRCS_C += $(SDIR)/isp_algo_ynr.c
SRCS_C += $(SDIR)/isp_iir_api.c
SRCS_C += $(SDIR)/isp_rvv_utility.c
SRCS_C += $(SDIR)/pchip.c
#
ODIR    := $(CURDIR)/obj
OBJS	:= $(SRCS_C:$(SDIR)/%.c=$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).o)
DEPS	:= $(SRCS_C:$(SDIR)/%.c=$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).d)
#
INCS = -I$(KERNEL_INC) -I$(MW_INC) -I$(ISP_INC) -I$(KERNEL_INC) -I$(SYS_INC) -Iinc -Isrc -I../isp/inc
INCS += -I../../common/clog

RELEASE_FOLDER = $(CURDIR)/bin

TARGET_A = $(MW_LIB)/libisp_algo.a
TARGET_SO = $(MW_LIB)/libisp_algo.so

LOCAL_CFLAGS = $(INCS)
LOCAL_LDFLAGS =

# Toolchain
ifeq ($(TARGET_MACHINE), riscv64-unknown-linux-gnu)
	LOCAL_CFLAGS += -DSDK_LIB_BIT=64
else ifeq ($(TARGET_MACHINE), riscv64-unknown-linux-musl)
	LOCAL_CFLAGS += -DSDK_LIB_BIT=64
else ifeq ($(TARGET_MACHINE), arm-linux-gnueabihf)
	LOCAL_CFLAGS += -DSDK_LIB_BIT=32
else
	LOCAL_CFLAGS += -DSDK_LIB_BIT=64
endif

.PHONY: clean all cleanall sdk_release

ifeq ($(ISP_SRC_RELEASE),1)
all: $(TARGET_A) $(TARGET_SO) sdk_release
else
all: $(TARGET_A) $(TARGET_SO)
endif

$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).o : $(SDIR)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(LOCAL_CFLAGS) -c $< -o $@
	@echo [$(notdir $(CC))] $(notdir $@)

$(TARGET_A): $(OBJS)
	@$(AR) $(ARFLAGS) $@ $(OBJS)
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $(TARGET_A))

$(TARGET_SO): $(OBJS)
	@$(LD) $(LDFLAGS) $(LOCAL_LDFLAGS) -o $@ --start-group $(OBJS) --end-group
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $(TARGET_SO))

sdk_release: $(TARGET_A) $(TARGET_SO)
	@echo "Create $(RELEASE_FOLDER)"
	-@mkdir -p $(RELEASE_FOLDER)
	-@rm -rf $(RELEASE_FOLDER)/*
	-@cp $(TARGET_A) $(RELEASE_FOLDER)
	-@cp $(TARGET_SO) $(RELEASE_FOLDER)
	-@rm -rf src
	-@rm -rf $(ODIR)
	-@mv Makefile.sdk_release Makefile

clean:
	@rm -f $(TARGET_A) $(TARGET_SO)

clean_all: clean
	@if [ -d "$(SDIR)" ]; then \
		echo "rm -rf $(ODIR)"; \
		rm -rf $(ODIR); \
		echo "rm -rf $(RELEASE_FOLDER)"; \
		rm -rf $(RELEASE_FOLDER); \
	fi

cleanall: clean_all

-include $(DEPS)
