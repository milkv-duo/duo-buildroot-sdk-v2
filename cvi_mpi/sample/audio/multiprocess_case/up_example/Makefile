SHELL = /bin/bash
STATIC ?= 0
#
ifeq ($(PARAM_FILE), )
PARAM_FILE:=../../../../mpi_param.mk
include $(PARAM_FILE)
endif
include ../../../sample.mk
#
EXENAME = audup_master
EXENAME2 = audup_slave
AAC_OBJ = ../../aac_sample/cvi_audio_aac_adp.o
C_OBJS = cvi_rpc_upmaster.o
C_OBJS2 = cvi_rpc_upslave.o
#
DEFINES =
DEFINES += -DCVIAUDIO_STATIC
DEFINES += -DSUPPORT_EXTERNAL_AAC
#
INC = -I ./ -I$(MW_INC) -I$(KERNEL_INC) -I ../../aac_sample
LIBS += -L$(MW_LIB) -lcvi_audio -lm -lpthread -ldl -lcvi_VoiceEngine -lcvi_vqe -lcvi_dnvqe -ltinyalsa -lcvi_RES1 -lcvi_ssp -lcvi_ssp2 -lsys
LIBS += -L$(MW_LIB)/3rd -lsbc
ifeq ($(TARGET_MACHINE), aarch64-linux-gnu)
LIBS += -L $(MW_PATH)/3rdparty/nanomsg/lib_64
else ifeq ($(TARGET_MACHINE), riscv64-unknown-linux-musl)
LIBS += -L $(MW_PATH)/3rdparty/nanomsg/musl_riscv64
else
LIBS += -L $(MW_PATH)/3rdparty/nanomsg/lib_32
endif
LIBS += -lnanomsg
LIBS += -laacdec2
LIBS += -laacsbrdec2
LIBS += -laacenc2
LIBS += -laaccomm2
LIBS += -laacsbrenc2
CFLAGS += -g -Wall -O0 -fPIC -fgnu89-inline $(DEFINES) $(INC)
LDFLAGS = -lpthread -pthread  -lm

all: $(EXENAME) $(EXENAME2)

$(EXENAME): $(C_OBJS) $(AAC_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

$(EXENAME2): $(C_OBJS2) $(AAC_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)
clean:
	$(RM) $(EXENAME) $(EXENAME2) *.o *.d


.PHONY:clean all

-include $(DEPS)





