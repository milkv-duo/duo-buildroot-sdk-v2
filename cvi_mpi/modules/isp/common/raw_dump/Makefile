SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../../../mpi_param.mk
	include $(PARAM_FILE)
endif

isp_chip_name := $(shell echo $(CHIP_ARCH) | tr A-Z a-z)
ISP_BASE_PATH = $(CURDIR)/../../$(isp_chip_name)
#
SDIR := $(CURDIR)/src
SRCS_C := $(SDIR)/raw_dump.c
SRCS_C += $(SDIR)/remote.c
#
ODIR    := $(CURDIR)/obj
OBJS	:= $(SRCS_C:$(SDIR)/%.c=$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).o)
DEPS	:= $(SRCS_C:$(SDIR)/%.c=$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).d)
#
INCS = -I$(MW_INC) -I$(ISP_INC) -I$(SYS_INC) -I./inc -I./src -I../../algo

TARGET_A = $(MW_LIB)/libraw_dump.a
TARGET_SO = $(MW_LIB)/libraw_dump.so

LOCAL_CFLAGS = $(INCS)
RELEASE_FOLDER = $(CURDIR)/bin

# SOC Architecture
ifeq ($(CHIP_ARCH), CV181X)
	LOCAL_CFLAGS += -I$(ISP_BASE_PATH)/isp/inc
else ifeq ($(CHIP_ARCH), CV180X)
	LOCAL_CFLAGS += -I$(ISP_BASE_PATH)/isp/inc
else
	$(error Un-support SOC Architecture)
endif

.PHONY: clean all cleanall sdk_release
ifeq ($(ISP_SRC_RELEASE),1)
all: $(TARGET_A) $(TARGET_SO) sdk_release
else
all: $(TARGET_A) $(TARGET_SO)
endif

$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).o : $(SDIR)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(LOCAL_CFLAGS) -c $< -o $@
	@echo [$(notdir $(CC))] $(notdir $@)

$(TARGET_A): $(OBJS)
	@$(AR) $(ARFLAGS) $@ $(OBJS)
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $@)

$(TARGET_SO): $(OBJS)
	@$(LD) $(LDFLAGS) -o $@ --start-group $(OBJS) $(LIBS) --end-group
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $@)

sdk_release: $(TARGET_A) $(TARGET_SO)
	@echo "Create $(RELEASE_FOLDER)"
	-@mkdir -p $(RELEASE_FOLDER)
	-@rm -rf $(RELEASE_FOLDER)/*
	-@cp $(TARGET_A) $(RELEASE_FOLDER)
	-@cp $(TARGET_SO) $(RELEASE_FOLDER)
	-@rm -rf src
	-@rm -rf $(TMP_FOLDER)
	-@mv Makefile.sdk_release Makefile

clean:
	@rm -f $(TARGET_A) $(TARGET_SO)
	@rm -rf $(RELEASE_FOLDER)

clean_all: clean
	@if [ -d "$(SDIR)" ]; then \
		echo "rm -rf $(ODIR)"; \
		rm -rf $(ODIR); \
	fi

cleanall: clean_all

-include $(DEPS)
