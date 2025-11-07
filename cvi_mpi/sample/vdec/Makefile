SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../mpi_param.mk
	include $(PARAM_FILE)
endif
include ../sample.mk

SDIR = $(CURDIR)
SRCS = $(wildcard $(SDIR)/../venc/src/*.c) $(wildcard $(SDIR)/src/*.c)
SRCS += $(SDIR)/../common/sample_common_vdec.c	\
		$(SDIR)/../common/sample_common_sys.c	\
		$(SDIR)/../common/sample_common_venc.c	\
		$(SDIR)/../common/sample_common_platform.c	\
		$(SDIR)/../common/md5sum.c	\
		$(SDIR)/../common/sample_common_sensor.c	\
		$(SDIR)/../common/sample_common_vi.c	\
		$(SDIR)/../common/sample_common_vpss.c	\
		$(SDIR)/../common/sample_common_vo.c	\
		$(SDIR)/../common/sample_common_isp.c
ifeq ($(CONFIG_ENABLE_SDK_ASAN), y)
SRCS += $(SDIR)/../common/sample_common_bin.c	\
		$(SDIR)/../common/sample_common_peripheral.c
endif
VDEC_SRCS = $(SDIR)/sample_vdec.c
VDEC_INC=../../modules/vdec
SAMPLE_VENC_INC=$(SDIR)/../venc/include
SAMPLE_VDEC_INC=$(SDIR)/include

INCS = -I$(MW_INC) -I$(ISP_INC) -I$(COMM_INC) -I$(SAMPLE_VDEC_INC) -I$(SAMPLE_VENC_INC) -I$(VDEC_INC) -I$(KERNEL_INC) -I../common -I$(SYS_INC) -I$(SENSOR_LIST_INC) -I$(PANEL_INC) -I$(MW_PATH)/3rdparty/inih

OBJS = $(SRCS:.c=.o)
VDEC_OBJS = $(VDEC_SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)
VDEC_DEPS = $(VDEC_SRCS:.c=.d)

TARGET = sample_vdec

ifeq ($(TARGET_MACHINE),arm-linux-gnueabihf)
#TARGET_WITH_ASAN = sample_vdec_asan
else ifeq ($(TARGET_MACHINE),aarch64-linux-gnu)
#TARGET_WITH_ASAN = sample_vdec_asan
else
TARGET_WITH_ASAN =
endif

PKG_CONFIG_PATH = $(MW_PATH)/pkgconfig
REQUIRES = cvi_common cvi_vdec cvi_sample
LIBS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config  --define-variable=mw_dir=$(MW_PATH) $(REQUIRES))

LIBS += -lvdec -lmisc -lsys -lvi -lvpss -lvo  -lgdc -lvenc -lcvi_bin -lcvi_bin_isp -lisp -lisp_algo -lsns_full -lae -laf -lawb -latomic
ifeq ($(MULTI_PROCESS_SUPPORT), 1)
DEFS += -DRPC_MULTI_PROCESS
LIBS += -lnanomsg
endif

EXTRA_CFLAGS = $(DEFS) $(INCS)
EXTRA_LDFLAGS = $(LIBS) -lini -lm -lpthread


EXTRA_LDFLAGS_WITH_ASAN = -Wl,-Bdynamic $(LIBS) -lini -lm -lpthread
EXTRA_LDFLAGS_WITH_ASAN +=  -fno-omit-frame-pointer -fsanitize-recover=address -lasan

.PHONY : clean all
all: $(TARGET) $(TARGET_WITH_ASAN)

$(SDIR)/%.o: $(SDIR)/%.c
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<
	@echo [$(notdir $(CC))] $(notdir $@)

$(TARGET): $(VDEC_OBJS) $(OBJS) $(MW_LIB)/libvdec.a $(MW_LIB)/libsys.a
	@$(CXX) -o $@ -Wl,--start-group $(OBJS) $(VDEC_OBJS) -Wl,--end-group $(ELFFLAGS) $(EXTRA_LDFLAGS)
	@echo -e $(BLUE)[LINK]$(END)[$(notdir $(CXX))] $(notdir $@)

$(TARGET_WITH_ASAN): $(VDEC_OBJS) $(OBJS)
	@$(CXX) -o $@ -Wl,--start-group $(OBJS) $(VDEC_OBJS) -Wl,--end-group $(ELFFLAGS) $(EXTRA_LDFLAGS_WITH_ASAN)
	@echo -e $(BLUE)[LINK]$(END)[$(notdir $(CXX))] $(notdir $@)

clean:
	@rm -f $(VDEC_OBJS) $(OBJS) $(VDEC_DEPS) $(DEPS) $(TARGET) $(TARGET_WITH_ASAN)

-include $(VDEC_DEPS) $(DEPS)
