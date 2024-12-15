# ----------------------------------------------------------------------
#
# Project: cvi_audio lib release and sample code exe bin
#
# ----------------------------------------------------------------------
SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
PARAM_FILE:=../../mpi_param.mk
include $(PARAM_FILE)
endif
#
AUDIO_PARAM_FILE:=./audio.mk
include $(AUDIO_PARAM_FILE)
#
DEBUG ?= 1
OS ?= linux
DEFINES =
DESTDIR ?= ./install/lib
INSTALL_LIBDIR = $(MW_LIB)
INSTALL_3rdLIBDIR = $(MW_3RD_LIB)
#
ifeq ($(TARGET_MACHINE), )
$(error "TARGET_MACHINE is null, please check!")
endif
#
ifeq ($(TARGET_MACHINE), aarch64-linux-gnu)
else ifeq ($(TARGET_MACHINE), riscv64-unknown-linux-gnu)
DEFINES += -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE
else ifeq ($(TARGET_MACHINE), riscv64-unknown-linux-musl)
DEFINES += -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE
else
DEFINES += -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE
endif
PREBUILD_LIBDIR = prebuilt/$(TARGET_MACHINE)

ifeq ($(USE_ALSA), yes)
PREBUILD_LIBDIR_ALSA ?= $(PREBUILD_LIBDIR)/alsa_lib
else  ifeq ($(USE_TINYALSA), yes)
PREBUILD_LIBDIR_ALSA ?= $(PREBUILD_LIBDIR)/tinyalsa_lib
endif

ifeq ($(USE_ALGO), yes)
DEFINES += -DUSE_ALGO_SOURCE
INCLUDES += -I./include
endif

ifeq ($(USE_AiMultiMic), yes)
DEFINES += -DAiMultiMic
endif

ifeq ($(USE_CYCLEBUFFER), yes)
DEFINES += -DUSE_CYCLE_BUFFER
endif

ifeq ($(USE_NEON_SIMD), yes)
DEFINES += -DNEON_OPT
endif

ifeq ($(USE_SYSTRACE), yes)
DEFINES += -DENABLE_FUNCTION_SYSTRACE
endif

ifeq ($(USE_CVI_AEC), no)
DEFINES += -DEXTERNAL_AEC
else
DEFINES += -DUSE_CVI_AEC
endif

ifeq ($(USE_ALSA), yes)
DEFINES += -DUSE_ALSA
endif

ifeq ($(USE_SYS_GLOBAL_LOG), yes)
DEFINES += -DAUDIO_PRINT_WITH_GLOBAL_COMM_LOG
endif

ifeq ($(USE_AUDIN_DEFAULT_PSIZE), yes)
DEFINES += -DUSE_AUDIN_DEFAULT_PSIZE
endif

#$(shell cp sample_v2/component_list_all.h sample_v2/component/component_list.h)
ifeq ($(OS),linux)
ifeq ($(PRODUCTFORM),soc)
BUILD_CONFIGURATION = EmbeddedLinux
else # pcie
BUILD_CONFIGURATION = NativeLinux
endif
endif

ifeq ($(OS),none)
ifeq ($(MAKECMDGOALS), test)
$(error test is only supported in linux environment)
endif
BUILD_CONFIGURATION = NonOS
endif

SLIBNAME = libcvi_audio.so
LIBAUDIO_A=$(INSTALL_LIBDIR)/libcvi_audio.a
LIBAUDIO_SO=$(INSTALL_LIBDIR)/libcvi_audio.so
$(info, "cvi_audio platform EmbeddedLinux")

INCLUDES = -I./include -I$(MW_INC)

INCLUDES += -I $(MW_PATH)/sample/common/ -I$(KERNEL_INC)
INCLUDES += -I $(MW_PATH)/modules/audio/src/


ALGOINCLUDES =
ALGOINCLUDES += -I./include -I./../../include
ALGOINCLUDES += -I$(KERNEL_INC) -I$(SYS_INC)
CVI_NR_INCLUDES =
CVI_NR_INCLUDES += -I./include
CVI_NR_INCLUDES += -I./include -I./../../include

