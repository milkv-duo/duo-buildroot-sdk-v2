SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
PARAM_FILE:=../../../../../../mpi_param.mk
include $(PARAM_FILE)
endif

ifeq ($(TARGET_MACHINE), aarch64-linux-gnu)
LDFLAGS = -shared -fPIC -O3 --gc-sections -export-dynamic
OUTPUT_DIR = ../lame_output/64bit/lib
RELEASE_DIR = $(MW_PATH)/3rdparty/lame/lib_64
LAME_INCLUDE = ../lame_output/64bit/include/lame
LIB_DIR = -L$(CURDIR)/ -L$(OUTPUT_DIR)/ -L$(MW_LIB) -L$(MW_3RD_LIB)
endif

ifeq ($(TARGET_MACHINE), arm-linux-gnueabihf)
LDFLAGS = -shared -fPIC -O3 --gc-sections -export-dynamic
OUTPUT_DIR = ../lame_output/32bit/lib
RELEASE_DIR = $(MW_PATH)/3rdparty/lame/lib_32
LAME_INCLUDE = ../lame_output/32bit/include/lame
LIB_DIR = -L$(CURDIR)/ -L$(OUTPUT_DIR)/ -L$(MW_LIB) -L$(MW_3RD_LIB)
endif

ifeq ($(TARGET_MACHINE), riscv64-unknown-linux-gnu)
LDFLAGS = -shared -fPIC -O3 --gc-sections -export-dynamic
OUTPUT_DIR = ../lame_output/riscv64/lib
RELEASE_DIR = $(MW_PATH)/3rdparty/lame/lib_riscv64
LAME_INCLUDE = ../lame_output/riscv64/include/lame
LIB_DIR = -L$(CURDIR)/ -L$(OUTPUT_DIR)/ -L$(MW_LIB) -L$(MW_3RD_LIB)
endif

CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld

EXENAME := cvimp3enc
DEFS=
CFLAGS += $(DEFS)
##CFLAGS += -DTEST_IN_ENCODE_UNIT
TARGET_SO = $(OUTPUT_DIR)/libcvi_mp3enc.so

SDIR = $(CURDIR)
SRC_C += $(shell find ./ -name 'cvi_mp3_encode.c')
INCS = -I$(MW_INC) -I$(CURDIR)/../ -I$(LAME_INCLUDE) -I$(KERNEL_INC)
LIBS +=  -lm -lpthread
LIBS += -lmp3lame

OBJS := $(SRC_C:%.c=%.o)

CFLAGS += -fPIC -pthread
CFLAGS += $(INCLUDES)
CFLAGS += $(INCS)
##turn on below if you want to build unit test in cvi_mp3_decode.c
##CFLAGS += -DTEST_IN_DECODE_UNIT

EXPORT_LIBMAD = export_mp3enc_lib

BUILD_LIST =
##BUILD_LIST += $(EXENAME)
BUILD_LIST += $(TARGET_SO)
BUILD_LIST += $(EXPORT_LIBMAD)

all:$(BUILD_LIST)


$(EXENAME): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^  $(LIB_DIR) $(LIBS)

$(TARGET_SO): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ --start-group $(OBJS) --end-group

export_mp3enc_lib:
#	@cp $(OUTPUT_DIR)/*  $(MW_LIB)/.
	@cp -a $(OUTPUT_DIR)/libcvi*  $(RELEASE_DIR)/.
	@cp -a $(OUTPUT_DIR)/libmp3lame.a  $(RELEASE_DIR)/.
	@cp -a $(OUTPUT_DIR)/libmp3lame.la  $(RELEASE_DIR)/.
	@cp -a $(OUTPUT_DIR)/libmp3lame.so*  $(RELEASE_DIR)/.
clean:
	rm -f $(OBJS) $(EXENAME) $(SHARED) *.o
	@rm -f $(MW_LIB)/libcvi_mp3enc*
	@rm -f $(MW_LIB)/libmp3lame*
	@rm -f $(RELEASE_DIR)/*
	@rm -f $(OUTPUT_DIR)/libcvi_mp3enc*

.PHONY:clean all
