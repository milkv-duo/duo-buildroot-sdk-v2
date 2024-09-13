include Makefile.inc

SUBDIR = src
TMP_DIR ?= $(DESTDIR)/../cvi_rtsp

ifeq ($(BUILD_SERVICE),1)
SUBDIR += service
endif

ifeq ($(BUILD_EXAMPLE),1)
SUBDIR += example
endif

.PHONY: all clean install $(SUBDIR)

all: $(SUBDIR)

$(SUBDIR):
	$(MAKE) -C $@ $(MAKECMDGOALS)

clean: $(SUBDIR)
	rm -rf install

install: $(SUBDIR)
	@mkdir -p $(DESTDIR)/include/cvi_rtsp
	@cp -r include/cvi_rtsp $(DESTDIR)/include
ifeq ($(BUILD_SERVICE),1)
	@cp -r include/cvi_rtsp_service $(DESTDIR)/include
endif

package:
	@mkdir -p $(TMP_DIR)
	@cp -r $(DESTDIR)/* $(TMP_DIR)
	@cp -r cvi_models $(TMP_DIR)
	@tar zcf cvi_rtsp.tar.gz $(TMP_DIR)
	@rm $(TMP_DIR) -rf
