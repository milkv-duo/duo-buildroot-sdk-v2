include ../Makefile.inc

SRC = $(wildcard ./src/*.cpp)
BIN = $(SRC:.cpp=)

INNER_INC = -I../include
INNER_LIB = -L../src -lcvi_rtsp -L../service -lcvi_rtsp_service

CFLAGS += $(INNER_INC) $(LIVE555_INC)
LDFLAGS += $(INNER_LIB) $(MW_LIB) $(LIVE555_LIB) $(MW_AUDIO_LIB)

CPPFLAGS += $(CFLAGS)

all: $(BIN)

$(BIN): % : %.cpp
	$(CXX) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o $(BIN)

install:
	install -D $(BIN) -t $(DESTDIR)/test/
	install -D test_cvi_rtsp.sh $(DESTDIR)/test/
	install -D test_streaming.sh $(DESTDIR)/test/
	install -D *.json $(DESTDIR)/test/
