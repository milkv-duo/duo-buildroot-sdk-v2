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
SRCS_C	:= $(SDIR)/cvi_audio_dnvqe.c
SRCS_C	+= $(SDIR)/cvi_audio_dnvqe_adp.c
#
ODIR    := $(CURDIR)/obj
OBJS	:= $(SRCS_C:$(SDIR)/%.c=$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).o)
DEPS	:= $(SRCS_C:$(SDIR)/%.c=$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).d)
#
INCS = -I$(MW_INC) -I$(KERNEL_INC) -I$(AUD_INC) -I./include

TARGET_A = $(MW_LIB)/libcvi_dnvqe.a
TARGET_SO = $(MW_LIB)/libcvi_dnvqe.so

INCS += -I../../include
DEFS = -DUSE_CVI_AEC

ifeq ($(USE_CVIAUDIO_STATIC), yes)
DEFS += -DCVIAUDIO_STATIC
endif


EXTRA_CFLAGS = $(DEFS) $(INCS)
EXTRA_LDFLAGS = -L$(MW_LIB) $(LDLIB)

.PHONY : clean all
all : $(TARGET_A) $(TARGET_SO)

$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).o : $(SDIR)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<
	@echo [$(notdir $(CC))] $(notdir $@)

$(TARGET_A): $(OBJS)
	echo $(AUD_INC)
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
