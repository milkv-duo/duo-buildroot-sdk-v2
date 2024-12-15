SHELL = /bin/bash
#
ifeq ($(PARAM_FILE), )
     PARAM_FILE:=../../mpi_param.mk
     include $(PARAM_FILE)
endif
#
SDIR	:= $(CURDIR)/src
SRCS_C	:= $(SDIR)/cvi_bin.c
SRCS_C	+= $(SDIR)/md5.c
SRCS_C	+= $(SDIR)/rw_json.c
SRCS_C	+= $(SDIR)/vo_bin.c
SRCS_C	+= $(SDIR)/vo_json_struct.c
SRCS_C	+= $(SDIR)/vpss_bin.c
SRCS_C	+= $(SDIR)/vpss_json_struct.c
#
ODIR    := $(CURDIR)/obj
OBJS    := $(SRCS_C:$(SDIR)/%.c=$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).o)
DEPS	:= $(SRCS_C:.c=.d)

# 3rdparty lib
TRD_INCLUDE_PATH = $(CURDIR)/prebuilt/include
#
ifneq ($(findstring $(CHIP_ARCH), CV181X CV180X),)
ISP_BIN_DIR = $(MW_PATH)/modules/isp/$(shell echo $(CHIP_ARCH) | tr A-Z a-z)/isp_bin/inc
else
$(error UNKNOWN chip architecture - $(CHIP_ARCH))
endif

INCS = -I$(MW_INC) -I$(KERNEL_INC) -I$(SYS_INC) -I$(VPSS_INC) -I$(ISP_INC) -Iinclude
INCS += -I$(SYS_INC) -I$(TRD_INCLUDE_PATH)/cvi_json-c -I$(ISP_BIN_DIR) -I$(TRD_INCLUDE_PATH)/cvi_miniz

TARGET_A = $(MW_LIB)/libcvi_bin.a
TARGET_SO = $(MW_LIB)/libcvi_bin.so

EXTRA_CFLAGS = $(INCS) -fpack-struct=8
EXTRA_CFLAGS += -DMMF_VERSION=\"$(shell git describe --always)\"
EXTRA_CFLAGS += -DISP_COMMIT=\"$(shell git rev-parse --short HEAD)\"
EXTRA_CFLAGS += -DISP_GERRIT=\"$(shell git log -n1 | grep Reviewed-on | tail -n1 | sed "s/.*\///g")\"
ifeq ($(MULTI_PROCESS_SUPPORT), 1)
EXTRA_CFLAGS += -DRPC_MULTI_PROCESS
endif

.PHONY : clean all
all : $(TARGET_A) $(TARGET_SO)

$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).o : $(SDIR)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -c $< -o $@
	@echo [$(notdir $(CC))] $(notdir $@)

$(TARGET_A): $(OBJS)
	@$(AR) $(ARFLAGS) $(TARGET_A) $(OBJS)
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $(TARGET_A))
	@echo "$$AR_MRI" | $(AR) -M

$(TARGET_SO): $(OBJS)
	@$(LD) $(LDFLAGS) $(EXTRA_LDFLAGS) -o $(TARGET_SO) --start-group $(OBJS) $(LIBS) --end-group
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $(TARGET_SO))
	@$(STRIP) $(TARGET_SO)

clean:
	@rm -rf $(TARGET_A) $(TARGET_SO)

clean_all: clean
	@if [ -d "$(SDIR)" ]; then \
		echo "rm -rf $(ODIR)"; \
		rm -rf $(ODIR); \
	fi

-include $(DEPS)
