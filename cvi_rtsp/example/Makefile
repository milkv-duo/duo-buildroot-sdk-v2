include ../Makefile.inc

SRC = $(wildcard *.cpp)
BIN=$(SRC:.cpp=)

INNER_INC = -I../include -I$(CVI_TRACER_INC)
INNER_LIB = -L../src -lcvi_rtsp -L../service -lcvi_rtsp_service

CFLAGS += $(INNER_INC) $(MW_INC)
LDFLAGS += -lpthread $(MW_LIB) $(MW_AUDIO_LIB) $(INNER_LIB)

ifeq ($(BUILD_FILEMODE), 1)
CVI_DEMUXER_DIR ?= ../../framework/framework/components/demuxer
CVI_DEMUXER_INC ?= -I$(CVI_DEMUXER_DIR)/include
CVI_DEMUXER_LIB ?= -L$(CVI_DEMUXER_DIR)/build -lcvi_demuxer -L$(CVI_DEMUXER_DIR)/../../../cpsl/common/log/lib -lcvi_log

FFMPEG_DIR ?= ../../framework/cpsl/third_party/ffmpeg
FFMPEG_INC ?= -I$(FFMPEG_DIR)/include
FFMPEG_LIB ?= -L$(FFMPEG_DIR)/lib -lavformat -lavcodec -lavutil

CFLAGS += $(CVI_DEMUXER_INC) $(FFMPEG_INC) -DBUILD_FILEMODE
LDFLAGS += $(CVI_DEMUXER_LIB) $(FFMPEG_LIB)
endif

CPPFLAGS += $(CFLAGS)

all: $(BIN)

$(BIN): % : %.cpp
	$(CXX) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o $(BIN)

install:
	install -D $(BIN) -t $(DESTDIR)/example/
