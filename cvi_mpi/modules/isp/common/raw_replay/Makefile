SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../../../mpi_param.mk
	include $(PARAM_FILE)
endif

isp_chip_name := $(shell echo $(CHIP_ARCH) | tr A-Z a-z)
ISP_BASE_PATH = $(CURDIR)/../../$(isp_chip_name)
#
SDIR := $(CURDIR)
SRCS_C := $(SDIR)/raw_replay.c
SRCS_C += $(SDIR)/raw_replay_test.c
#
ODIR    := $(CURDIR)/obj
OBJS	:= $(SRCS_C:$(SDIR)/%.c=$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).o)
DEPS	:= $(SRCS_C:$(SDIR)/%.c=$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).d)
#
INCS = -I$(MW_INC) -I$(ISP_INC) -I$(KERNEL_INC) -I$(SYS_INC) -I$(SENSOR_LIST_INC) -I../../algo

# SOC Architecture
ifeq ($(CHIP_ARCH), CV181X)
	INCS += -I$(ISP_BASE_PATH)/isp/inc
	INCS += -I$(VI_INC)
	INCS += -I$(VPSS_INC)
	INCS += -I$(VO_INC)
	INCS += -I$(GDC_INC)
	INCS += -I$(RGN_INC)
else ifeq ($(CHIP_ARCH), CV180X)
	INCS += -I$(ISP_BASE_PATH)/isp/inc
	INCS += -I$(VI_INC)
	INCS += -I$(VPSS_INC)
	INCS += -I$(VO_INC)
	INCS += -I$(GDC_INC)
	INCS += -I$(RGN_INC)
else
	$(error Un-support SOC Architecture)
endif

TARGET_A = $(MW_LIB)/libraw_replay.a
TARGET_SO = $(MW_LIB)/libraw_replay.so

EXTRA_CFLAGS = $(INCS)
EXTRA_CFLAGS += $(DEFS)

.PHONY: clean all sdk_release
ifeq ($(ISP_SRC_RELEASE),1)
all: $(TARGET_A) $(TARGET_SO) sdk_release
else
all: $(TARGET_A) $(TARGET_SO)
endif

$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).o : $(SDIR)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c $< -o $@
	@echo [$(notdir $(CC))] $(notdir $@)

$(TARGET_A): $(OBJS)
	@$(AR) $(ARFLAGS) $@ $(OBJS)
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $@)

$(TARGET_SO): $(OBJS)
	@$(LD) $(LDFLAGS) -o $@ --start-group $(OBJS) $(LIBS) --end-group
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $@)

sdk_release: $(TARGET_A) $(TARGET_SO)
	-@rm -f $(OBJS) $(DEPS)
	-@cp $(TARGET_A) .
	-@cp $(TARGET_SO) .
	-@rm -f *.c
	-@mv Makefile.sdk_release Makefile

clean:
	@rm -f $(TARGET_A) $(TARGET_SO)

clean_all: clean
	@if [ -d "$(SDIR)" ]; then \
		echo "rm -rf $(ODIR)"; \
		rm -rf $(ODIR); \
	fi

-include $(DEPS)