RES_INCLUDES =
RES_INCLUDES +=  -I ./include -I ../../include

BD_INCLUDES =
BD_INCLUDES += $(INCLUDES)

ifeq ($(USE_CVI_AEC), yes)
AEC_INCLUDES += $(ALGOINCLUDES)
endif

EXTRA_CFLAGS = $(AEC_INCLUDES) $(DEFINES)

RES_CFLAGS += -I . -fPIC -Wno-unused-result  -Wno-format -Wl,--fatal-warning $(RES_INCLUDES) $(DEFINES)
BD_CFLAGS += -I . -fPIC -Wno-unused-result  -Wno-format -Wl,--fatal-warning $(BD_INCLUDES) $(DEFINES)
TMP_LDFLAGS =

ifeq ($(USE_ALSA), yes)
EXTRA_LDFLAGS += -L$(PREBUILD_LIBDIR)
EXTRA_LDFLAGS += -L$(PREBUILD_LIBDIR)/alsa_lib
endif

ifeq ($(USE_TINYALSA), yes)
EXTRA_LDFLAGS += -L$(PREBUILD_LIBDIR)
EXTRA_LDFLAGS += -L$(PREBUILD_LIBDIR)/tinyalsa_lib
endif

ifeq ($(USE_CVI_AEC), yes)
#EXTRA_LDFLAGS += -L$(PREBUILD_LIBDIR)/aec
endif

EXTRA_LDFLAGS += -L$(INSTALL_LIBDIR)
EXTRA_LDFLAGS += -L$(PREBUILD_LIBDIR)/cvi_resample

RES_LDFLAGS =
RES_LDFLAGS += -L$(PREBUILD_LIBDIR)/cvi_resample
RES_LDFLAGS += -L$(INSTALL_LIBDIR)

BD_LDFLAGS =
BD_LDFLAGS += -L$(INSTALL_LIBDIR)

LDLIBS = -lcvi_audio
ifeq ($(USE_SYS_GLOBAL_LOG), yes)
LDLIBS += -lsys
endif
TMP_LDLIBS =
AEC_LDLIBS =
ifeq ($(USE_CVI_AEC), no)
AEC_TEST_LDLIBS +=  -lfsp.v1.0.1.Release
else
AEC_TEST_LDLIBS += -laec
endif
RES_LDLIBS =

ifeq ($(USE_CVI_AEC), no)
LDLIBS += -lfsp.v1.0.1.Release
else
#LDLIBS += -laec
endif
LDLIBS += -lcvi_vqe
LDLIBS += -lcvi_VoiceEngine
TMP_LDLIBS  += -lcvi_VoiceEngine
LDLIBS += -lcvi_RES1
LDLIBS += -lrt

ifeq ($(USE_CVI_AEC), yes)
#LDLIBS += -laec
endif

ifeq ($(USE_ALSA), yes)
LDLIBS += -lasound
endif

ifeq ($(USE_TINYALSA), yes)
LDLIBS += -ltinyalsa
endif

ifeq ($(USE_SONIC_LIB), yes)
EXTRA_CFLAGS += -DUSE_SONIC_LIB
endif

AEC_LDLIBS += -laec.v1.0.4
RES_LDLIBS += -lcvi_RES1

AEC_LDFLAGS = -L$(INSTALL_LIBDIR)

BUILDLIST=
ifeq ($(OS),linux)
BUILDLIST += libssp20220128
BUILDLIST += libdnssp
BUILDLIST += libDnVqe #this should be removed !

ifeq ($(USE_SONIC_LIB), yes)
BUILDLIST += libsonic
endif
BUILDLIST += $(LIBAUDIO_A)
BUILDLIST += $(LIBAUDIO_SO)
BUILDLIST += libvqe
BUILDLIST += libvoice_engine
BUILDLIST += libcvi_RES1
BUILDLIST += libaac
else
$(info, "[Error]OS not Linux ...No build list")
endif

