SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
PARAM_FILE:=../../../../../../mpi_param.mk
include $(PARAM_FILE)
endif

ifeq ($(TARGET_MACHINE), aarch64-linux-gnu)
LDFLAGS = -shared -fPIC --gc-sections -export-dynamic
OUTPUT_DIR = ../libmad_output/64bit/lib
RELEASE_DIR = $(MW_PATH)/3rdparty/libmad/lib_64
LIBMAD_INCLUDE = ../libmad_output/64bit/include
LIB_DIR = -L$(CURDIR)/ -L$(OUTPUT_DIR)/ -L$(MW_LIB) -L$(MW_3RD_LIB)
endif

ifeq ($(TARGET_MACHINE), arm-linux-gnueabihf)
LDFLAGS = -shared -fPIC --gc-sections -export-dynamic
OUTPUT_DIR = ../libmad_output/32bit/lib
RELEASE_DIR = $(MW_PATH)/3rdparty/libmad/lib_32
LIBMAD_INCLUDE = ../libmad_output/32bit/include
LIB_DIR = -L$(CURDIR)/ -L$(OUTPUT_DIR)/ -L$(MW_LIB) -L$(MW_3RD_LIB)
endif

ifeq ($(TARGET_MACHINE), riscv64-unknown-linux-musl)
LDFLAGS = -shared -fPIC --gc-sections -export-dynamic
OUTPUT_DIR = ../libmad_output/musl64/lib
RELEASE_DIR = $(MW_PATH)/3rdparty/libmad/lib_riscv64
LIBMAD_INCLUDE = ../libmad_output/musl64/include
LIB_DIR = -L$(CURDIR)/ -L$(OUTPUT_DIR)/ -L$(MW_LIB) -L$(MW_3RD_LIB)
endif

CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld

EXENAME := cvimp3
DEFS=
CFLAGS += $(DEFS)
TARGET_SO = $(OUTPUT_DIR)/libcvi_mp3.so
TARGET_A = $(OUTPUT_DIR)/libcvi_mp3.a

SDIR = $(CURDIR)
SRC_C += $(shell find ./ -name 'cvi_mp3_decode.c')
INCS = -I$(MW_INC) -I$(LIBMAD_INCLUDE)
LIBS +=  -lm -lpthread
LIBS += -lmad

OBJS := $(SRC_C:%.c=%.o)

CFLAGS += -fPIC -pthread
CFLAGS += $(INCLUDES)
CFLAGS += $(INCS)
##turn on below if you want to build unit test in cvi_mp3_decode.c
##CFLAGS += -DTEST_IN_DECODE_UNIT

EXPORT_LIBMAD = export_mp3dec_lib

BUILD_LIST =
##BUILD_LIST += $(EXENAME)
BUILD_LIST += $(TARGET_SO)
BUILD_LIST += $(EXPORT_LIBMAD)
BUILD_LIST += $(TARGET_A)

all:$(BUILD_LIST)


$(EXENAME): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^  $(LIB_DIR) $(LIBS)

$(TARGET_SO): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ --start-group $(OBJS) --end-group

$(TARGET_A):$(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

export_mp3dec_lib:
#	cp $(OUTPUT_DIR)/*  $(MW_LIB)/.
	cp -a $(OUTPUT_DIR)/libcvi*  $(RELEASE_DIR)/.
	cp -a $(OUTPUT_DIR)/libmad.a  $(RELEASE_DIR)/.
	cp -a $(OUTPUT_DIR)/libmad.la  $(RELEASE_DIR)/.
	cp -a $(OUTPUT_DIR)/libmad.so*  $(RELEASE_DIR)/.
	cp -a $(CURDIR)/*.h $(LIBMAD_INCLUDE)
clean:
	rm -f $(OBJS) $(EXENAME) $(SHARED) *.o
	rm -f $(MW_LIB)/libmad*
	rm -f $(MW_LIB)/libcvi_mp3*
	rm -f $(RELEASE_DIR)/*
	rm -f $(OUTPUT_DIR)/libcvi_mp3*

.PHONY:clean all
