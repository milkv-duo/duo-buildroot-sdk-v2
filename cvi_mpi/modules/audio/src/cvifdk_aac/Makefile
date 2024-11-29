.PHONY: CREATE_DIR

# FDK_AAC_DIR=./fdkaac
# CUR = $(CURDIR)

SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
PARAM_FILE:=../../../../mpi_param.mk
include $(PARAM_FILE)
endif
CUR = $(CURDIR)

#aac encoder -------------------------------------------start
SYS_SRC = \
    libSYS/src/genericStds.cpp \
    libSYS/src/syslib_channelMapDescr.cpp

AACENC_SRC =  $(wildcard $(CURDIR)/fdkaac/libAACenc/src/*.cpp)
AACENC_SRC_EXTRA = $(wildcard $(CURDIR)/fdkaac/libSACenc/src/*.cpp)
AACENC_SRC_EXTRA += $(wildcard $(CURDIR)/fdkaac/libSBRenc/src/*.cpp)
AACENC_SRC_EXTRA += $(wildcard $(CURDIR)/fdkaac/libMpegTPEnc/src/*.cpp)
AACENC_SRC_EXTRA += $(wildcard $(CURDIR)/fdkaac/libSYS/src/*.cpp)
AACENC_SRC_EXTRA += $(wildcard $(CURDIR)/fdkaac/libPCMutils/src/*.cpp)
AACENC_SRC_EXTRA += $(wildcard $(CURDIR)/fdkaac/libFDK/src/*.cpp)
AACENC_SRC += $(AACENC_SRC_EXTRA)


AACENC_INCS =
AACENC_INCS += -I ./fdkaac/libFDK/include/
AACENC_INCS += -I ./fdkaac/libSYS/include/
AACENC_INCS += -I ./fdkaac/libMpegTPEnc/include/
AACENC_INCS += -I ./fdkaac/libSBRenc/include/
AACENC_INCS += -I ./fdkaac/libAACenc/include/
AACENC_INCS += -I ./fdkaac/libSACenc/include/
AACENC_INCS += -I ./fdkaac/libPCMutils/include/

OBJS_ENC = $(AACENC_SRC:.cpp=.o)
#aac encoder -------------------------------------------end


#aac decoder -------------------------------------------start
AACDEC_SRC = $(wildcard $(CURDIR)/fdkaac/libAACdec/src/*.cpp)
AACDEC_SRC_EXTRA = $(wildcard $(CURDIR)/fdkaac/libFDK/src/*.cpp)
AACDEC_SRC_EXTRA += $(wildcard $(CURDIR)/fdkaac/libSYS/src/*.cpp)
AACDEC_SRC_EXTRA += $(wildcard $(CURDIR)/fdkaac/libArithCoding/src/*.cpp)
AACDEC_SRC_EXTRA += $(wildcard $(CURDIR)/fdkaac/libMpegTPDec/src/*.cpp)
AACDEC_SRC_EXTRA += $(wildcard $(CURDIR)/fdkaac/libSBRdec/src/*.cpp)
AACDEC_SRC_EXTRA += $(wildcard $(CURDIR)/fdkaac/libPCMutils/src/*.cpp)
AACDEC_SRC_EXTRA += $(wildcard $(CURDIR)/fdkaac/libDRCdec/src/*.cpp)
AACDEC_SRC_EXTRA += $(wildcard $(CURDIR)/fdkaac/libSACdec/src/*.cpp)

AACDEC_SRC += $(AACDEC_SRC_EXTRA)

AACDEC_INCS =
AACDEC_INCS += -I ./fdkaac/libFDK/include/
AACDEC_INCS += -I ./fdkaac/libSYS/include/
AACDEC_INCS += -I ./fdkaac/libAACdec/include/
AACDEC_INCS += -I ./fdkaac/libArithCoding/include/
AACDEC_INCS += -I ./fdkaac/libAACdec/src/
AACDEC_INCS += -I ./fdkaac/libMpegTPDec/include/
AACDEC_INCS += -I ./fdkaac/libSBRdec/include/
AACDEC_INCS += -I ./fdkaac/libPCMutils/include/
AACDEC_INCS += -I ./fdkaac/libDRCdec/include/
AACDEC_INCS += -I ./fdkaac/libSACdec/include/


OBJS_DEC = $(AACDEC_SRC:.cpp=.o)
#aac decoder -------------------------------------------end

ifeq ($(TARGET_MACHINE), aarch64-linux-gnu)
AACENC_TARGET = $(CUR)/$(TARGET_MACHINE)/libfdkaac_enc.a
AACENC_TARGET_S = $(CUR)/$(TARGET_MACHINE)/libfdkaac_enc.so

AACDEC_TARGET = $(CUR)/$(TARGET_MACHINE)/libfdkaac_dec.a
AACDEC_TARGET_S = $(CUR)/$(TARGET_MACHINE)/libfdkaac_dec.so
TARGET_FLD = $(MW_LIB)/../modules/audio/lib
PREBUILD_FLD = ./$(TARGET_MACHINE)/
endif

ifeq ($(TARGET_MACHINE), $(TARGET_MACHINE)/-linux-gnueabihf)
AACENC_TARGET = $(CUR)/32bit/libfdkaac_enc.a
AACENC_TARGET_S = $(CUR)/$(TARGET_MACHINE)/libfdkaac_enc.so

AACDEC_TARGET = $(CUR)/$(TARGET_MACHINE)/libfdkaac_dec.a
AACDEC_TARGET_S = $(CUR)/$(TARGET_MACHINE)/libfdkaac_dec.so
TARGET_FLD = $(MW_LIB)/../modules/audio/lib
PREBUILD_FLD = ./$(TARGET_MACHINE)/
endif

ifeq ($(TARGET_MACHINE), riscv64-unknown-linux-gnu)
AACENC_TARGET = $(CUR)/$(TARGET_MACHINE)/libfdkaac_enc.a
AACENC_TARGET_S = $(CUR)/$(TARGET_MACHINE)/libfdkaac_enc.so

AACDEC_TARGET = $(CUR)/$(TARGET_MACHINE)/libfdkaac_dec.a
AACDEC_TARGET_S = $(CUR)/$(TARGET_MACHINE)/libfdkaac_dec.so
TARGET_FLD = $(MW_LIB)/../modules/audio/lib
PREBUILD_FLD = ./$(TARGET_MACHINE)
endif

ARFLAGS = rcs
ARFLAGS_S = -shared -fPIC -o
CFLAGS = -fPIC -c -Os

BUILDLIST=
BUILDLIST += $(AACENC_TARGET)
BUILDLIST += $(AACDEC_TARGET)

OBJECTPATHS_ENCSRC=$(addprefix $(CURDIR)/,$(notdir $(OBJS_ENC)))
OBJECTPATHS_DECSRC=$(addprefix $(CURDIR)/,$(notdir $(OBJS_DEC)))

all:$(BUILDLIST)


$(OBJS_ENC):
	$(CC) $(AACENC_SRC) $(CFLAGS) $(AACENC_INCS)

$(OBJS_DEC):
	$(CC) $(AACDEC_SRC) $(CFLAGS) $(AACDEC_INCS)


$(AACENC_TARGET) : $(OBJS_ENC)
	$(AR) $(ARFLAGS) $(AACENC_TARGET) $(OBJECTPATHS_ENCSRC)
	$(CC) $(ARFLAGS_S)  $(AACENC_TARGET_S) $(OBJECTPATHS_ENCSRC)
	rm -rf  $(OBJECTPATHS_ENCSRC)

$(AACDEC_TARGET) : $(OBJS_DEC)
	$(AR) $(ARFLAGS) $(AACDEC_TARGET) $(OBJECTPATHS_DECSRC)
	$(CC) $(ARFLAGS_S)  $(AACDEC_TARGET_S) $(OBJECTPATHS_DECSRC)
	rm -rf  $(OBJECTPATHS_DECSRC)


TARGET_PREBUILD :
	@cp -rfa $(PREBUILD_FLD)/libaacenc.so  $(TARGET_FLD)/cvi_aacenc/.
	@cp -rfa $(PREBUILD_FLD)/libaacdec.so  $(TARGET_FLD)/cvi_aacdec/.
	@echo "USE_QUICK_AACDEC_BUILD = yes[fdk-aac]"

.PHONY : clean
clean:
	rm -f    $(OBJECTPATHS_ENCSRC) $(OBJECTPATHS_DECSRC)