AUDIO_SDIR := $(CURDIR)/src
AUDIO_SRCS = $(AUDIO_SDIR)/cvi_aud_internal.c
ifeq ($(USE_ALSA), yes)
AUDIO_SRCS += src/cvi_audio_interface.c
endif
ifeq ($(USE_TINYALSA), yes)
AUDIO_SRCS += $(AUDIO_SDIR)/cvi_audio_interface_tinyalsa.c
AUDIO_SRCS += $(AUDIO_SDIR)/cvi_ao_internal.c
endif

#TO BE ADD for cviaudio_core.ko command
#AUDIO_SRCS += src/audio_kernel_mode/cviaudio_kernel_mode.c
ifeq ($(USE_CYCLEBUFFER), yes)
AUDIO_SRCS += $(AUDIO_SDIR)/cyclebuffer.c
endif
AUDIO_SRCS += $(AUDIO_SDIR)/cvi_audio_dec.c
AUDIO_SRCS += $(AUDIO_SDIR)/cvi_audio_enc.c
AUDIO_SRCS += $(AUDIO_SDIR)/cvi_audio_threads.c
AUDIO_SRCS += $(AUDIO_SDIR)/cvi_audio_common.c
AUDIO_SRCS += $(AUDIO_SDIR)/cvi_audio_fader.c
AUDIO_SRCS += $(AUDIO_SDIR)/sharebuffer.c
AUDIO_SRCS += $(AUDIO_SDIR)/volume_ctrl.c
AUDIO_SRCS += $(AUDIO_SDIR)/cvi_ai_internal.c

ifeq ($(USE_SPEED), yes)
AUDIO_SRCS += $(AUDIO_SDIR)/cvi_audio_speed.c
endif

ifeq ($(MULTI_PROCESS_SUPPORT_AUDIO), yes)
INCLUDES += -I ./$(AUDIO_SDIR)/audio_rpc
INCLUDES += -I ./$(AUDIO_SDIR)/audio_rpc/nanomsg/include
AUDIO_SRCS += ./$(AUDIO_SDIR)/audio_rpc/cvi_audio_rpc.c
EXTRA_CFLAGS += -DRPC_MULTI_PROCESS_AUDIO
LDLIBS += -lnanomsg
ifeq ($(USE_SHARE_MEM_IN_AUDIO_RPC), yes)
EXTRA_CFLAGS += -DSHARE_MEM_IN_AUDIO_RPC
AUDIO_SRCS += ./$(AUDIO_SDIR)/audio_rpc/cvi_audio_shm.c
endif
ifeq ($(TARGET_MACHINE), aarch64-linux-gnu)
EXTRA_LDFLAGS += -L $(MW_PATH)/3rdparty/nanomsg/lib_64
NANOMSG_PATH = $(MW_PATH)/3rdparty/nanomsg/lib_64
else ifeq ($(TARGET_MACHINE), riscv64-unknown-linux-musl)
EXTRA_LDFLAGS += -L $(MW_PATH)/3rdparty/nanomsg/musl_riscv64
NANOMSG_PATH = $(MW_PATH)/3rdparty/nanomsg/musl_riscv64
else
EXTRA_LDFLAGS += -L $(MW_PATH)/3rdparty/nanomsg/lib_32
NANOMSG_PATH = $(MW_PATH)/3rdparty/nanomsg/lib_32
endif
endif


ifeq ($(USE_CVITEK_SOFTWARE_AEC), yes)
EXTRA_CFLAGS += -DCVIAUDIO_SOFTWARE_AEC
endif

ifeq ($(AUD_USE_32BIT_WIDTH), yes)
EXTRA_CFLAGS += -DUSE_32_BIT_WIDTH
endif

ifeq ($(USE_PDM_AEC), yes)
EXTRA_CFLAGS += -DSUPPORT_PDM_AEC
endif

AUDIO_ODIR := $(CURDIR)/obj
AUDIO_OBJS	:= $(AUDIO_SRCS:$(AUDIO_SDIR)/%.c=$(AUDIO_ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).o)
AUDIO_DEPS	:= $(AUDIO_SRCS:$(AUDIO_SDIR)/%.c=$(AUDIO_ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).d)

