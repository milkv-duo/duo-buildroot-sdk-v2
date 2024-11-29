SHELL = /bin/bash
#
CHIP_ARCH	?= CV181X
#
ifneq ($(BUILD_PATH),)
include $(BUILD_PATH)/.config
endif
## setup path ##
ROOT_DIR:=$(shell dirname $(realpath $(PARAM_FILE)))
export MW_PATH	:= $(ROOT_DIR)
export MW_INC 	:= $(MW_PATH)/include
export MW_LIB 	:= $(MW_PATH)/lib
export MW_3RD_LIB := $(MW_PATH)/lib/3rd
#
## GCC COMPILER ##
CC := $(CROSS_COMPILE)gcc
CXX := $(CROSS_COMPILE)g++
AR := $(CROSS_COMPILE)ar
LD := $(CROSS_COMPILE)ld
NM := $(CROSS_COMPILE)nm
RANLIB := $(CROSS_COMPILE)ranlib
STRIP := $(CROSS_COMPILE)strip
OBJCOPY := $(CROSS_COMPILE)objcopy
#
# riscv64-unknown-linux-musl, riscv64-unknown-linux-gnu, aarch64-linux-gnu, arm-linux-gnueabihf
export TARGET_MACHINE	:= $(shell ${CC} -dumpmachine)
export CHIP_ARCH_L	:= $(shell echo $(CHIP_ARCH) | tr A-Z a-z)
##
## INCLUDE PATH ##
SYS_INC = $(MW_PATH)/modules/sys/include
VI_INC = $(MW_PATH)/modules/vi/include
VPSS_INC = $(MW_PATH)/modules/vpss/include
VO_INC = $(MW_PATH)/modules/vo/include
GDC_INC = $(MW_PATH)/modules/gdc/include
RGN_INC = $(MW_PATH)/modules/rgn/include
AUD_INC = $(MW_PATH)/modules/audio/include
BIN_INC = $(MW_PATH)/modules/bin/include
SENSOR_LIST_INC = $(MW_PATH)/component/isp/common
#
ifeq ($(TARGET_MACHINE), arm-linux-gnueabihf)
  OPT_LEVEL := -O3
else ifeq ($(TARGET_MACHINE), aarch64-linux-gnu)
  OPT_LEVEL := -O3
else ifeq ($(TARGET_MACHINE), riscv64-unknown-linux-gnu)
  OPT_LEVEL := -Os
  OPT_LEVEL += -mno-ldd -mcpu=c906fdv -march=rv64imafdcv0p7xthead -mcmodel=medany -mabi=lp64d
else ifeq ($(TARGET_MACHINE), riscv64-unknown-linux-musl)
  OPT_LEVEL := -Os
  OPT_LEVEL += -mno-ldd -mcpu=c906fdv -march=rv64imafdcv0p7xthead -mcmodel=medany -mabi=lp64d
endif
KERNEL_INC  := ./

OSDRV_PATH	?= $(CURDIR)/../osdrv
KERNEL_PATH	?= $(CURDIR)/../linux_5.10
ISP_INC := $(MW_PATH)/modules/isp/include/$(CHIP_ARCH_L)

export OSDRV_PATH KERNEL_PATH CHIP_ARCH

## LIBRARY ##
ISP_LIB := -lisp -lawb -lae -laf
ISP_OBJ := $(MW_LIB)/libisp.a $(MW_LIB)/libae.a $(MW_LIB)/libawb.a $(MW_LIB)/libaf.a

ifeq ($(CHIP_ARCH), $(filter $(CHIP_ARCH), CV181X CV180X SG200X))
ISP_LIB += -lisp_algo
ISP_OBJ += $(MW_LIB)/libisp_algo.a
else
ifeq ($(SUBTYPE), fpga)
ISP_LIB += -lisp_algo
ISP_OBJ += $(MW_LIB)/libisp_algo.a
else
$(error UNKNOWN chip architecture - $(CHIP_ARCH))
endif
endif

