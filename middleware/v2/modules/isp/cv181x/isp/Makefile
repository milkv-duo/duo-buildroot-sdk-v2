SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../../../../$(shell echo $(MW_VER))/Makefile.param
	include $(PARAM_FILE)
endif

TMP_FOLDER = $(PWD)/tmp
SDIR = $(PWD)/src
INCS = -I$(MW_INC) -I$(ISP_INC) -I$(KERNEL_INC) -I$(SYS_INC) -I$(VPU_INC) -Iinc -Isrc -I../isp_algo/inc
INCS += -I../../common/clog
INCS += -I$(TOP_DIR)/osdrv/interdrv/v2/rtos_cmdqu

SRCS = $(wildcard $(SDIR)/*.c)
SRCS += $(wildcard $(SDIR)/../../../common/clog/*.c)

#ifeq ($(ENABLE_ISP_C906L),1)
#	SRCS := $(filter-out $(SDIR)/isp_dpc_ctrl.c, $(SRCS))
#	SRCS := $(filter-out $(SDIR)/isp_crosstalk_ctrl.c, $(SRCS))
#	SRCS := $(filter-out $(SDIR)/isp_wb_ctrl.c, $(SRCS))
#	SRCS := $(filter-out $(SDIR)/isp_bnr_ctrl.c, $(SRCS))
#	SRCS := $(filter-out $(SDIR)/isp_demosaic_ctrl.c, $(SRCS))
#	SRCS := $(filter-out $(SDIR)/isp_rgbcac_ctrl.c, $(SRCS))
#	SRCS := $(filter-out $(SDIR)/isp_lcac_ctrl.c, $(SRCS))
#	SRCS := $(filter-out $(SDIR)/isp_mlsc_ctrl.c, $(SRCS))
#	SRCS := $(filter-out $(SDIR)/isp_ccm_ctrl.c, $(SRCS))
#	SRCS := $(filter-out $(SDIR)/isp_fswdr_ctrl.c, $(SRCS))
#	SRCS := $(filter-out $(SDIR)/isp_drc_ctrl.c, $(SRCS))
#	SRCS := $(filter-out $(SDIR)/isp_gamma_ctrl.c, $(SRCS))
#	SRCS := $(filter-out $(SDIR)/isp_dehaze_ctrl.c, $(SRCS))
#	SRCS := $(filter-out $(SDIR)/isp_csc_ctrl.c, $(SRCS))
#	SRCS := $(filter-out $(SDIR)/isp_dci_ctrl.c, $(SRCS))
#	SRCS := $(filter-out $(SDIR)/isp_ldci_ctrl.c, $(SRCS))
#	SRCS := $(filter-out $(SDIR)/isp_ca_ctrl.c, $(SRCS))
#	SRCS := $(filter-out $(SDIR)/isp_presharpen_ctrl.c, $(SRCS))
#	SRCS := $(filter-out $(SDIR)/isp_tnr_ctrl.c, $(SRCS))
#	SRCS := $(filter-out $(SDIR)/isp_ynr_ctrl.c, $(SRCS))
#	SRCS := $(filter-out $(SDIR)/isp_cnr_ctrl.c, $(SRCS))
#	SRCS := $(filter-out $(SDIR)/isp_cac_ctrl.c, $(SRCS))
#	SRCS := $(filter-out $(SDIR)/isp_ca2_ctrl.c, $(SRCS))
#	SRCS := $(filter-out $(SDIR)/isp_sharpen_ctrl.c, $(SRCS))
#	SRCS := $(filter-out $(SDIR)/isp_ycontrast_ctrl.c, $(SRCS))
#	SRCS := $(filter-out $(SDIR)/isp_mono_ctrl.c, $(SRCS))
#endif

OBJS = $(patsubst $(SDIR)/%.c, $(TMP_FOLDER)/%.o, $(SRCS))
DEPS = $(patsubst $(SDIR)/%.c, $(TMP_FOLDER)/%.d, $(SRCS))

TARGET_A = $(MW_LIB)/libisp.a
TARGET_SO = $(MW_LIB)/libisp.so

LOCAL_CFLAGS = $(INCS) -std=gnu11
LOCAL_LDFLAGS =

# SOC Architecture
ifeq ($(CHIP_ARCH), CV183X)
	LOCAL_CFLAGS += -DCHIP_ARCH_CV183X
else ifeq ($(CHIP_ARCH), CV182X)
	LOCAL_CFLAGS += -DCHIP_ARCH_CV182X
else ifeq ($(CHIP_ARCH), CV181X)
	LOCAL_CFLAGS += -DCHIP_ARCH_CV181X
else ifeq ($(CHIP_ARCH), CV180X)
	LOCAL_CFLAGS += -DCHIP_ARCH_CV180X
else
	$(error Un-support SOC Architecture)
endif

ifeq ($(ENABLE_ISP_C906L),1)
	LOCAL_CFLAGS += -DENABLE_ISP_C906L
endif

# Toolchain
ifeq ($(SDK_VER), glibc_riscv64)
	LOCAL_CFLAGS += -DSDK_LIB_BIT=64
else ifeq ($(SDK_VER), musl_riscv64)
	LOCAL_CFLAGS += -DSDK_LIB_BIT=64
else ifeq ($(SDK_VER), 32bit)
	LOCAL_CFLAGS += -DSDK_LIB_BIT=32
else ifeq ($(SDK_VER), uclibc)
	LOCAL_CFLAGS += -DSDK_LIB_BIT=32
	LOCAL_CFLAGS += -DIGNORE_LOG_ERR -DIGNORE_LOG_WARNING -DIGNORE_LOG_NOTICE -DIGNORE_LOG_INFO -DIGNORE_LOG_DEBUG
	LOCAL_CFLAGS += -DAPI_ATTR_CHECK_LOG_LEVEL=1
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

$(TMP_FOLDER)/%.o: $(SDIR)/%.c | $(TMP_FOLDER)
	@$(CC) $(CFLAGS) $(LOCAL_CFLAGS) -c $< -o $@
	@echo [$(notdir $(CC))] $(notdir $@)

$(TARGET_A): $(OBJS)
	@$(AR) $(ARFLAGS) $@ $(OBJS)
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $(TARGET_A))

$(TARGET_SO): $(OBJS)
	@$(LD) $(LDFLAGS) $(LOCAL_LDFLAGS) -o $@ --start-group $(OBJS) --end-group
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $(TARGET_SO))

$(TMP_FOLDER):
	@echo "Create $(TMP_FOLDER) for object file"
	-@mkdir -p $(TMP_FOLDER)

clean:
	@rm -f $(OBJS) $(DEPS) $(TARGET_A) $(TARGET_SO)

cleanall: clean
	@rm -rf $(TMP_FOLDER)

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
	$(eval chid := $(shell git log -n1 -- $(PWD) | grep Change-Id: | sed 's/^.*Change-Id: //g' | cut -c1-7))
	$(eval cmid := $(shell git log -n1 --pretty=format:'%h' -- $(PWD)))
	echo "#define ISP_VERSION \"T(${date}), V(${chid}, ${cmid})\"" > $(PWD)/inc/isp_version.h

sdk_release: update_isp_version
	-@rm -rf $(TMP_FOLDER)
	-@mv Makefile.sdk_release Makefile

cppcheck:
	@cppcheck --enable=all --quiet --template='{file},{line},{severity},{id}:{message}' -I ../../include/$(shell echo $(MW_VER)) -I ./inc -I ./src algo/ src/ 2>cppcheck_output.txt
	@python3 $(MW_PATH)/tool/cppcheckparser/cppcheckparser.py --input cppcheck_output.txt --output cppcheck_output.xlsx

-include $(DEPS)
