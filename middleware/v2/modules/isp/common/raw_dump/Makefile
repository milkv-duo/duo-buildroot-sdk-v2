SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../../../../$(shell echo $(MW_VER))/Makefile.param
	include $(PARAM_FILE)
endif

isp_chip_name := $(shell echo $(CHIP_ARCH) | tr A-Z a-z)
ISP_BASE_PATH = $(PWD)/../../$(isp_chip_name)

SDIR = $(PWD)/src
SRCS = $(wildcard $(SDIR)/*.c)

INCS = -I$(MW_INC) -I$(ISP_INC) -I$(KERNEL_INC) -I$(SYS_INC) -I./inc -I./src -I../../algo

OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)
TARGET_A = $(MW_LIB)/libraw_dump.a
TARGET_SO = $(MW_LIB)/libraw_dump.so

LOCAL_CFLAGS = $(INCS)
RELEASE_FOLDER = $(PWD)/bin

# SOC Architecture
ifeq ($(CHIP_ARCH), CV183X)
	LOCAL_CFLAGS += -I$(ISP_BASE_PATH)/include
	LOCAL_CFLAGS += -DCHIP_ARCH_CV183X
	DEFS += -DCHIP_ARCH_CV183X
else ifeq ($(CHIP_ARCH), CV182X)
	LOCAL_CFLAGS += -I$(ISP_BASE_PATH)/isp/inc
	LOCAL_CFLAGS += -DCHIP_ARCH_CV182X
	DEFS += -DCHIP_ARCH_CV182X
else ifeq ($(CHIP_ARCH), CV181X)
	LOCAL_CFLAGS += -I$(ISP_BASE_PATH)/isp/inc
	LOCAL_CFLAGS += -DCHIP_ARCH_CV181X
	DEFS += -DCHIP_ARCH_CV181X
else ifeq ($(CHIP_ARCH), CV180X)
	LOCAL_CFLAGS += -I$(ISP_BASE_PATH)/isp/inc
	LOCAL_CFLAGS += -DCHIP_ARCH_CV180X
	DEFS += -DCHIP_ARCH_CV180X
else
	$(error Un-support SOC Architecture)
endif

.PHONY: clean all cleanall sdk_release
ifeq ($(ISP_SRC_RELEASE),1)
all: $(TARGET_A) $(TARGET_SO) sdk_release
else
all: $(TARGET_A) $(TARGET_SO)
endif

$(SDIR)/%.o: $(SDIR)/%.c
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
	@rm -f $(OBJS) $(DEPS) $(TARGET_A) $(TARGET_SO)

cleanall: clean
	@rm -rf $(RELEASE_FOLDER)

-include $(DEPS)
