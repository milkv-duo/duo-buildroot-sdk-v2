SHELL = /bin/bash
#
ifeq ($(PARAM_FILE), )
     PARAM_FILE:=../../mpi_param.mk
     include $(PARAM_FILE)
endif
#
SDIR := $(CURDIR)/src
SRCS_C := $(SDIR)/cvi_venc.c
#
ODIR    := $(CURDIR)/obj
OBJS    := $(SRCS_C:$(SDIR)/%.c=$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).o)
DEPS	:= $(SRCS_C:.c=.d)
#
INCS = -I$(MW_INC) -I$(KERNEL_INC) -I$(SYS_INC) -Iinclude
#
TARGET_A = $(MW_LIB)/libvenc.a
TARGET_SO = $(MW_LIB)/libvenc.so
ifeq ($(MULTI_PROCESS_SUPPORT), 1)
DEFS += -DRPC_MULTI_PROCESS
endif

EXTRA_CFLAGS = $(INCS) $(DEFS)

ifeq ($(DEBUG), 1)
EXTRA_CFLAGS += -g -O0
endif

ldflags-y = -lsys

TMP_INC = $(MW_INC)/cvi_jpg* $(MW_INC)/cvi_h265*

.PHONY : clean all
all : $(TARGET_A) $(TARGET_SO)

$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).o : $(SDIR)/%.c
	$(Q)mkdir -p $(dir $@)
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -c $< -o $@
	@echo [$(notdir $(CC))] $(notdir $@)

$(TARGET_A): $(OBJS) $(LIB_A_FLAGS)
	@$(AR) $(ARFLAGS) $@ $(OBJS)
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $@)

$(TARGET_SO): $(OBJS) $(LIB_SO_FLAGS)
	@$(LD) $(LDFLAGS) $(ldflags-y) -o $@ --start-group $(OBJS) --end-group
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $@)

clean:
	@rm -f $(TARGET_A) $(TARGET_SO)

clean_all: clean
	@if [ -d "$(SDIR)" ]; then \
		echo "rm -rf $(ODIR)"; \
		rm -rf $(ODIR); \
	fi

-include $(DEPS)
