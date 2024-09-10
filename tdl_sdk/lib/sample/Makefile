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

# link OpenCV shared object only if platform is cv183X series

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
CFLAGS += -march=rv64imafdcv0p7xthead -mabi=lp64d
LDFLAGS = -mcpu=c906fdv
else ifeq ($(SDK_VER), musl_riscv64)
ARCH=riscv64
CROSS_COMPILE = $(CROSS_COMPILE_RISCV64_MUSL)
CFLAGS += -march=rv64imafdcv0p7xthead
LDFLAGS = -mcpu=c906fdv
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
MW_SAMPLE_PATH = $(MW_PATH)/sample/common
MW_PANEL_INC_PATH += ${MW_PATH}/include/panel
MW_3RD_LIB_PATH = $(MW_LIB_PATH)/3rd

ifeq ($(CHIP), CV186X)
	MW_LIBS = -lsys -lvi -lvpss -lvenc -lvdec -lvo -lrgn -lgdc -lini -lisp -lsns_full -lawb -lae -laf -lcvi_bin_isp -lcvi_bin -lisp_algo
	MW_ISP_INC_PATH = $(MW_INC_PATH)/isp/cv186x/
	CFLAGS += -DCV186X
	MW_PANEL_BOARD_INC_PATH = ${MW_PATH}/component/panel/cv186x
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
	CFLAGS += -DCV181X -D__CV181X__
else ifeq ($(CHIP), CV180X)
	MW_LIBS = -lini -lsns_full -lsample -lisp -lvdec -lvenc -lawb -lae -laf -lcvi_bin -lcvi_bin_isp -lmisc -lisp_algo -lsys  -lvi -lvo -lvpss -lrgn -lgdc
	MW_ISP_INC_PATH = $(MW_INC_PATH)/isp/cv180x
	CFLAGS += -DCV180X -D__CV180X__
endif

#####################################################
# TDL SDK stuffs
#####################################################
SDK_ROOT_PATH ?= $(abspath ../..)
SDK_LIB_PATH = $(SDK_ROOT_PATH)/lib
SDK_INC_PATH = $(SDK_ROOT_PATH)/include
SDK_3RD_LIB_PATH = $(SDK_ROOT_PATH)/sample/3rd/lib
SDK_TDL_INC_PATH = $(SDK_ROOT_PATH)/include/cvi_tdl
SDK_APP_INC_PATH = $(SDK_ROOT_PATH)/include/cvi_tdl_app
SDK_SAMPLE_INC_PATH = $(SDK_ROOT_PATH)/sample/3rd/include
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
TPU_LIB_PATH = ${TPU_PATH}/libsophon-0.4.9/lib/
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
# CVI LIB stuffs
#####################################################
MD_INC_PATH = $(SDK_ROOT_PATH)/include/cvi_md
DRAW_RECT_INC_PATH = $(SDK_ROOT_PATH)/include/cvi_draw_rect
PREPROCESS_INC_PATH = $(SDK_ROOT_PATH)/include/cvi_preprocess

CFLAGS += -fsigned-char -Werror=all -Wno-format-truncation -fdiagnostics-color=always -s

SAMPLE_LIBS = -L$(MW_LIB_PATH) -L$(MW_3RD_LIB_PATH) $(MW_LIBS) \
 			  -L$(TPU_LIB_PATH) $(TPU_LIBS) \
			  -L$(IVE_SDK_LIB_PATH) $(IVE_LIBS) \
			  -L$(SDK_LIB_PATH) -L$(SDK_3RD_LIB_PATH) -lpthread -latomic

SAMPLE_LIBS_OPENCV += $(OPENCV_LIBS) -L$(OPENCV_LIB_PATH)

CFLAGS += -I$(SDK_INC_PATH) \
          -I$(SDK_TDL_INC_PATH) \
		  -I$(SDK_SAMPLE_INC_PATH) \
		  -I$(IVE_SDK_INC_PATH) \
	  	  -I$(OPENCV_INC_PATH) \
		  -I$(MD_INC_PATH) \
		  -I$(DRAW_RECT_INC_PATH) \
	  	  -I$(PREPROCESS_INC_PATH) \
		  -I$(MW_INC_PATH) \
		  -I$(MW_SAMPLE_PATH) \
		  -I$(MW_ISP_INC_PATH) \
		  -I$(MW_INC_PATH)/linux \
		  -I$(KERNEL_ROOT)/include

ifeq ($(CONFIRM_ENV_VAR), 1)
$(info ---------------------------------------)
$(info CHIP: $(CHIP))
$(info SDK_VER: $(SDK_VER))
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
$(info ---------------------------------------)
endif

TARGETS := $(shell find . -type f -name 'sample_lib_*.cpp' -exec basename {} .cpp ';')

.PHONY : all clean

all: $(TARGETS)

clean:
	rm -f $(TARGETS) *.o

$(PWD)/%.o: $(PWD)/%.cpp
	$(CXX) $(DEPFLAGS) $(CFLAGS) -o $@ -c $<

sample_lib_cvimd: $(PWD)/sample_lib_cvimd.o
	$(CXX) $(DEPFLAGS) $(LDFLAGS) $(SAMPLE_LIBS) -lcvi_md -o $@ $^

sample_lib_draw_rect: $(PWD)/sample_lib_draw_rect.o
	$(CXX) $(DEPFLAGS) $(LDFLAGS) $(SAMPLE_LIBS_OPENCV) $(SAMPLE_LIBS) -lcvi_draw_rect -o $@ $^

sample_lib_preprocess: $(PWD)/sample_lib_preprocess.o
	$(CXX) $(DEPFLAGS) $(LDFLAGS) $(SAMPLE_LIBS) -lcvi_preprocess -o $@ $^

