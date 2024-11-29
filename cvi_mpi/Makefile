SHELL = /bin/bash
#
#
PARAM_FILE := $(CURDIR)/mpi_param.mk
include $(PARAM_FILE)
#
.PHONY: clean all module component sample install uninstall package 3rdparty self_test clean_all

all: prepare 3rdparty module component sample self_test

module: prepare 3rdparty
	@make -C modules/

prepare:
	@if [ -f "mpi_prepare.mk" ]; then \
		make -f mpi_prepare.mk prepare; \
	fi

3rdparty:
	@if [ -d "3rdparty/" ]; then \
		make -C 3rdparty/; \
	fi

sample: module component
	@if [ -d "sample/" ]; then \
		make -C sample/; \
	fi

self_test: module sample
	@if [ -d "self_test/" ]; then \
		make -C self_test/; \
	fi

component:
	@make -C component/isp/ all

dl_daemon: sample
	@cmake self_test/dl_daemon -Bself_test/dl_daemon/build
	@make -C self_test/dl_daemon/build/

ifneq ($(SUBTYPE), fpga)
install:
	@mkdir -p $(DESTDIR)/usr/bin
	@mkdir -p $(DESTDIR)/usr/lib/3rd
ifneq ($(FLASH_SIZE_SHRINK),y)
	# copy sample_xxx
	@cp -f sample/audio/sample_audio $(DESTDIR)/usr/bin
	@cp -f sample/sample_panel/sample_panel $(DESTDIR)/usr/bin
	@cp -f sample/venc/sample_venc $(DESTDIR)/usr/bin
	@cp -f sample/venc/sample_vcodec $(DESTDIR)/usr/bin
	@cp -f sample/vdec/sample_vdec $(DESTDIR)/usr/bin
	@cp -f sample/vio/sample_vio $(DESTDIR)/usr/bin
	@cp -f sample/sensor_test/sample_sensor_test $(DESTDIR)/usr/bin

	@if [ -d self_test ]; then \
		cp -f self_test/cvi_test/cvi_test $(DESTDIR)/usr/bin; \
		cp -f self_test/cvi_test/sensor_cfg.ini $(DESTDIR)/usr/bin; \
	fi
endif

ifneq ($(FLASH_SIZE_SHRINK),y)
	# copy others
	@cp -f sample/ir_auto/ir_auto $(DESTDIR)/usr/bin
endif

ifneq ($(FLASH_SIZE_SHRINK),y)
	# copy audio libs and elf
	@cp -f sample/audio/sample_audio*  $(DESTDIR)/usr/bin
	@if [ -e "sample/audio/cvi_mp3player" ]; then cp -f sample/audio/cvi_mp3player $(DESTDIR)/usr/bin; fi
endif

	# copy mw lib
	@cp -a lib/*.so*  $(DESTDIR)/usr/lib
	@cp -a lib/3rd/*.so*  $(DESTDIR)/usr/lib/3rd

uninstall:
	@rm $(DESTDIR) -rf

package:
	$(call package_mw,tmp)
	@install -d $(DESTDIR)
	@tar fcz $(DESTDIR)/mw.tar.gz -C tmp .
	@echo $(KERNEL_INC)
	@tar fcz $(DESTDIR)/kernel_header.tar.gz -C $(KERNEL_INC) ./
	@rm tmp -r
	@echo "package done"
endif

clean:
	@make -C modules/ clean
	@make -C 3rdparty/ clean
	@make -C sample/ clean
	@make -C component/isp/ clean
	@rm -rf self_test/dl_daemon/build
	@if [ -d "self_test/" ]; then \
		make -C self_test/ clean; \
	fi
	@if [ -f "mpi_prepare.mk" ]; then \
		make -f mpi_prepare.mk clean; \
	fi

clean_all:
	@make -C modules/ clean_all
