include ../Makefile.inc
#
LIBNAME = libcvi_rtsp_service
VENC_JSON_LIB = libvenc_json.a
SHARED = $(LIBNAME).so
STATIC = $(LIBNAME).a
#
C_SRCS += $(SAMPLE_COMMON_DIR)/sample_common_sys.c
C_SRCS += $(SAMPLE_COMMON_DIR)/sample_common_platform.c
C_SRCS += $(SAMPLE_COMMON_DIR)/sample_common_vpss.c
C_SRCS += $(SAMPLE_COMMON_DIR)/sample_common_vi.c
C_SRCS += $(SAMPLE_COMMON_DIR)/sample_common_isp.c
C_SRCS += $(SAMPLE_COMMON_DIR)/sample_common_venc.c
C_SRCS += $(SAMPLE_COMMON_DIR)/sample_common_bin.c
C_SRCS += $(SAMPLE_COMMON_DIR)/sample_common_vo.c
C_SRCS += $(SAMPLE_COMMON_DIR)/sample_common_sensor.c
C_SRCS += $(SAMPLE_COMMON_DIR)/sample_common_peripheral.c
C_OBJS = $(C_SRCS:.c=.o)
CXX_SRCS = $(filter-out venc_json.cpp, $(wildcard *.cpp))
CXX_OBJS = $(CXX_SRCS:.cpp=.o)
#
INNER_INC = -I../include
INNER_LIB = -L ../src -lcvi_rtsp
#
JSON_INC ?= -I./3rdparty/nlohmann_json/include
#
ifneq ($(findstring $(CHIP_ARCH), MARS mars),)
CFLAGS += -DCHIP_ARCH_MARS
else ifneq ($(findstring $(CHIP_ARCH), PHOBOS phobos),)
CFLAGS += -DCHIP_ARCH_PHOBOS
endif
#
CFLAGS += $(INNER_INC) $(CVI_RTSP_INC) $(SENSOR_LIST_INC) $(ISP_INC) $(MW_INC) $(JSON_INC) $(AISDK_INC) $(LIVE555_INC)
LDFLAGS += -lpthread -ldl $(MW_LIB) $(INNER_LIB) $(VENC_JSON_LIB)
CPPFLAGS += $(CFLAGS)

.PHONY: all clean install

all: $(SHARED) $(STATIC) $(VENC_JSON_LIB)

$(SHARED): $(C_OBJS) $(CXX_OBJS) $(VENC_JSON_LIB)
	$(CXX) $(CFLAGS) $(LDFLAGS) -shared -o $@ $^

$(STATIC): $(C_OBJS) $(CXX_OBJS) $(VENC_JSON_LIB)
	$(AR) $(ARFLAGS) $@ $^ $(LIVE555_LIB) $(VENC_JSON_LIB)
	$(RANLIB) $@

$(VENC_JSON_LIB): venc_json.o
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@

clean:
	rm -f *.o *.so *.a

install:
	install -D $(SHARED) $(DESTDIR)/lib/$(SHARED)
	install -D $(STATIC) $(DESTDIR)/lib/$(STATIC)
	install -D $(VENC_JSON_LIB) $(DESTDIR)/lib/$(VENC_JSON_LIB)
