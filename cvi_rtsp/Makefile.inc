ifneq ($(BUILD_PATH),)
include $(BUILD_PATH)/.config
else
CHIP_ARCH ?= CV182X
endif

CHIP_ARCH := CV186X

CHIP_ARCH_LOWERCASE := $(shell echo $(CHIP_ARCH) | tr A-Z a-z)
SDK_VER ?= 64bit
AISDK_VER ?= 2021-09-27

ifeq ($(CONFIG_ENABLE_SDK_ASAN),y)
    CFLAGS += -g -fsanitize=address -fno-omit-frame-pointer
    LDFLAGS += -lasan
endif

ifeq ($(SDK_VER), glibc_riscv64)
  CFLAGS += -march=rv64imafdcvxthead -mcmodel=medany -mabi=lp64dv
endif

CFLAGS += -DMW_VER=$(subst v,,$(MW_VER))
CFLAGS += -O3 -Wall  -Werror=all -DNO_OPENSSL=1
CPPFLAGS = -fPIC
CXXFLAGS = $(CPPFLAGS)

LIVE555_DIR ?= /system
LIVE555_INC ?= -I$(LIVE555_DIR)/include/BasicUsageEnvironment -I$(LIVE555_DIR)/include/groupsock -I$(LIVE555_DIR)/include/liveMedia -I$(LIVE555_DIR)/include/UsageEnvironment
LIVE555_LIB ?= $(LIVE555_DIR)/lib/libliveMedia.a $(LIVE555_DIR)/lib/libBasicUsageEnvironment.a $(LIVE555_DIR)/lib/libUsageEnvironment.a $(LIVE555_DIR)/lib/libgroupsock.a

MW_DIR ?= $(PWD)/../middleware/$(MW_VER)
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


ifeq ($(MW_VER), v2)
    PKG_CONFIG_PATH = $(MW_DIR)/pkgconfig
    MW_INC ?= $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags --define-variable=mw_dir=$(MW_DIR) cvi_common cvi_sample) -I$(KERNEL_INC)
    MW_LIB ?= $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs --define-variable=mw_dir=$(MW_DIR) cvi_common cvi_sample)
    # for cviai
    CFLAGS += -D_MIDDLEWARE_V2_=1

    MW_INC += -I$(MW_DIR)/include/isp/$(CHIP_ARCH_LOWERCASE)
else

ifeq ($(CHIP_ARCH), CV183X)
    MW_ISP_LIB ?= -L$(MW_DIR)/lib -lisp -lae -laf -lawb
else
    MW_ISP_LIB ?= -L$(MW_DIR)/lib -lisp -lisp_algo -lae -laf -lawb
endif

    MW_INC ?= -I$(MW_DIR)/include -I$(MW_DIR)/sample/common -I$(MW_DIR)/include/isp/$(CHIP_ARCH_LOWERCASE) -I$(CVI_TRACER_INC) $(CVI_TARGET_PACKAGES_INCLUDE)
    MW_LIB ?= -L$(MW_DIR)/lib -lsample -lcvi_bin -lcvi_bin_isp -lvpu -lm -lsys -lvenc -lvdec -lsns_full -lcvi_jpeg -lcvi_vcodec -lz -lini $(MW_ISP_LIB) -L$(MW_DIR)/lib/3rd $(CVI_TARGET_PACKAGES_LIBDIR) $(CVI_TRACER_LIB)
endif

CFLAGS += -DARCH_$(CHIP_ARCH)

RAMDISK_DIR ?= $(PWD)/../ramdisk
CVI_TRACER_INC ?= $(RAMDISK_DIR)/rootfs/public/cvitracer/include
ifeq ($(SDK_VER),64bit)
CVI_TRACER_LIB ?= -L$(RAMDISK_DIR)/rootfs/public/cvitracer/arm64/lib/3rd/ -lcvitracer
else ifeq ($(SDK_VER),32bit)
CVI_TRACER_LIB ?= -L$(RAMDISK_DIR)/rootfs/public/cvitracer/arm/lib/3rd/ -lcvitracer
else
CVI_TRACER_LIB ?= -L$(RAMDISK_DIR)/rootfs/public/cvitracer/uclibc/lib/3rd/ -lcvitracer
endif

MW_AUDIO_LIB ?= -L$(MW_DIR)/lib -ldl -lcvi_audio -lcvi_vqe -lae -lcvi_VoiceEngine -lcvi_RES1 -L/$(MW_DIR)/lib/3rd -ltinyalsa

AISDK_DIR ?= $(PWD)/../cviai
AISDK_INC ?= -I$(AISDK_DIR)/include
