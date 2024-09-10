SHELL = /bin/bash
# include ./Makefile.param

CHECK_SDK = 1
ifeq ($(MAKECMDGOALS), $(filter $(MAKECMDGOALS), install clean))
CHECK_SDK = 0
endif
ifndef MAKECMDGOALS
CHECK_SDK = 1
endif

ifeq ($(CHECK_SDK), 1)
ifndef MW_PATH
$(error "Please set middleware sdk root path to MW_PATH")
endif

ifeq ($(USE_TPU_IVE), ON)
ifndef IVE_PATH
$(error "Please set rtsp sdk root path to IVE_PATH)
endif
endif

ifndef KERNEL_ROOT
$(error "Please set path to kernrel_root)
endif

ifndef TPU_PATH
$(error "Please set rtsp sdk root path to TPU_PATH)
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
ifeq ($(CHIP), $(filter $(CHIP), cv1832 cv1835 cv1838 cv9520 CV183X))
LINK_OPENCV = 1
else
LINK_OPENCV = 0
endif

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
else ifeq ($(SDK_VER), musl_riscv64)
ARCH=riscv64
CROSS_COMPILE = $(CROSS_COMPILE_RISCV64_MUSL)
CFLAGS += -march=rv64imafdcv0p7xthead
else
$(error "Unknown SDK_VER: $(SDK_VER")")
endif

# Middleware stuffs
#####################################################
MW_INC_PATH = $(MW_PATH)/include
#####################################################


# TDLSDK stuffs
#####################################################
TDLSDK_ROOT_PATH ?= ../..
TDLSDK_LIB_PATH = $(TDLSDK_ROOT_PATH)/lib
TDLSDK_INC_PATH = $(TDLSDK_ROOT_PATH)/include/cvi_tdl
TDLSDK_APP_INC_PATH = $(TDLSDK_ROOT_PATH)/include
#####################################################


# IVE
#####################################################
IVE_SDK_INC_PATH = $(IVE_PATH)/include

ifeq ($(CHIP), CV181X)
CFLAGS += -DCV181X
endif

CFLAGS += -std=gnu11 -Wno-pointer-to-int-cast -fsigned-char -Werror=all -fdiagnostics-color=always -s 
CFLAGS += -I$(KERNEL_ROOT)/include

ifeq ($(DEBUG), 1)
CFLAGS += -g -O0
STRIPSYMBOL =
else
CFLAGS += -O3 -DNDEBUG
STRIPSYMBOL = @$(STRIP) $@
endif


## GCC COMPILER ##
CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
AR = $(CROSS_COMPILE)ar
LD = $(CROSS_COMPILE)ld
STRIP = $(CROSS_COMPILE)strip

TDLSDK_LIBS = -lcvi_tdl

TDL_APP_LIBS = $(TDLSDK_LIBS) -L$(TDLSDK_LIB_PATH) -lpthread

CFLAGS += -I$(TDLSDK_INC_PATH) -I$(TDLSDK_APP_INC_PATH) -I$(MW_INC_PATH) -I$(TDLSDK_ROOT_PATH)/sample/3rd/include

ifeq ($(USE_TPU_IVE), ON)
CFLAGS += -I$(IVE_SDK_INC_PATH)
endif

CFLAGS += -Wl,-gc-sections


ifeq ($(CHECK_SDK), 1)
$(info ---------------------------------------)
$(info CHIP: $(CHIP))
$(info SDK_VER: $(SDK_VER))
$(info Middleware include path: $(MW_INC_PATH))
$(info Middleware library path: $(MW_LIB_PATH))
$(info TDL SDK library path: $(TDLSDK_LIB_PATH))
$(info TDL SDK include path: $(TDLSDK_INC_PATH))
$(info Kernel library path: $(KERNEL_ROOT))

$(info IVE library path: $(IVE_SDK_LIB_PATH))
$(info IVE include path: $(IVE_SDK_INC_PATH))
$(info TPU library path: $(TPU_LIB_PATH))
$(info CFLAGS: $(CFLAGS))
$(info CC: $(CC))
$(info CXX: $(CXX))
$(info USE_TPU_IVE: $(USE_TPU_IVE))
$(info ---------------------------------------)
endif


.PHONY : all clean install

all: libcvi_tdl_app.so

face_capture.o: face_capture/face_capture.c
	$(CC) $(DEPFLAGS) $(CFLAGS) -fPIC -o $@ -c $^

person_capture.o: person_capture/person_capture.c
	$(CC) $(DEPFLAGS) $(CFLAGS) -fPIC -o $@ -c $^

personvehicle_capture.o: personvehicle_capture/personvehicle_capture.c
	$(CC) $(DEPFLAGS) $(CFLAGS) -fPIC -o $@ -c $^

cvi_tdl_app.o: cvi_tdl_app.c
	$(CC) $(DEPFLAGS) $(CFLAGS) -fPIC -o $@ -c $^

libcvi_tdl_app.so: cvi_tdl_app.o face_capture.o person_capture.o personvehicle_capture.o
	$(CC) $(CFLAGS) $(TDL_APP_LIBS) $^ -o $@ -shared

clean:
	rm -f libcvi_tdl_app.so cvi_tdl_app.o face_capture.o person_capture.o personvehicle_capture.o

install:
	mkdir -p ../../tmp_install/lib
	cp libcvi_tdl_app.so ../../tmp_install/lib
