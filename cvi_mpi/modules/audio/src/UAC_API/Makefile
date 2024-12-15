SHELL = /bin/bash
#
ifeq ($(PARAM_FILE), )
     PARAM_FILE:=../../../../mpi_param.mk
     include $(PARAM_FILE)
endif
#
ifeq ($(TARGET_MACHINE), aarch64-linux-gnu)
PREBUILD_UAC = prebuild64
CFLAGS += -DTHIS_IS_64
else ifeq ($(TARGET_MACHINE), arm-linux-gnueabihf)
PREBUILD_UAC = prebuild32
CFLAGS += -DTHIS_IS_32
endif
#
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld

LIB_DIR = -L$(CURDIR)/lib/
EXENAME := sample_audio_uac
DEFS=

CFLAGS += $(DEFS)
TARGET_SO = $(CURDIR)/lib/libcvi_uac.so

SRC_C += $(shell find ./ -name 'cvi_audio_uac.c')
SRC_C += $(shell find ./ -name 'cvi_audio_dl_adp.c')
SRC_C += $(shell find ./ -name 'cyclebuffer.c')
SRC_C_EXE  = $(shell find ./ -name 'cvi_sample_uac.c')


LIBS +=  -lm -ldl -lpthread  -ltinyalsa  -lcvi_mp3 -lcvi_RES1 -lmad -lcvi_uac -lgcc_s
AAC_LIBS = -lcvi_aac_dec  -lfdkaac_dec
LIBS += $(AAC_LIBS)

OBJS := $(SRC_C:%.c=%.o)
OBJS_TEST := $(SRC_C_EXE:%.c=%.o)

CFLAGS += -fPIC -pthread
UAC_INC = -I ./include/
CFLAGS += $(UAC_INC)

BUILD_LIST =
BUILD_LIST += $(PREBUILD_UAC)
BUILD_LIST += $(TARGET_SO)
BUILD_LIST += $(EXENAME)


all:$(BUILD_LIST)

prebuild64:
	@echo "prebuild 64bit lib untar"
	@pushd $(CURDIR)/prebuild/; tar zxvf prebuild.tar.gz -C $(CURDIR)/lib/; popd
	@mv $(CURDIR)/lib/asoundlib.h  $(CURDIR)/include/

prebuild32:
	@echo "prebuild 32bit lib untar"
	@pushd $(CURDIR)/prebuild/; tar zxvf prebuild32bit.tar.gz -C $(CURDIR)/lib/; popd
	@mv $(CURDIR)/lib/asoundlib.h  $(CURDIR)/include/

prebuilduclibc:
	@echo "prebuild uclibc lib untar"
	@pushd $(CURDIR)/prebuild/; tar zxvf prebuilduclibc.tar.gz -C $(CURDIR)/lib/; popd
	@mv $(CURDIR)/lib/asoundlib.h  $(CURDIR)/include/

$(EXENAME): $(OBJS_TEST)
	$(CC)   -o $@ $^  $(LIB_DIR) $(LIBS) $(CFLAGS) $(UAC_INC)

$(TARGET_SO): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ --start-group $(OBJS) --end-group

clean:
	$(RM) $(OBJS) $(EXENAME) $(SHARED) *.o
	@pushd $(CURDIR)/lib/; rm -rf ./*; popd
	@pushd $(CURDIR)/include/; [ -e asoundlib.h ] && rm -- asoundlib.h; popd

.PHONY:clean all
