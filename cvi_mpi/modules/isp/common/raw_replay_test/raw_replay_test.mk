SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../mpi_param.mk
	include $(PARAM_FILE)
endif

include ../self_test.mk
include ../../modules/isp/isp.mk

SDIR = $(CURDIR)
ISP_PATH := $(SDIR)/../../modules/isp/$(isp_chip_dir)
ISP_COMMON_PATH = $(SDIR)/../../modules/isp/common
ISP_3A_PATH = $(SDIR)/../../modules/isp/algo
RAW_REPLAY_SRC_DIR = $(ISP_COMMON_PATH)/raw_replay_test

SRCS = $(wildcard $(SDIR)/*.c) $(wildcard $(RAW_REPLAY_SRC_DIR)/*.c)

INCS = -I$(MW_INC) -I$(ISP_INC) -I$(COMM_INC) -I$(KERNEL_INC)

# SOC Architecture
ifeq ($(CHIP_ARCH), CV181X)
	INCS += -I$(ISP_PATH)/isp/inc
else
	$(error Un-support SOC Architecture)
endif

INCS += -I$(ISP_COMMON_PATH)/inc
INCS += -I$(ISP_COMMON_PATH)/raw_replay_test
INCS += -I$(ISP_3A_PATH)
INCS += -I$(SDIR)/../../modules/sys/include

OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)

TARGET = raw_replay_test
LIBS = -lvi -lvpss -lvo -lrgn -lgdc -lvenc -lvdec -lcvi_jpeg -lcvi_vcodec $(ISP_LIB) -lsys $(SNS_LIB) -lraw_replay

ifeq ($(MULTI_PROCESS_SUPPORT), 1)
DEFS += -DRPC_MULTI_PROCESS
LIBS += -lnanomsg
endif

EXTRA_CFLAGS = $(DEFS) $(INCS)
EXTRA_LDFLAGS = $(LIBS) -lini -lcvitracer -lpthread
EXTRA_LDFLAGS += -lsample -lcvi_bin -lcvi_bin_isp

ifeq ($(SELF_TEST_STATIC), 1)
ELFFLAGS += -static
endif

ifeq ($(DEBUG), 1)
EXTRA_LDFLAGS += -g -O0
endif

ifeq ($(MTRACE), y)
EXTRA_LDFLAGS += -g
endif

.PHONY : clean all
all : $(TARGET)

$(SDIR)/%.o: $(SDIR)/%.c
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<
	@echo [$(notdir $(CC))] $(notdir $@)

$(TARGET) : $(COMM_OBJ) $(OBJS)
	@$(CXX) -o $@ $(OBJS) $(COMM_OBJ) $(ELFFLAGS) $(EXTRA_LDFLAGS)
	@echo -e $(BLUE)[LINK]$(END)[$(notdir $(CXX))] $(notdir $@)

clean:
	@rm -f $(OBJS) $(DEPS) $(TARGET)

-include $(DEPS)
