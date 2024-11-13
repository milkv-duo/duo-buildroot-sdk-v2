SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../../../mpi_param.mk
	include $(PARAM_FILE)
endif

#
SDIR := $(CURDIR)
SRCS_C += $(SDIR)/ae_buf.c
SRCS_C += $(SDIR)/ae_debug.c
SRCS_C += $(SDIR)/ae_detect_flicker.c
SRCS_C += $(SDIR)/ae_iris.c
SRCS_C += $(SDIR)/aealgo.c
SRCS_C += $(SDIR)/cvi_ae.c
#
ODIR    := $(CURDIR)/obj
OBJS	:= $(SRCS_C:$(SDIR)/%.c=$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).o)
DEPS	:= $(SRCS_C:$(SDIR)/%.c=$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).d)
#
INCS = -I$(MW_INC) -I$(ISP_INC) -I$(SYS_INC)

ifneq ($(findstring $(CHIP_ARCH), CV181X cv181x),)
INCS += -I../../cv181x/isp/inc
INCS += -I../../cv181x/isp/src
INCS += -I../../cv181x/isp_algo/inc
else ifneq ($(findstring $(CHIP_ARCH), CV180X cv180x),)
INCS += -I../../cv180x/isp/inc
INCS += -I../../cv180x/isp/src
INCS += -I../../cv180x/isp_algo/inc
endif
INCS += -I../../common/clog

TARGET_A = $(MW_LIB)/libae.a
TARGET_SO = $(MW_LIB)/libae.so

EXTRA_CFLAGS = $(INCS)


.PHONY: all clean sdk_release
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
	-@find . -name "*.h" -not -name "ae_debug.h" -exec rm -f {} \;
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
