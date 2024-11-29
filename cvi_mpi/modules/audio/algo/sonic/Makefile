# This file was written by Bill Cox in 2010, and is licensed under the Apache
# 2.0 license.
#
# Note that -pthread is only included so that older Linux builds will be thread
# safe.  We call malloc, and older Linux versions only linked in the thread-safe
# malloc if -pthread is specified.

# Uncomment this if you want to link in spectrogram generation.  It is not
# needed to adjust speech speed or pitch.  It is included primarily to provide
# high-quality spectrograms with low CPU overhead, for applications such a
# speech recognition.
#USE_SPECTROGRAM=1


#This file is modify only for embedded board test, not for any profitable usage or any sales behavior
#We conly use this for interest without any further modification in original code.
#All rights reserved to original SONIC algorithm
#download data: 2021/11/2  download version 0.3.0

SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
PARAM_FILE:=../../../../mpi_param.mk
include $(PARAM_FILE)
endif

AUDIO_PARAM_FILE:=../../audio.mk
include $(AUDIO_PARAM_FILE)

PREFIX=$(PWD)/install

#UNAME := $(shell uname)
#ifeq ($(UNAME), Darwin)
#  PREFIX=/usr/local
#endif

BINDIR=$(PREFIX)/bin
LIBDIR=$(PREFIX)/lib
INCDIR=$(PREFIX)/include

#SONAME=$(LIBDIR)/
##SHARED_OPT= -shared
#SHARED_OPT=
#LIB_NAME=libsonic.so
#LIB_TAG=

SONAME=-soname,
SHARED_OPT=-shared
#LIB_NAME=$(LIBDIR)/libsonic.so
#LIB_TAG=.0.3.0
LIB_NAME=$(MW_LIB)/libsonic.so
LIB_TAG=

#ifeq ($(UNAME), Darwin)
#  SONAME=-install_name,$(LIBDIR)/
#  SHARED_OPT=-dynamiclib
#  LIB_NAME=libsonic.so
#  LIB_TAG=
#endif


#CFLAGS ?= -O3
#CFLAGS += -Wall -Wno-unused-function -ansi -fPIC -pthread
#CC=gcc

# Set NO_MALLOC=1 as a parameter to make to compile Sonic with static buffers
# instead of calling malloc.  This is usefule primarily on microcontrollers.
ifeq ($(NO_MALLOC), 1)
  CFLAGS+= -DSONIC_NO_MALLOC
  # Set MAX_MEMORY=<memory size> if you need to incease the static memory buffer
  ifdef MAX_MEMORY
    CFLAGS+= -DSONIC_MAX_MEMORY=$(MAX_MEMORY)
  else
    CFLAGS+= -DSONIC_MAX_MEMORY=4096
  endif
endif

ifdef NO_PITCH
  CFLAGS+= -DSONIC_NO_PITCH
endif

ifdef MIN_PITCH
  CFLAGS+= -DSONIC_MIN_PITCH=$(MIN_PITCH)
endif

SRC=sonic.c
# Set this to empty if not using spectrograms.
FFTLIB=
ifeq ($(USE_SPECTROGRAM), 1)
  CFLAGS+= -DSONIC_SPECTROGRAM
  SRC+= spectrogram.c
  FFTLIB= -L$(LIBDIR) -lfftw3
endif
OBJ=$(SRC:.c=.o)



all: cvi_sonic sonic sonic_lite $(LIB_NAME)$(LIB_TAG) libsonic.a

cvi_sonic: wave.o cvi_main.o libsonic.a
	$(CC) $(CFLAGS) -o cvi_sonic wave.o cvi_main.o libsonic.a -lm $(FFTLIB)
	@echo "-->cvi_sonic"
	@echo [$(notdir $(CC))] $(notdir $@)

