include ../Makefile.inc

LIBNAME = libcvi_rtsp
SHARED = $(LIBNAME).so
STATIC = $(LIBNAME).a

CXX_SRCS = $(wildcard *.cpp)
CXX_OBJS = $(CXX_SRCS:.cpp=.o)

INNER_INC = -I../include

CFLAGS += $(INNER_INC) $(LIVE555_INC)
LDFLAGS += $(LIVE555_LIB)
CPPFLAGS += $(CFLAGS)

.PHONY: all clean install

all: $(SHARED) $(STATIC)

$(SHARED): $(CXX_OBJS)
	$(CXX) $(CFLAGS) -shared -o $@ $^ $(LDFLAGS)

$(STATIC): $(CXX_OBJS)
	$(AR) $(ARFLAGS) $@ $^ $(LIVE555_LIB)
	$(RANLIB) $@

clean:
	rm -f *.o *.so *.a

install:
	install -D $(SHARED) $(DESTDIR)/lib/$(SHARED)
	install -D $(STATIC) $(DESTDIR)/lib/$(STATIC)
