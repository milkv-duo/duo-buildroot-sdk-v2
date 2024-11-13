#
ifeq ($(TARGET_MACHINE), aarch64-linux-gnu)
PREBUILD_UAC = prebuild64
LDFLAGS = -shared -fPIC -O3 --gc-sections -export-dynamic
CFLAGS += -DTHIS_IS_64
endif
#
ifeq ($(TARGET_MACHINE), arm-linux-gnueabihf)
PREBUILD_UAC = prebuild32
LDFLAGS = -shared -fPIC -O3 --gc-sections -export-dynamic
CFLAGS += -DTHIS_IS_32
endif
#
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
#
LIB_DIR = -L$(PWD)/
EXENAME := uacaudio
DEFS=
CFLAGS += $(DEFS)
TARGET_SO = libcvi_uac.so

SRC_C += $(shell find ./ -name 'cvi_audio_uac.c')
SRC_C += $(shell find ./ -name 'cyclebuffer.c')

LIBS +=  -lm -lpthread  -ltinyalsa

OBJS := $(SRC_C:%.c=%.o)

CFLAGS += -fPIC -pthread
CFLAGS += $(INCLUDES)


#BUILD_LIST =  prebuild
BUILD_LIST =
BUILD_LIST += $(PREBUILD_UAC)
BUILD_LIST += $(EXENAME)
#BUILD_LIST += $(TARGET_SO)

#all:$(EXENAME)
all:$(BUILD_LIST)

prebuild64:
	@echo "prebuild 64bit untar"
	@tar zxvf prebuild.tar.gz

prebuild32:
	@echo "prebuild 32bit untar"
	@tar zxvf prebuild32bit.tar.gz

$(EXENAME): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^  $(LIB_DIR) $(LIBS)
	@rm -f asoundlib.h
	@rm -f libtinyalsa.so

$(TARGET_SO): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ --start-group $(OBJS) --end-group

clean:
	$(RM) $(OBJS) $(EXENAME) $(SHARED) *.o
	@rm -f asoundlib.h
	@rm -f libtinyalsa.so
	@rm -rf libcvi_uac.so

.PHONY:clean all
