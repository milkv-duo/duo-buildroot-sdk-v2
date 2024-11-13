#!bash/bin
#
ifeq ($(PARAM_FILE), )
PARAM_FILE:=../../../../../mpi_param.mk
include $(PARAM_FILE)
endif
#
AUDIO_PARAM_FILE :=../../../audio.mk
include $(AUDIO_PARAM_FILE)
#
SDIR := $(CURDIR)
C_SRCS = $(wildcard $(SDIR)/*.c)
#
ifeq ($(TARGET_MACHINE), arm-linux-gnueabihf)
TARGET_A = $(SDIR)/32bit/libcvi_aac_dec.a
TARGET_SO = $(SDIR)/32bit/libcvi_aac_dec.so
LIBS = -L$(CURDIR)/../32bit/ -lfdkaac_dec
PREBUILT_LIB = $(MW_LIB)/../3rdparty/libaac/32bit
endif
#
ifeq ($(TARGET_MACHINE), aarch64-linux-gnu)
TARGET_A = $(SDIR)/64bit/libcvi_aac_dec.a
TARGET_SO = $(SDIR)/64bit/libcvi_aac_dec.so
LIBS = -L$(CURDIR)/../64bit/ -lfdkaac_dec
PREBUILT_LIB = $(MW_LIB)/../3rdparty/libaac/64bit
endif
#
ifeq ($(TARGET_MACHINE), riscv64-unknown-linux-musl)
TARGET_A = $(SDIR)/$(TARGET_MACHINE)/libcvi_aac_dec.a
TARGET_SO = $(SDIR)/$(TARGET_MACHINE)/libcvi_aac_dec.so
LIBS = -L$(CURDIR)/../$(TARGET_MACHINE)/ -lfdkaac_dec
PREBUILT_LIB = $(MW_LIB)/../3rdparty/libaac/$(TARGET_MACHINE)
endif
#
DEFINES =
#
INC = -I$(CURDIR)/
INC += -I ../fdkaac/libFDK/include/
INC += -I ../fdkaac/libSYS/include/
INC += -I ../fdkaac/libAACdec/include/
INC += -I ../fdkaac/libArithCoding/include/
INC += -I ../fdkaac/libAACdec/src/
INC += -I ../fdkaac/libMpegTPDec/include/
INC += -I ../fdkaac/libSBRdec/include/
INC += -I ../fdkaac/libPCMutils/include/
INC += -I ../fdkaac/libDRCdec/include/
INC += -I ../fdkaac/libSACdec/include/
INC += -I$(MW_PATH)/include
#
CFLAGS = -g -Wall -O0 -fPIC -fgnu89-inline  $(DEFINES) $(INC)
#LDFLAGS = -lpthread -pthread  -lm

ARFLAGS = rcs
ARFLAGS_S = -shared -fPIC -o

C_OBJS = $(C_SRCS:.c=.o)

#all: $(EXENAME)
all: $(TARGET_A) $(TARGET_SO)
	cp -a $(TARGET_A) $(TARGET_SO) $(PREBUILT_LIB)

$(SDIR)/%.o: $(SDIR)/%.c
	$(CC) $(CFLAGS) $(LIBS) -o $@ -c $<

$(TARGET_A): $(C_OBJS)
	$(AR) $(ARFLAGS) $@ $(C_OBJS)

$(TARGET_SO): $(C_OBJS)
	$(CC) $(LDFLAGS) $(ARFLAGS_S) $@ $(C_OBJS)

clean:
	@rm -f $(C_OBJ) server *.o $(TARGET_A) $(TARGET_SO)
	@rm -rf $(PREBUILT_LIB)/libcvi_aac_dec.*

.PHONY:clean all