ifeq ($(DEBUG), 0)
CFLAGS +=
else ifeq ($(DEBUG), 1)
CFLAGS += -g -O0
endif
CFLAGS  += -I. -fPIC -Wno-unused-result -Wno-implicit-function-declaration -Wno-format -Wl,--fatal-warning $(INCLUDES) $(DEFINES)

ts := $(shell /bin/date "+%Y-%m-%d---%H-%M-%S")

.PHONY: clean all install clean_all
all: prepare $(BUILDLIST) install

$(AUDIO_ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).o : $(AUDIO_SDIR)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) $(EXTRA_DEF) -o $@ -c $<
	@echo [$(notdir $(CC))] $(notdir $@)

$(LIBAUDIO_A): $(AUDIO_OBJS)
	@$(AR) $(ARFLAGS) $@ $(AUDIO_OBJS)
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $@)

$(LIBAUDIO_SO): $(AUDIO_OBJS)
	@$(LD) $(LDFLAGS) $(EXTRA_LDFLAGS) -o $@ --start-group $(AUDIO_OBJS) --end-group
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $@)


libvoice_engine:
	@cd ./audio_codec; make all

libvqe:
	@cd ./audio_vqe; make all

libDnVqe:
	@cd ./audio_dnvqe; make clean; make all

libcvi_RES1:
	@cd ./audio_resample; make clean; make all

libssp20220128:
	@echo "build libssp version 20220128"
	@cd ./algo/SSP_Algorithm_20220128; make clean; make all

libdnssp:
	@echo "build libssp2 version 20231218"
	@cd ./algo/downlink; make clean; make all

libsonic:
	@echo "build libsonic"
	@cd ./algo/sonic; make clean; make all

libaac:
	@echo "build fakeaac"
	@cd ./fdk_aac_fake; make clean; make all

install:
	@echo "[audio step]COPY LIB!! "
	@echo "[audio insatll>>>>] copy lib to:" $(INSTALL_LIBDIR)
	@cp -rf $(PREBUILD_LIBDIR_ALSA)/* $(INSTALL_LIBDIR)/
#	@cp $(NANOMSG_PATH)/* $(INSTALL_3rdLIBDIR)/
	@(/bin/date "+%Y-%m-%d---%H-%M-%S")
	@echo "[audio step]build lib finished!! "

clean:
	@echo "[audio clean>>>>] remove lib from:" $(MW_LIB)
	@rm -f $(TARGET)
	@rm -f $(LIBAUDIO_A)
	@rm -f $(LIBAUDIO_SO)
	@rm -f $(MW_INC)/cvi_transcode_interface.h
#	@rm -f $(INSTALL_3rdLIBDIR)/libnanomsg*
	@rm -f $(INSTALL_LIBDIR)/libcvi_RES*
	@rm -f $(INSTALL_LIBDIR)/libcvi_ssp*
	@rm -f $(INSTALL_LIBDIR)/libdnvqe*
	@rm -f $(INSTALL_LIBDIR)/libtinyalsa*
	@make -C ./fdk_aac_fake clean
	@cd ./src/audio_rpc; rm -f *.o
	@make -C ./audio_codec clean
	@make -C ./audio_vqe clean
	@make -C ./algo/SSP_Algorithm_20220128 clean
	@make -C ./audio_dnvqe clean
	@make -C ./algo/sonic clean
	@make -C ./audio_resample clean

clean_all: clean
	@rm -rf $(AUDIO_ODIR)
	@make -C ./fdk_aac_fake clean_all
	@make -C ./audio_codec clean_all
	@make -C ./audio_vqe clean_all
	@make -C ./algo/SSP_Algorithm_20220128 clean_all
	@make -C ./algo/downlink clean_all
	@make -C ./audio_dnvqe clean_all
	@make -C ./audio_resample clean_all

prepare:
	@echo [aduio Start] Timestamp is $(ts)
	@echo $(KERNEL_INC)
	@mkdir -p $(INSTALL_LIBDIR)

-include $(AUDIO_DEPS)
