SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
PARAM_FILE:=../../../mpi_param.mk
include $(PARAM_FILE)
endif
#
AUDIO_PARAM_FILE :=../audio.mk
include $(AUDIO_PARAM_FILE)
#
SDIR	:= $(CURDIR)/src
SRCS_C	:= $(SDIR)/cvi_audio_vqe.c
SRCS_C	+= $(SDIR)/cvi_audio_vqe_load.c
SRCS_C	+= $(SDIR)/farend_buffer.c
#
EXTRA_DEF =
ifeq ($(CONFIG_ENABLE_FREERTOS),y)
SRCS_C += $(SDIR)/cviaudio_kernel_mode.c
EXTRA_DEF += -DAUD_SUPPORT_KERNEL_MODE
else ifeq ($(USE_KERNEL_MODE_VQE), yes)
SRCS_C += $(SDIR)/cviaudio_kernel_mode.c
EXTRA_DEF += -DAUD_SUPPORT_KERNEL_MODE
endif

#
ODIR    := $(CURDIR)/obj
OBJS	:= $(SRCS_C:$(SDIR)/%.c=$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).o)
DEPS	:= $(SRCS_C:$(SDIR)/%.c=$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).d)
INCS = -I$(MW_INC) -I$(KERNEL_INC)

TARGET_A = $(MW_LIB)/libcvi_vqe.a
TARGET_SO = $(MW_LIB)/libcvi_vqe.so

INCS += -I./include -I../include -I../algo/aec/

ifeq ($(MULTI_PROCESS_SUPPORT_AUDIO), yes)
INCS += -I ../src/audio_rpc
EXTRA_DEF += -DRPC_MULTI_PROCESS_AUDIO
endif

ifeq ($(USE_CVI_AEC), no)
DEFS = -DEXTERNAL_AEC
else
DEFS += -DUSE_CVI_AEC
endif

ifeq ($(USE_CVIAUDIO_STATIC), yes)
DEFS += -DCVIAUDIO_STATIC
endif

ifeq ($(USE_CVIAUDIO_RTOS_SSP), yes)
DEFS += -DCVIAUDIO_RTOS_SSP
endif

ifeq ($(USE_CVI_AEC), no)
#LDLIB += -laec.v1.0.4
endif

ifeq ($(USE_CVITEK_SOFTWARE_AEC), yes)
DEFS += -DCVIAUDIO_SOFTWARE_AEC
endif

EXTRA_CFLAGS = $(DEFS) $(INCS)
#EXTRA_LDFLAGS = -L$(MW_LIB) -lcvi_AEC $(LDLIB)
EXTRA_LDFLAGS = -L$(MW_LIB) $(LDLIB)
#CFLAGS +=  -Werror
CFLAGS:=$(filter-out -Werror,$(CFLAGS))
#CFLAGS += -Wattributes
.PHONY : clean all
all : $(TARGET_A) $(TARGET_SO)

$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).o : $(SDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) $(EXTRA_DEF) -o $@ -c $<
	@echo [$(notdir $(CC))] $(notdir $@)

$(TARGET_A): $(OBJS)
	@$(AR) $(ARFLAGS) $@ $(OBJS) $(LIB_A_FLAGS)
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $@)

$(TARGET_SO): $(OBJS)
	@$(LD) $(LDFLAGS) $(EXTRA_LDFLAGS) -o $@ --start-group $(OBJS) --end-group
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $@)

clean:
	@rm -f $(TARGET_A) $(TARGET_SO)

clean_all: clean
	@if [ -d "$(SDIR)" ]; then \
		echo "rm -rf $(ODIR)"; \
		rm -rf $(ODIR); \
	fi

-include $(DEPS)
