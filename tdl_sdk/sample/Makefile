CONFIRM_ENV_VAR = 1
ifneq (, $(filter install clean, $(MAKECMDGOALS)))
CONFIRM_ENV_VAR = 0
endif

ifeq ($(CONFIRM_ENV_VAR), 1)
ifndef MW_PATH
$(error "Please set middleware sdk root path to MW_PATH")
endif

ifeq ($(USE_TPU_IVE), ON)
ifndef IVE_PATH
$(error "Please set ive sdk root path to IVE_PATH)
endif
endif

ifndef TPU_PATH
$(error "Please set tpu sdk root path to TPU_PATH)
endif
endif

CHIP ?= cv1835
SDK_VER ?= 64bit

CROSS_COMPILE_32 ?= arm-linux-gnueabihf-
CROSS_COMPILE_64 ?= aarch64-linux-gnu-
CROSS_COMPILE_32_UCLIBC ?= arm-cvitek-linux-uclibcgnueabihf-
CROSS_COMPILE_RISCV64_GLIBC ?= riscv64-unknown-linux-gnu-
CROSS_COMPILE_RISCV64_MUSL ?= riscv64-unknown-linux-musl-

ifeq ($(SDK_VER), 32bit)
ARCH=arm
CROSS_COMPILE = $(CROSS_COMPILE_32)
CFLAGS += -mfloat-abi=hard -mfpu=neon-vfpv4 -march=armv7-a
else ifeq ($(SDK_VER), 64bit)
ARCH=arm64
CROSS_COMPILE = $(CROSS_COMPILE_64)
CFLAGS += -march=armv8-a
else ifeq ($(SDK_VER), uclibc)
ARCH=uclibc
CROSS_COMPILE = $(CROSS_COMPILE_32_UCLIBC)
CFLAGS += -mfloat-abi=hard -mfpu=neon-vfpv4 -march=armv7-a
else ifeq ($(SDK_VER), glibc_riscv64)
ARCH=riscv64
CROSS_COMPILE = $(CROSS_COMPILE_RISCV64_GLIBC)
CFLAGS += -march=rv64imafdcv0p7xthead -mabi=lp64d -mcpu=c906fdv
else ifeq ($(SDK_VER), musl_riscv64)
ARCH=riscv64
CROSS_COMPILE = $(CROSS_COMPILE_RISCV64_MUSL)
CFLAGS += -march=rv64imafdcv0p7xthead -mcpu=c906fdv
else
$(error "Unknown SDK_VER: $(SDK_VER")")
endif

#####################################################
# Gcc Compiler
#####################################################
CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
AR = $(CROSS_COMPILE)ar
LD = $(CROSS_COMPILE)ld
STRIP = $(CROSS_COMPILE)strip

ifeq ($(DEBUG), 1)
CFLAGS += -g -O0
else
CFLAGS += -O3 -DNDEBUG
endif

#####################################################
# Middleware stuffs
#####################################################
MW_LIB_PATH = $(MW_PATH)/lib
MW_INC_PATH = $(MW_PATH)/include
MW_PANEL_INC_PATH = ${MW_PATH}/include/panel
MW_SAMPLE_PATH = $(MW_PATH)/sample/common
MW_SENSOR_INC_PATH = $(MW_PATH)/component/isp/common
MW_3RD_LIB_PATH = $(MW_LIB_PATH)/3rd
ifeq ($(origin MW_PANEL_BOARD_INC_PATH), undefined)
  MW_PANEL_BOARD_INC_PATH := $(MW_PATH)/include
endif

MW_AUDIO_LIBS = -lcvi_audio -lcvi_vqe -lcvi_ssp -lcvi_dnvqe -lcvi_ssp2 -ltinyalsa -lcvi_VoiceEngine -lcvi_RES1


ifeq ($(CHIP), CV186X)
	MW_LIBS = -lsys -lvi -lvpss -lvenc -lvdec -lvo -lrgn -lgdc -lini -lisp -lsns_full -lawb -lae -laf -lcvi_bin_isp -lcvi_bin -lisp_algo
	MW_ISP_INC_PATH = $(MW_INC_PATH)/isp/cv186x/
	MW_LINUX_INC_PATH = $(KERNEL_ROOT)/include/
	MW_PANEL_BOARD_INC_PATH = ${MW_PATH}/component/panel/cv186x
	CFLAGS += -DCV186X
	SAMPLE_COMMON_FILE = ${MW_SAMPLE_PATH}/sample_common_vo.c \
		 ${MW_SAMPLE_PATH}/sample_common_sys.c \
		 ${MW_SAMPLE_PATH}/sample_common_platform.c \
		 ${MW_SAMPLE_PATH}/sample_common_vpss.c \
		 ${MW_SAMPLE_PATH}/sample_common_vi.c \
		 ${MW_SAMPLE_PATH}/sample_common_isp.c \
		 ${MW_SAMPLE_PATH}/sample_common_bin.c \
		 ${MW_SAMPLE_PATH}/sample_common_venc.c \
		 ${MW_SAMPLE_PATH}/sample_common_peripheral.c
else ifeq ($(CHIP), CV181X)
	MW_LIBS = -lini -lsns_full -lsample -lisp -lvdec -lvenc -lawb -lae -laf -lcvi_bin -lcvi_bin_isp -lmisc -lisp_algo -lsys  -lvi -lvo -lvpss -lrgn -lgdc
	MW_ISP_INC_PATH = $(MW_INC_PATH)/isp/cv181x/
	MW_LINUX_INC_PATH = $(KERNEL_ROOT)/include/
	CFLAGS += -DCV181X -D__CV181X__
else ifeq ($(CHIP), CV180X)
	MW_LIBS = -lini -lsns_full -lsample -lisp -lvdec -lvenc -lawb -lae -laf -lcvi_bin -lcvi_bin_isp -lmisc -lisp_algo -lsys  -lvi -lvo -lvpss -lrgn -lgdc
	MW_ISP_INC_PATH = $(MW_INC_PATH)/isp/cv180x
	MW_LINUX_INC_PATH = $(KERNEL_ROOT)/include/
	CFLAGS += -DCV180X -D__CV180X__
endif

#####################################################
# TDL SDK stuffs
#####################################################
SDK_ROOT_PATH ?= $(abspath ../..)
SDK_LIB_PATH = $(SDK_ROOT_PATH)/lib
SDK_INC_PATH = $(SDK_ROOT_PATH)/include
SDK_TDL_INC_PATH = $(SDK_ROOT_PATH)/include/cvi_tdl
SDK_APP_INC_PATH = $(SDK_ROOT_PATH)/include/cvi_tdl_app
SDK_SAMPLE_INC_PATH = $(SDK_ROOT_PATH)/sample/3rd/include
SDK_3RD_LIB_PATH = $(SDK_ROOT_PATH)/sample/3rd/lib
SDK_SAMPLE_UTILS_PATH = $(SDK_ROOT_PATH)/sample/utils
SDK_TDL_LIBS = -lcvi_tdl
SDK_APP_LIBS = -lcvi_tdl_app

#####################################################
# Opencv
#####################################################
OPENCV_INC_PATH = $(SDK_ROOT_PATH)/sample/3rd/opencv/include
OPENCV_LIB_PATH = $(SDK_ROOT_PATH)/sample/3rd/opencv/lib
OPENCV_LIBS = -lopencv_core -lopencv_imgproc -lopencv_imgcodecs

#####################################################
# TPU
#####################################################
ifeq ($(CHIP), CV186X)
TPU_LIB_PATH = $(TPU_PATH)/libsophon-0.4.9/lib/
TPU_LIBS = -lbmrt -lbmlib
else
TPU_LIB_PATH = $(TPU_PATH)/lib
TPU_LIBS = -lcnpy  -lcvikernel  -lcvimath  -lcviruntime  -lz -lm
endif

#####################################################
# IVE SDK
#####################################################
ifeq ($(USE_TPU_IVE), ON)
IVE_SDK_LIB_PATH = $(IVE_PATH)/lib
IVE_SDK_INC_PATH = $(IVE_PATH)/include
IVE_LIBS = -lcvi_ive_tpu
CFLAGS += -DUSE_TPU_IVE
else
IVE_SDK_LIB_PATH = $(MW_PATH)/lib
IVE_SDK_INC_PATH = $(MW_PATH)/include
IVE_LIBS = -lcvi_ive
endif

#####################################################
# RTSP stuffs
#####################################################
RTSP_LIB_PATH = $(SDK_ROOT_PATH)/sample/3rd/rtsp/lib
RTSP_INC_PATH = $(SDK_ROOT_PATH)/sample/3rd/rtsp/include/cvi_rtsp
RTSP_LIBS = -lcvi_rtsp

#####################################################
# STB
#####################################################
STB_INC_PATH = $(SDK_ROOT_PATH)/sample/3rd/stb/include

#####################################################
# CVI LIB stuffs
#####################################################
MD_INC_PATH = $(SDK_ROOT_PATH)/include/cvi_md
DRAW_RECT_INC_PATH = $(SDK_ROOT_PATH)/include/cvi_draw_rect
PREPROCESS_INC_PATH = $(SDK_ROOT_PATH)/include/cvi_preprocess

CFLAGS += -std=gnu11 -Wno-pointer-to-int-cast -fsigned-char -Wno-format-truncation -fdiagnostics-color=always -s -lpthread -latomic

SRCS := $(wildcard $(PWD)/*.c)
CPPS := $(wildcard $(PWD)/*.cpp)
OBJS := $(SRCS:%.c=%.o) $(CPPS:%.cpp=%.o)

SAMPLE_BASE_LIBS = -L$(TPU_LIB_PATH) $(TPU_LIBS) -L$(IVE_SDK_LIB_PATH) $(IVE_LIBS) -L$(SDK_LIB_PATH) $(SDK_TDL_LIBS)
SAMPLW_MW_LIB = -L$(MW_LIB_PATH) -L$(MW_3RD_LIB_PATH) $(MW_LIBS) -L$(SDK_3RD_LIB_PATH)
SAMPLE_OPENCV_LIB = -L$(OPENCV_LIB_PATH) $(OPENCV_LIBS)

SAMPLE_INIT_LIBS = $(SAMPLW_MW_LIB) $(SAMPLE_BASE_LIBS)
SAMPLE_AUD_LIBS = $(SAMPLW_MW_LIB) $(SAMPLE_BASE_LIBS)  $(MW_AUDIO_LIBS)  $(SAMPLE_OPENCV_LIB)
SAMPLE_YOLO_LIBS = $(SAMPLW_MW_LIB) $(SAMPLE_BASE_LIBS)  $(SAMPLE_OPENCV_LIB)
SAMPLE_VI_LIBS = $(SAMPLW_MW_LIB) $(SAMPLE_OPENCV_LIB) $(SAMPLE_BASE_LIBS) -L$(RTSP_LIB_PATH) $(RTSP_LIBS)
SAMPLE_APP_LIBS = $(SAMPLW_MW_LIB) $(SAMPLE_OPENCV_LIB) $(SAMPLE_BASE_LIBS) -L$(RTSP_LIB_PATH) $(RTSP_LIBS) $(SDK_APP_LIBS)

CFLAGS += -I$(SDK_INC_PATH) \
          -I$(SDK_TDL_INC_PATH) \
          -I$(SDK_APP_INC_PATH) \
          -I$(SDK_SAMPLE_INC_PATH) \
		  -I$(SDK_SAMPLE_UTILS_PATH) \
          -I$(RTSP_INC_PATH) \
          -I$(IVE_SDK_INC_PATH) \
          -I$(OPENCV_INC_PATH) \
          -I$(STB_INC_PATH) \
		  -I$(MW_SAMPLE_PATH) \
          -I$(MW_ISP_INC_PATH) \
		  -I$(MW_PANEL_INC_PATH) \
		  -I$(MW_PANEL_BOARD_INC_PATH) \
		  -I$(MW_SENSOR_INC_PATH) \
		  -I$(MW_LINUX_INC_PATH) \
		  -I$(MW_INC_PATH) \
		  -I$(MW_INC_PATH)/linux \
          -I$(AISDK_ROOT_PATH)/include/stb

ifeq ($(CONFIRM_ENV_VAR), 1)
$(info ---------------------------------------)
$(info CHIP: $(CHIP))
$(info SDK_VER: $(SDK_VER))
$(info KERNEL_ROOT: $(KERNEL_ROOT))
$(info MW_LINUX_INC_PATH: $(MW_LINUX_INC_PATH))
$(info TDL SDK library path: $(SDK_LIB_PATH))
$(info TDL SDK include path: $(SDK_INC_PATH))
$(info Middleware include path: $(MW_INC_PATH))
$(info Middleware library path: $(MW_LIB_PATH))
$(info IVE library path: $(IVE_SDK_LIB_PATH))
$(info IVE include path: $(IVE_SDK_INC_PATH))
$(info TPU library path: $(TPU_LIB_PATH))
$(info CFLAGS: $(CFLAGS))
$(info CC: $(CC))
$(info CXX: $(CXX))
$(info USE_TPU_IVE: $(USE_TPU_IVE))
$(info SAMPLW_MW_LIB: $(SAMPLW_MW_LIB))
$(info SAMPLE_AUD_LIBS: $(SAMPLE_AUD_LIBS))
$(info ---------------------------------------)
endif

TARGETS_SAMPLE_INIT := $(shell find . -type f -name 'sample_init.c' -exec basename {} .c ';')
TARGETS_VI_SAMPLE := $(shell find . -type f -name 'sample_vi_*.c' -exec basename {} .c ';')
TARGETS_AUDIO_SAMPLE := $(shell find . -type f -name 'sample_aud_*.c' -exec basename {} .c ';')
TARGETS_AUDIO_SAMPLE += $(shell find . -type f -name 'sample_aud_*.cpp' -exec basename {} .cpp ';')

TARGETS_IMAGE_SAMPLE := $(shell find . -type f -name 'sample_read_*.c' -exec basename {} .c ';')
TARGETS_IMAGE_SAMPLE += $(shell find . -type f -name 'sample_read_*.cpp' -exec basename {} .cpp ';')

TARGETS_APP_SAMPLE := $(shell find . -type f -name 'sample_app_*.c' -exec basename {} .c ';')
TARGETS_YOLO_SAMPLE := $(shell find . -type f -name 'sample_yolo*.cpp' -exec basename {} .cpp ';')

TARGETS = $(TARGETS_SAMPLE_INIT) \
  	      $(TARGETS_VI_SAMPLE) \
 	      $(TARGETS_AUDIO_SAMPLE) \
	      $(TARGETS_READ_SAMPLE) \
	      $(TARGETS_APP_SAMPLE) \
	      $(TARGETS_YOLO_SAMPLE)

.PHONY : all clean

all: $(TARGETS)

clean:
	rm -f $(OBJS) $(TARGETS)

$(PWD)/%.o: $(PWD)/%.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(PWD)/%.o: $(PWD)/%.cpp
	$(CXX) $(CFLAGS) -o $@ -c $<

$(SDK_ROOT_PATH)/sample/utils/%.o: $(SDK_ROOT_PATH)/sample/utils/%.c
	$(CC) $(CFLAGS) -o $@ -c $<

sample_init: $(PWD)/sample_init.o
	$(CC) $(CFLAGS) $(SAMPLE_INIT_LIBS) -o $@ $^

sample_aud_%: $(PWD)/sample_aud_%.o $(SDK_ROOT_PATH)/sample/utils/sample_utils.o
	$(CXX) $(CFLAGS) $(SAMPLE_AUD_LIBS) -o $@ $^

sample_yol%: $(PWD)/sample_yol%.o
	$(CXX) $(CFLAGS) $(SAMPLE_YOLO_LIBS) -o $@ $^

sample_vi_%: $(PWD)/sample_vi_%.o \
			 $(SDK_ROOT_PATH)/sample/utils/sample_utils.o \
			 $(SDK_ROOT_PATH)/sample/utils/middleware_utils.o \
			 $(SAMPLE_COMMON_FILE)
	$(CC) $(CFLAGS) $(SAMPLE_VI_LIBS) -o $@ $^

sample_app_%: $(PWD)/sample_app_%.o \
			  $(SDK_ROOT_PATH)/sample/utils/vi_vo_utils.o \
			  $(SAMPLE_COMMON_FILE)
	$(CC) $(CFLAGS) $(SAMPLE_APP_LIBS) -o $@ $^

