SHELL = /bin/bash

ifeq ("$(CHIP_ARCH)", "CV180X")
all:
clean:
else
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../mpi_param.mk
	include $(PARAM_FILE)
endif
include ../sample.mk

SDIR = $(CURDIR)
SRCS = $(wildcard $(SDIR)/*.c)
INCS = -I$(MW_INC) -I$(ISP_INC) -I$(KERNEL_INC) -I$(PANEL_INC) -I../common -I$(SENSOR_LIST_INC)
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)

TARGET = sample_panel
ifeq ($(CONFIG_ENABLE_SDK_ASAN), y)
TARGET = sample_panel_asan
endif

LIBS = -lvo -lsys -lmipi_tx -lmisc

ifeq ($(MULTI_PROCESS_SUPPORT), 1)
DEFS += -DRPC_MULTI_PROCESS
LIBS += -lnanomsg
LIBS += -lvenc -lvdec -lmisc
endif

EXTRA_CFLAGS = $(INCS)
EXTRA_LDFLAGS = $(LIBS) -lm -lpthread -latomic
EXTRA_LDFLAGS += -lcvi_bin -lcvi_bin_isp $(ISP_LIB)

.PHONY : clean all lt9611 clean_lt9611
all: $(TARGET) lt9611

$(SDIR)/%.o: $(SDIR)/%.c
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<
	@echo [$(notdir $(CC))] $(notdir $@)

$(TARGET): $(OBJS) $(MW_LIB)/libvo.a $(MW_LIB)/libsys.a $(MW_LIB)/libmipi_tx.a
	@$(CXX) -o $@ $(OBJS) $(ELFFLAGS) $(EXTRA_LDFLAGS)
	@echo -e $(BLUE)[LINK]$(END)[$(notdir $(CC))] $(notdir $@)

lt9611:
	cd $(SDIR)/lt9611;make clean;make

clean_lt9611:
	cd $(SDIR)/lt9611;make clean;

clean: clean_lt9611
	@rm -f $(OBJS) $(DEPS) $(TARGET)

-include $(DEPS)
endif
