SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
PARAM_FILE:=../../../../mpi_param.mk
include $(PARAM_FILE)
endif
#
AUDIO_PARAM_FILE:=../../audio.mk
include $(AUDIO_PARAM_FILE)
#
SDIR	:= $(CURDIR)/src
SRCS_C	:= $(SDIR)/cvi_dn_interface.c
SRCS_C	+= $(SDIR)/cvi_drc.c
SRCS_C	+= $(SDIR)/cvi_eq.c
SRCS_C	+= $(SDIR)/cvi_hpf.c
SRCS_C	+= $(SDIR)/cviaudio_dnalgo_interface.c
#
ODIR    := $(CURDIR)/obj
OBJS	:= $(SRCS_C:$(SDIR)/%.c=$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).o)
DEPS	:= $(SRCS_C:$(SDIR)/%.c=$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).d)
#
INCS = -I./include -I$(MW_INC) -I$(KERNEL_INC) -I$(AUD_INC)
TARGET_A = $(MW_LIB)/libcvi_ssp2.a
TARGET_SO = $(MW_LIB)/libcvi_ssp2.so

DEFS =
ifeq ($(USE_HEAP_REPLACE_STACK), yes)
DEFS += -DSSP_STACK_TO_HEAP
endif

LOCAL_CFLAGS =  -fgnu89-inline -Wno-unused-variable -Wuninitialized $(DEFS)
EXTRA_CFLAGS = $(INCS)
EXTRA_LDFLAGS =
#EXTRA_CFLAGS += -DFAST_MATH_CALC
.PHONY : clean all
all : $(TARGET_A) $(TARGET_SO)

$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).o : $(SDIR)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(LOCAL_CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<
	@echo [$(notdir $(CC))] $(notdir $@)

$(TARGET_A): $(OBJS)
	@$(AR) $(ARFLAGS) $@ $(OBJS)
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $@)

$(TARGET_SO): $(OBJS)
	@$(LD) $(LDFLAGS) $(EXTRA_LDFLAGS) -o $@ --start-group $(OBJS) --end-group
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $@)
#	@$(CC) $(DEPFLAGS) $(CFLAGS) $(LOCAL_CFLAGS) $(EXTRA_CFLAGS) ssp_main.c  -o ssp_Test0128  -lcvi_ssp128 -L$(MW_LIB) -lm -static

clean:
	@rm -f $(TARGET_A) $(TARGET_SO)

clean_all: clean
	@if [ -d "$(SDIR)" ]; then \
		echo "rm -rf $(ODIR)"; \
		rm -rf $(ODIR); \
	fi

-include $(DEPS)