sonic: wave.o main.o libsonic.a
#	$(CC) $(CFLAGS) $(LDFLAGS) -o sonic wave.o main.o libsonic.a -lm $(FFTLIB)
	$(CC) $(CFLAGS) -o sonic wave.o main.o libsonic.a -lm $(FFTLIB)
	@echo "-->sonic"
	@echo [$(notdir $(CC))] $(notdir $@)

sonic_lite: wave.c main_lite.c sonic_lite.c sonic_lite.h
#	$(CC) $(CFLAGS) $(LDFLAGS) -o sonic_lite sonic_lite.c wave.c main_lite.c
	$(CC) $(CFLAGS) -o sonic_lite sonic_lite.c wave.c main_lite.c
	@echo "-->sonic_lite"
	@echo [$(notdir $(CC))] $(notdir $@)

sonic.o: sonic.c sonic.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c sonic.c
	@echo "-->sonic.o"
	@echo [$(notdir $(CC))] $(notdir $@)

wave.o: wave.c wave.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c wave.c
	@echo "-->wave.o"
	@echo [$(notdir $(CC))] $(notdir $@)

main.o: main.c sonic.h wave.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c main.c
	@echo "-->main.o"
	@echo [$(notdir $(CC))] $(notdir $@)

spectrogram.o: spectrogram.c sonic.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c spectrogram.c
	@echo "-->sonspectrogramic.o"
	@echo [$(notdir $(CC))] $(notdir $@)

$(LIB_NAME)$(LIB_TAG): $(OBJ)
#	$(CC) $(CFLAGS) $(LDFLAGS) $(SHARED_OPT) -Wl,$(SONAME)$(LIB_NAME) $(OBJ) -o $(LIB_NAME)$(LIB_TAG) $(FFTLIB)
	$(CC) $(CFLAGS) $(SHARED_OPT) -Wl,$(SONAME)$(LIB_NAME) $(OBJ) -o $(LIB_NAME)$(LIB_TAG) $(FFTLIB)

#ifneq ($(UNAME), Darwin)
#	ln -sf $(LIB_NAME)$(LIB_TAG) libsonic.so
#	ln -sf $(LIB_NAME)$(LIB_TAG) libsonic.so.0
#endif
	@echo "-->LIB_NAME.o"
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $@)

libsonic.a: $(OBJ)
	$(AR) cqs libsonic.a $(OBJ)
	@echo "libsonic.a---->"
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $@)

install: sonic $(LIB_NAME)$(LIB_TAG) sonic.h
	install -d $(DESTDIR)$(BINDIR) $(DESTDIR)$(INCDIR) $(DESTDIR)$(LIBDIR)
	install sonic $(DESTDIR)$(BINDIR)
	install sonic.h $(DESTDIR)$(INCDIR)
	install libsonic.a $(DESTDIR)$(LIBDIR)
	install $(LIB_NAME)$(LIB_TAG) $(DESTDIR)$(LIBDIR)
#ifneq ($(UNAME), Darwin)
#	ln -sf $(LIB_NAME)$(LIB_TAG) $(DESTDIR)$(LIBDIR)/$(LIB_NAME)
#	ln -sf $(LIB_NAME)$(LIB_TAG) $(DESTDIR)$(LIBDIR)/$(LIB_NAME).0
#endif
	@echo "install finish:---->"

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/sonic
	rm -f $(DESTDIR)$(INCDIR)/sonic.h
	rm -f $(DESTDIR)$(LIBDIR)/libsonic.a
	rm -f $(DESTDIR)$(LIBDIR)/$(LIB_NAME)$(LIB_TAG)
	rm -f $(DESTDIR)$(LIBDIR)/$(LIB_NAME).0
	rm -f $(DESTDIR)$(LIBDIR)/$(LIB_NAME)

clean:
	rm -f *.o sonic sonic_lite $(LIB_NAME)* libsonic.a test.wav cvi_sonic

check:
	./sonic -s 2.0 ./samples/talking.wav ./test.wav
