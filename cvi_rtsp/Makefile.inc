ifneq ($(BUILD_PATH),)
include $(BUILD_PATH)/.config
else
CHIP_ARCH ?= CV182X
endif

CHIP_ARCH_LOWERCASE := $(shell echo $(CHIP_ARCH) | tr A-Z a-z)
SDK_VER ?= 64bit
AISDK_VER ?= 2021-09-27

ifeq ($(CONFIG_ENABLE_SDK_ASAN),y)
    CFLAGS += -g -fsanitize=address -fno-omit-frame-pointer
    LDFLAGS += -lasan
endif

ifeq ($(SDK_VER), glibc_riscv64)
  CFLAGS += -march=rv64imafdcv0p7xthead -mcmodel=medany -mabi=lp64d
endif
ifeq ($(SDK_VER), musl_riscv64)
  CFLAGS += -march=rv64imafdcv0p7xthead -mcmodel=medany -mabi=lp64d
endif

CFLAGS += -O3 -Wall  -Werror=all -DNO_OPENSSL=1
CPPFLAGS = -fPIC
CXXFLAGS = $(CPPFLAGS)

LIVE555_DIR ?= /system
LIVE555_INC ?= -I$(LIVE555_DIR)/include/BasicUsageEnvironment -I$(LIVE555_DIR)/include/groupsock -I$(LIVE555_DIR)/include/liveMedia -I$(LIVE555_DIR)/include/UsageEnvironment
LIVE555_LIB ?= $(LIVE555_DIR)/lib/libliveMedia.a $(LIVE555_DIR)/lib/libBasicUsageEnvironment.a $(LIVE555_DIR)/lib/libUsageEnvironment.a $(LIVE555_DIR)/lib/libgroupsock.a

MW_DIR ?= $(PWD)/../cvi_mpi
KERNEL_DIR ?= $(KERNEL_PATH)/build/$(PROJECT_FULLNAME)
ifeq ($(SDK_VER), 32bit)
    KERNEL_INC ?= $(KERNEL_DIR)/arm/usr/include
else ifeq ($(SDK_VER), 64bit)
    KERNEL_INC ?= $(KERNEL_DIR)/arm64/usr/include
else ifeq ($(SDK_VER), uclibc)
    KERNEL_INC ?= $(KERNEL_DIR)/arm/usr/include
else ifeq ($(SDK_VER), glibc_riscv64)
    KERNEL_INC ?= $(KERNEL_DIR)/riscv/usr/include
else ifeq ($(SDK_VER), musl_riscv64)
    KERNEL_INC := $(KERNEL_DIR)/riscv/usr/include
endif

PKG_CONFIG_PATH = $(MW_DIR)/pkgconfig
MW_INC ?= $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags --define-variable=mw_dir=$(MW_DIR) cvi_common cvi_sample) -I$(KERNEL_INC) -I$(MW_DIR)/include/linux
MW_LIB ?= $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs --define-variable=mw_dir=$(MW_DIR) cvi_common cvi_sample)

SENSOR_LIST_INC ?= -I$(MW_DIR)/component/isp/common
ISP_INC ?= -I$(MW_DIR)/include/isp/$(CHIP_ARCH_LOWERCASE)
SAMPLE_COMMON_DIR := $(MW_DIR)/sample/common

CFLAGS += -DARCH_$(CHIP_ARCH)

ifeq ("$(CVIARCH)", "CV181X")
CFLAGS += -D__CV181X__
endif
ifeq ("$(CVIARCH)", "CV180X")
CFLAGS += -D__CV180X__
endif

RAMDISK_DIR ?= $(PWD)/../ramdisk
CVI_TRACER_INC ?= $(RAMDISK_DIR)/rootfs/public/cvitracer/include
ifeq ($(SDK_VER),64bit)
CVI_TRACER_LIB ?= -L$(RAMDISK_DIR)/rootfs/public/cvitracer/arm64/lib/3rd/ -lcvitracer
else ifeq ($(SDK_VER),32bit)
CVI_TRACER_LIB ?= -L$(RAMDISK_DIR)/rootfs/public/cvitracer/arm/lib/3rd/ -lcvitracer
else
CVI_TRACER_LIB ?= -L$(RAMDISK_DIR)/rootfs/public/cvitracer/uclibc/lib/3rd/ -lcvitracer
endif

MW_AUDIO_LIB ?= -L$(MW_DIR)/lib -ldl -lcvi_audio -lcvi_vqe -lcvi_dnvqe -lcvi_ssp2 -lae -lcvi_VoiceEngine -lsbc -lcvi_RES1 -L/$(MW_DIR)/lib/3rd -ltinyalsa

AISDK_DIR ?= $(PWD)/../tdl_sdk
AISDK_INC ?= -I$(AISDK_DIR)/include
