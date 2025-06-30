SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../../../mpi_param.mk
	include $(PARAM_FILE)
endif
include ../../isp.mk

TMP_FOLDER = $(PWD)/tmp
SDIR = $(PWD)/src
INCS = -I$(MW_INC) -I$(ISP_INC) -I$(VI_INC) -I$(KERNEL_INC) -I$(SYS_INC) -I$(VI_INC) -Iinc -Isrc
INCS += -I../isp/inc
INCS += -I../isp/src
INCS += -I../isp_algo/inc
INCS += -I../../algo
INCS += -I../../common/inc
INCS += -I../../common/clog
INCS += -I../../common/isp_ipc
INCS += -I$(MW_PATH)/../cviruntime/include

SRCS = $(wildcard $(SDIR)/*.c)

OBJS = $(patsubst $(SDIR)/%.c, $(TMP_FOLDER)/%.o, $(SRCS))
DEPS = $(patsubst $(SDIR)/%.c, $(TMP_FOLDER)/%.d, $(SRCS))

TARGET_A = $(MW_LIB)/libteaisp.a
TARGET_SO = $(MW_LIB)/libteaisp.so

LOCAL_CFLAGS = $(INCS) -std=gnu11

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

$(TMP_FOLDER)/%.o: $(SDIR)/%.c | $(TMP_FOLDER)
	@echo LOCAL_CFLAGS_EVAN=$(LOCAL_CFLAGS)
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

clean_all: clean
	@rm -rf $(TMP_FOLDER)

cleanall: clean_all

sdk_release:
	-@rm -rf $(TMP_FOLDER)
	-@mv Makefile.sdk_release Makefile

-include $(DEPS)