MW_GLIBC_DEPENDENT := $(SYSROOT)/lib

## DEFINES ##
export PROJ_CFLAGS = -DF10

### COMMON COMPILER FLAGS ###
#
# export TARGET_PACKAGES_INCLUDE and TARGET_PACKAGES_LIBDIR from build/Makefile
#
WARNING_LEVEL :=  -Wall -Wextra -Werror

#Generate object files by CC
CFLAGS    := $(OPT_LEVEL) -std=gnu11 -g $(WARNING_LEVEL) -fPIC -ffunction-sections -fdata-sections
#Generate object files by CXX
CXXFLAGS  := $(OPT_LEVEL) -std=gnu++11 -g $(WARNING_LEVEL) -fPIC -ffunction-sections -fdata-sections
#Generate dependencies files by CC and CXX
DEPFLAGS  := -MMD
# DEPFLAGS  += -save-temps=obj

### COMMON AR and LD FLAGS ###
#Generate archive file by AR
ARFLAGS   := rcs
#Generate shared library by LD
LDFLAGS   := -shared -export-dynamic -L$(MW_LIB) -L$(MW_3RD_LIB)

### COMMON ELF FLAGS ###
#Generate ELF files by CC and CXX
ELFFLAGS = $(OPT_LEVEL) -Wl,--gc-sections -rdynamic -L$(MW_LIB) -L$(MW_3RD_LIB)


ifeq ($(CONFIG_DEBUG_INFO), y)
ifeq ($(CONFIG_DEBUG_INFO_SPLIT), y)
CFLAGS += -gsplit-dwarf
CXXFLAGS += -gsplit-dwarf
else ifeq ($(CONFIG_DEBUG_INFO_DWARF4), y)
CFLAGS += -gdwarf-4
CXXFLAGS += -gdwarf-4
else ifeq ($(CONFIG_DEBUG_INFO_REDUCED), y)
CFLAGS += -femit-struct-debug-baseonly -fno-var-tracking
CXXFLAGS += -femit-struct-debug-baseonly -fno-var-tracking
else
CFLAGS += -Og
CXXFLAGS += -Og
endif
endif

ifeq ($(CONFIG_RTOS_INIT_MEDIA), y)
CFLAGS += -DRTOS_INIT_MEDIA
endif

ifeq ($(MTRACE), y)
CFLAGS += -DMTRACE
endif

SAMPLE_STATIC ?= 1
SELF_TEST_STATIC ?= 1
SAMPLE_TPU_ENABLE ?= 0
ENABLE_ISP_C906L = 0
PQBIN_USE_JSON := 1

ifeq ($(CONFIG_ENABLE_SDK_ASAN), y)
CFLAGS += -fsanitize=address -fno-omit-frame-pointer
CXXFLAGS += -fsanitize=address -fno-omit-frame-pointer
ELFFLAGS += -fsanitize=address -fno-omit-frame-pointer -g
LDFLAGS += -fsanitize=address -fno-omit-frame-pointer -fno-common -g
SAMPLE_STATIC = 0
SELF_TEST_STATIC = 0
endif

ifeq ("$(CHIP_ARCH)", "CV181X")
CFLAGS += -D__CV181X__
CXXFLAGS += -D__CV181X__
else ifeq ("$(CHIP_ARCH)", "CV180X")
CFLAGS += -D__CV180X__
CXXFLAGS += -D__CV180X__
else
$(error "nuknown soc type.")
endif

CFLAGS += -DOS_IS_LINUX

## COLOR ##
BLACK = "\e[30;1m"
RED  =  "\e[31;1m"
GREEN = "\e[32;1m"
YELLOW = "\e[33;1m"
BLUE  = "\e[34;1m"
PURPLE = "\e[35;1m"
CYAN  = "\e[36;1m"
WHITE = "\e[37;1m"

END= "\e[0m"

