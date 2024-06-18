SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../../../../$(shell echo $(MW_VER))/Makefile.param
	include $(PARAM_FILE)
endif

isp_chip_name := $(shell echo $(CHIP_ARCH) | tr A-Z a-z)
ISP_BASE_PATH = $(PWD)/../../$(isp_chip_name)

SDIR = $(PWD)
SRCS = $(wildcard $(SDIR)/*.c)

INCS = -I$(MW_INC) -I$(ISP_INC) -I$(KERNEL_INC) -I$(SYS_INC) -I$(VPU_INC) -I../../algo

# SOC Architecture
ifeq ($(CHIP_ARCH), CV183X)
	INCS += -I$(ISP_BASE_PATH)/include
	DEFS += -DCHIP_ARCH_CV183X
else ifeq ($(CHIP_ARCH), CV182X)
	INCS += -I$(ISP_BASE_PATH)/isp/inc
	DEFS += -DCHIP_ARCH_CV182X
else ifeq ($(CHIP_ARCH), CV181X)
	INCS += -I$(ISP_BASE_PATH)/isp/inc
	DEFS += -DCHIP_ARCH_CV181X
else ifeq ($(CHIP_ARCH), CV180X)
	INCS += -I$(ISP_BASE_PATH)/isp/inc
	DEFS += -DCHIP_ARCH_CV180X
else
	$(error Un-support SOC Architecture)
endif

OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)
TARGET_A = $(MW_LIB)/libraw_replay.a
TARGET_SO = $(MW_LIB)/libraw_replay.so

EXTRA_CFLAGS = $(INCS)
EXTRA_CFLAGS += $(DEFS)

.PHONY: clean all sdk_release
ifeq ($(ISP_SRC_RELEASE),1)
all: $(TARGET_A) $(TARGET_SO) sdk_release
else
all: $(TARGET_A) $(TARGET_SO)
endif

$(SDIR)/%.o: $(SDIR)/%.c
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
	-@rm -f *.c
	-@mv Makefile.sdk_release Makefile

clean:
	@rm -f $(OBJS) $(DEPS) $(TARGET_A) $(TARGET_SO)

-include $(DEPS)
