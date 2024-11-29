SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../../../mpi_param.mk
	include $(PARAM_FILE)
endif

#
SDIR	:= $(CURDIR)
SRCS_C	:= $(SDIR)/sample_af.c
#
ODIR    := $(CURDIR)/obj
OBJS	:= $(SRCS_C:$(SDIR)/%.c=$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).o)
DEPS	:= $(SRCS_C:$(SDIR)/%.c=$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).d)
#
INCS = -I$(MW_INC) -I$(ISP_INC) -I$(SYS_INC) -I../

ifneq ($(findstring $(CHIP_ARCH), CV181X cv181x),)
INCS += -I../../cv181x/include
else ifneq ($(findstring $(CHIP_ARCH), CV180X cv180x),)
INCS += -I../../cv180x/isp/inc
else ifneq ($(findstring $(CHIP_ARCH), CV181X cv181x),)
INCS += -I../../cv181x/isp/inc
else ifneq ($(findstring $(CHIP_ARCH), CV180X cv180x),)
INCS += -I../../cv180x/isp/inc
endif

TARGET_A = $(MW_LIB)/libaf.a
TARGET_SO = $(MW_LIB)/libaf.so

EXTRA_CFLAGS = $(INCS)


.PHONY: clean all
all: $(TARGET_A) $(TARGET_SO)

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

clean:
	@rm -f $(TARGET_A) $(TARGET_SO)

clean_all: clean
	@if [ -d "$(SDIR)" ]; then \
		echo "rm -rf $(ODIR)"; \
		rm -rf $(ODIR); \
	fi

-include $(DEPS)
