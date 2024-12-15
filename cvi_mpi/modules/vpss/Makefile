SHELL = /bin/bash
#
ifeq ($(PARAM_FILE), )
     PARAM_FILE:=../../mpi_param.mk
     include $(PARAM_FILE)
endif
#
SDIR = $(CURDIR)/src
SRCS_C := $(SDIR)/cvi_vpss.c
SRCS_C += $(SDIR)/vpss_ioctl.c
#
ODIR := $(CURDIR)/obj
OBJS := $(SRCS_C:$(SDIR)/%.c=$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).o)
DEPS := $(SRCS_C:.c=.d)
#
INCS = -I$(MW_INC) -I$(ISP_INC) -I$(SYS_INC) -I$(BIN_INC) -I$(VPSS_INC) -I$(GDC_INC) -I$(KERNEL_INC)
#
TARGET_A = $(MW_LIB)/libvpss.a
TARGET_SO = $(MW_LIB)/libvpss.so
#
EXTRA_CFLAGS = $(INCS)


.PHONY : clean all
all : $(TARGET_A) $(TARGET_SO)

$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).o : $(SDIR)/%.c
	$(Q)mkdir -p $(dir $@)
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -c $< -o $@
	@echo [$(notdir $(CC))] $(notdir $@)

$(TARGET_A): $(OBJS)
	@$(AR) $(ARFLAGS) $(TARGET_A) $(OBJS)
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $(TARGET_A))

$(TARGET_SO): $(OBJS)
	@$(LD) $(LDFLAGS) -o $(TARGET_SO) --start-group $(OBJS) --end-group
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $(TARGET_SO))

clean:
	@rm -f $(TARGET_A) $(TARGET_SO)

clean_all: clean
	@if [ -d "$(SDIR)" ]; then \
		echo "rm -rf $(ODIR)"; \
		rm -rf $(ODIR); \
	fi

-include $(DEPS)
