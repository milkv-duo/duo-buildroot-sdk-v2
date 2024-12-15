SHELL = /bin/bash
#
ifeq ($(PARAM_FILE), )
PARAM_FILE:=../../../../../../../mpi_param.mk
include $(PARAM_FILE)
endif
#
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
#
LIB_DIR = -L$(CURDIR)/../lib64bit -L$(MW_LIB) -L$(MW_3RD_LIB)
EXENAME := cvimp3
DEFS=
CFLAGS += $(DEFS)
TARGET_SO = ../lib64bit/libcvi_mp3.so

SDIR = $(CURDIR)
SRC_C += $(shell find ./ -name 'cvi_mp3_decode.c')
INCS = -I$(MW_INC) -I$(CURDIR)/../
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

all:$(BUILD_LIST)


$(EXENAME): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^  $(LIB_DIR) $(LIBS)

$(TARGET_SO): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ --start-group $(OBJS) --end-group

export_mp3dec_lib:
#	@cp ../lib64bit/*  $(MW_LIB)/.
	@cp ../lib64bit/*  $(MW_PATH)/3rdparty/libmad/lib_64/.
clean:
	$(RM) $(OBJS) $(EXENAME) $(SHARED) *.o
	@rm -f $(MW_LIB)/libmad*
	@rm -f $(MW_LIB)/libcvi_mp3*
	@rm -f $(MW_PATH)/3rdparty/libmad/lib_64/*

.PHONY:clean all
