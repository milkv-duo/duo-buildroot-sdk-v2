SHELL = /bin/bash
SDK_VER? := 64bit

ifeq ($(CHIP_ARCH), MARS)
	CHIP_ARCH:=CV181X
else ifeq ($(CHIP_ARCH), PHOBOS)
	CHIP_ARCH:=CV180X
endif

ifeq ($(PARAM_FILE), )
     PARAM_FILE:=Makefile.param
     include $(PARAM_FILE)
endif

CHIP_DIR := $(MW_PATH)/chips/$(shell echo $(CHIP_ARCH) | tr A-Z a-z)

ifeq ($(DESTDIR),)
    DESTDIR := $(shell pwd)/install
endif

define package_mw
    @install -d $(1)
    @tar -c --exclude='chips' --exclude='modules' --exclude='$(1)' --exclude='.git' --exclude='tool' --exclude='self_test' --exclude='component/panel/cv1835/customer' -f - . | (cd $(1); tar xf -)
    @rm $(1)/3rdparty/gtest -r
    @rm $(1)/sample/ -r
    @make -C sample/ clean
    @cp sample $(1)/sample -r
endef

.PHONY: clean all module component sample install uninstall package 3rdparty

all: prepare 3rdparty module component sample

module: prepare 3rdparty
	@make -C modules/

prepare:
	@if [ -d mod_tmp ]; then mv mod_tmp modules; fi;
	@mkdir -p include/isp
	@cp -rf modules/isp/include/* include/isp
ifeq ($(CHIP_ARCH), $(filter $(CHIP_ARCH), CV181X CV180X))
	@cp -rf modules/isp/cv181x/isp-daemon2/inc/cvi_ispd2.h include/isp/cv181x/
endif
3rdparty:
	@make -C 3rdparty/

sample: module component
	@mv modules mod_tmp
	@make -C sample/ || mv mod_tmp modules
	@mv mod_tmp modules

component:
	@make -C component/isp/ all


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
endif

ifneq ($(FLASH_SIZE_SHRINK),y)
	# copy others
	@cp -f sample/sensor_test/sensor_test $(DESTDIR)/usr/bin
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
	@if [ -d mod_tmp ]; then mv mod_tmp modules; fi;
ifeq ($(CHIP_ARCH), $(filter $(CHIP_ARCH), CV180X CV181X CV182X))
else
	@rm -f $(MW_INC)/cvi_defines.h
endif
	@rm -rf include/isp
	@make -C modules/ clean
	@make -C 3rdparty/ clean
	@make -C sample/ clean
	@make -C component/isp/ clean
