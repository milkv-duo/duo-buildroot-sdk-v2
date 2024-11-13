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
SRCS_C	:= $(SDIR)/cvi_audio_resampler_interface.c
#
ODIR    := $(CURDIR)/obj
OBJS	:= $(SRCS_C:$(SDIR)/%.c=$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).o)
DEPS	:= $(SRCS_C:$(SDIR)/%.c=$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).d)
#
INCS = -I$(MW_INC) -I$(KERNEL_INC)
INCS += -I../include

TARGET_A = $(MW_LIB)/libcvi_RES1.a
TARGET_SO = $(MW_LIB)/libcvi_RES1.so

ARFLAGS = rcs
ARFLAGS_S = -shared -fPIC -o
CFLAGS =  -O3 -fPIC $(OPT_LEVEL)
CFLAGS += -D__FILENAME__=\"$(notdir $<)\"
EXTRA_CFLAGS = $(INCS) $(DEFS)

.PHONY : all clean clean_all
all : $(TARGET_A) $(TARGET_SO)

$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).o : $(SDIR)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -c $<  -o $@
	@echo [$(notdir $(CC))] $(notdir $@)

$(TARGET_A): $(OBJS)
	@$(AR) $(ARFLAGS) $@ $(OBJS)
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


