SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../../../mpi_param.mk
	include $(PARAM_FILE)
endif

#
SDIR := $(CURDIR)/src
SRCS_C := $(SDIR)/cvi_isp.c
SRCS_C += $(SDIR)/cvi_mipi.c
SRCS_C += $(SDIR)/isp_3a.c
SRCS_C += $(SDIR)/isp_blc_ctrl.c
SRCS_C += $(SDIR)/isp_bnr_ctrl.c
SRCS_C += $(SDIR)/isp_ca2_ctrl.c
SRCS_C += $(SDIR)/isp_cac_ctrl.c
SRCS_C += $(SDIR)/isp_ca_ctrl.c
SRCS_C += $(SDIR)/isp_ccm_ctrl.c
SRCS_C += $(SDIR)/isp_clut_ctrl.c
SRCS_C += $(SDIR)/isp_cnr_ctrl.c
SRCS_C += $(SDIR)/isp_control.c
SRCS_C += $(SDIR)/isp_crosstalk_ctrl.c
SRCS_C += $(SDIR)/isp_csc_ctrl.c
SRCS_C += $(SDIR)/isp_dci_ctrl.c
SRCS_C += $(SDIR)/isp_debug.c
SRCS_C += $(SDIR)/isp_dehaze_ctrl.c
SRCS_C += $(SDIR)/isp_demosaic_ctrl.c
SRCS_C += $(SDIR)/isp_dis_ctrl.c
SRCS_C += $(SDIR)/isp_dpc_ctrl.c
SRCS_C += $(SDIR)/isp_drc_ctrl.c
SRCS_C += $(SDIR)/isp_feature_ctrl.c
SRCS_C += $(SDIR)/isp_flow_ctrl.c
SRCS_C += $(SDIR)/isp_freeze.c
SRCS_C += $(SDIR)/isp_fswdr_ctrl.c
SRCS_C += $(SDIR)/isp_gamma_ctrl.c
SRCS_C += $(SDIR)/isp_interpolate.c
SRCS_C += $(SDIR)/isp_lcac_ctrl.c
SRCS_C += $(SDIR)/isp_ldci_ctrl.c
SRCS_C += $(SDIR)/isp_lut.c
SRCS_C += $(SDIR)/isp_mailbox.c
SRCS_C += $(SDIR)/isp_mgr_buf.c
SRCS_C += $(SDIR)/isp_mgr.c
SRCS_C += $(SDIR)/isp_mlsc_ctrl.c
SRCS_C += $(SDIR)/isp_mono_ctrl.c
SRCS_C += $(SDIR)/isp_motion_ctrl.c
SRCS_C += $(SDIR)/isp_mw_compat.c
SRCS_C += $(SDIR)/isp_param_default.c
SRCS_C += $(SDIR)/isp_peri_ctrl.c
SRCS_C += $(SDIR)/isp_presharpen_ctrl.c
SRCS_C += $(SDIR)/isp_proc.c
SRCS_C += $(SDIR)/isp_rgbcac_ctrl.c
SRCS_C += $(SDIR)/isp_sensor.c
SRCS_C += $(SDIR)/isp_sharpen_ctrl.c
SRCS_C += $(SDIR)/isp_sts_ctrl.c
SRCS_C += $(SDIR)/isp_tnr_ctrl.c
SRCS_C += $(SDIR)/isp_tun_buf_ctrl.c
SRCS_C += $(SDIR)/isp_wb_ctrl.c
SRCS_C += $(SDIR)/isp_ycontrast_ctrl.c
SRCS_C += $(SDIR)/isp_ynr_ctrl.c
SRCS_C += $(SDIR)/../../../common/clog/clog.c
#
ODIR    := $(CURDIR)/obj
OBJS	:= $(SRCS_C:$(SDIR)/%.c=$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).o)
DEPS	:= $(SRCS_C:$(SDIR)/%.c=$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).d)
#
INCS = -I$(MW_INC) -I$(ISP_INC) -I$(SYS_INC) -I$(VI_INC) -Iinc -Isrc -I../isp_algo/inc
INCS += -I../../common/clog
#
TARGET_A = $(MW_LIB)/libisp.a
TARGET_SO = $(MW_LIB)/libisp.so

LOCAL_CFLAGS = $(INCS) -std=gnu11
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

#LOCAL_CFLAGS += -DFREEZE_FRAME_CO_RELEASE_MODE
LOCAL_CFLAGS += -DISP_COMMIT=\"$(shell git rev-parse --short HEAD)\"
LOCAL_CFLAGS += -DISP_GERRIT=\"$(shell git log -n1 | grep Reviewed-on | tail -n1 | sed "s/.*\///g")\"

.PHONY: clean all cleanall sdk_release
.SILENT: check_kernel_dependence update_isp_version cppcheck

ifeq ($(ISP_SRC_RELEASE),1)
all: update_isp_version $(TARGET_A) $(TARGET_SO) sdk_release
else
all: update_isp_version $(TARGET_A) $(TARGET_SO)
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

clean:
	@rm -f $(TARGET_A) $(TARGET_SO)

clean_all: clean
	@if [ -d "$(SDIR)" ]; then \
		echo "rm -rf $(ODIR)"; \
		rm -rf $(ODIR); \
	fi

cleanall: clean_all

check_kernel_dependence:
	# get kernel commit id in last 6 month
	git -C ../../../linux-linaro-stable log --since=6.month | grep -E '^commit .{40}' | sed -e 's/^commit //g' | sed -e 's/\s*//g' >kernel-commit-id.list
	# get dependent kernel id in middleware in last 1 month
	git log --since=1.month | grep -E "kernel-commit-id:" | sed -e 's/^\s*kernel-commit-id:\s*//g' | sed -e 's/\s*//g' >dependent-commit-id.list
	#cat t | grep -E "kernel-commit-id:" | sed -e 's/^\s*kernel-commit-id:\s*//g' | sed -e 's/\s*//g' >dependent-commit-id.list
	# check all dependent kernel id
	cat dependent-commit-id.list | while read -r line; do \
		if grep -Fxq $$line kernel-commit-id.list; \
			then echo "kernel commit id $$line exist"; \
			else echo "can't find kernel commit id $$line" && exit 1 ; \
		fi; \
	done
	rm kernel-commit-id.list
	rm dependent-commit-id.list

update_isp_version:
	$(eval date := $(shell git log -n1 --date=format:'%Y-%m-%d' --pretty=format:'%cd'))
	$(eval chid := $(shell git log -n1 -- $(CURDIR) | grep Change-Id: | sed 's/^.*Change-Id: //g' | cut -c1-7))
	$(eval cmid := $(shell git log -n1 --pretty=format:'%h' -- $(CURDIR)))
	echo "#define ISP_VERSION \"T(${date}), V(${chid}, ${cmid})\"" > $(CURDIR)/inc/isp_version.h

sdk_release: update_isp_version
	-@rm -rf $(ODIR)
	-@mv Makefile.sdk_release Makefile

cppcheck:
	@cppcheck --enable=all --quiet --template='{file},{line},{severity},{id}:{message}' -I ../../include -I ./inc -I ./src algo/ src/ 2>cppcheck_output.txt
	@python3 $(MW_PATH)/tool/cppcheckparser/cppcheckparser.py --input cppcheck_output.txt --output cppcheck_output.xlsx

-include $(DEPS)
