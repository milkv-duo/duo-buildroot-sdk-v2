SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
PARAM_FILE:=../../../mpi_param.mk
include $(PARAM_FILE)
endif

#
SDIR	:= $(CURDIR)/src
SRCS_C	:= $(SDIR)/cvi_transcode_interface.c
SRCS_C	+= $(SDIR)/g711.c
SRCS_C	+= $(SDIR)/g726.c
SRCS_C	+= $(SDIR)/ima_adpcm.c
#
ODIR    := $(CURDIR)/obj
OBJS	:= $(SRCS_C:$(SDIR)/%.c=$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).o)
DEPS	:= $(SRCS_C:$(SDIR)/%.c=$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).d)
#
INCS = -I./include -I$(MW_INC) -I$(KERNEL_INC)
DEFS = -DUSE_ALGO_SOURCE

TARGET_A = $(MW_LIB)/libcvi_VoiceEngine.a
TARGET_SO = $(MW_LIB)/libcvi_VoiceEngine.so

EXTRA_CFLAGS = $(INCS) $(DEFS)
EXTRA_LDFLAGS =

.PHONY : clean all
all : $(OBJS) $(TARGET_A) $(TARGET_SO)

$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).o : $(SDIR)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<
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

