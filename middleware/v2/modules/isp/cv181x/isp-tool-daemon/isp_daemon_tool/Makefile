SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../../../../../$(shell echo $(MW_VER))/Makefile.param
	include $(PARAM_FILE)
endif

include Makefile.releaseList
include Makefile.cvirtsp
include ../../../isp.mk

# STREAM_TYPE 2:live555 3:normal output to panel
STREAM_TYPE ?= 2

# For STREAM_TYPE=3
# VPSS Out
VPSS_OUT_WIDTH ?= 1280
VPSS_OUT_HEIGHT ?= 720
SDIR = $(PWD)/src
TMP_FOLDER = tmp
ISP_COMMON_DIR = $(PWD)/../../../common

ifeq ($(DESTDIR),)
	DESTDIR := $(shell pwd)/install
endif

INCS = -I$(KERNEL_INC) -I$(MW_INC) -I$(ISP_INC) -Iinclude -I$(MW_PATH)/sample -I$(CVI_ISPD2_PATH)
INCS += -I$(ISP_COMMON_DIR)/raw_dump/inc

OBJS = $(patsubst $(SDIR)/%.cpp, $(TMP_FOLDER)/%.o, $(wildcard $(SDIR)/*.cpp))
COBJS = $(patsubst $(SDIR)/%.c, $(TMP_FOLDER)/%.o, $(wildcard $(SDIR)/*.c))
DEPS = $(patsubst $(SDIR)/%.cpp, $(TMP_FOLDER)/%.d, $(wildcard $(SDIR)/*.cpp))
CDEPS = $(patsubst $(SDIR)/%.c, $(TMP_FOLDER)/%.d, $(wildcard $(SDIR)/*.c))
AI_SDK_PATH = $(OUTPUT_DIR)/tpu_$(SDK_VER)/cvitek_ai_sdk
TPU_SDK_PATH = $(OUTPUT_DIR)/tpu_$(SDK_VER)/cvitek_tpu_sdk

AI_SDK_PATH_EXIST = $(shell if [ -d $(AI_SDK_PATH) ]; then echo "exist"; else echo "noexist"; fi)
TPU_SDK_PATH_EXIST = $(shell if [ -d $(TPU_SDK_PATH) ]; then echo "exist"; else echo "noexist"; fi)

TARGET = isp_tool_daemon
OUT_TARBALL = isp_tool_daemon.tar.gz

PKG_CONFIG_PATH = $(MW_PATH)/pkgconfig
REQUIRES = cvi_common cvi_sample cvi_vdec cvi_misc
MW_LIBS = $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs --define-variable=mw_dir=$(MW_PATH) $(REQUIRES))

LIBS += $(MW_LIBS) -latomic
LIBS += -lsys -lvenc -lvdec -lvpu -lisp -lisp_algo -lawb -lae -laf -lsample -lcvi_bin -lcvi_bin_isp -lsns_full -lini
LIBS += -ljson-c -lcvi_ispd2
LIBS += -lraw_dump
LIBS += -lcvi_dnvqe -lcvi_ssp2

LOCAL_CFLAGS = $(DEFS) $(INCS) -DSDK_VER=$(SDK_VER)
LOCAL_CPPFLAGS = -fno-use-cxa-atexit
ifeq ($(SDK_VER), uclibc)
LOCAL_LDFLAGS = $(LIBS) -Wl,-rpath-link=$(CVI_RTSP_LIB_PATH) -L$(CVI_ISPD2_PATH) -L$(CVI_RTSP_LIB_PATH) -lpthread
else
LOCAL_LDFLAGS = $(LIBS) -Wl,-rpath-link=$(CVI_RTSP_LIB_PATH) -L$(CVI_ISPD2_PATH) -L$(CVI_RTSP_LIB_PATH) -lm -lpthread
endif

ifeq ($(STREAM_TYPE), 2)
INCS += -I$(CVI_RTSP_INC) -I$(CVI_RTSP_SERVICE_INC)
LIBS += -L/source/cvi_rtsp/src -L/source/cvi_rtsp/service -ldl -lcvi_rtsp -lcvi_rtsp_service
DEFS += -DSTREAM_TYPE=2
else
LIBS += -ldl
DEFS += -DVPSS_OUT_WIDTH=$(VPSS_OUT_WIDTH) -DVPSS_OUT_HEIGHT=$(VPSS_OUT_HEIGHT)
DEFS += -DSTREAM_TYPE=3
endif

.PHONY: clean all package install uninstall sdk_release
ifeq ($(ISP_SRC_RELEASE),1)
all: sdk_release
else
all: package
endif

prepare: | $(TMP_FOLDER)
ifeq ($(STREAM_TYPE), 2)
ifneq "$(wildcard $(CVI_RTSP_TARBALL_PATH))" ""
	@tar -zxf $(CVI_RTSP_TARBALL_PATH) -C $(TMP_FOLDER)
else
	$(error Can't find $(CVI_RTSP_PATH))
endif
endif

$(TMP_FOLDER)/%.o: $(SDIR)/%.cpp | prepare $(TMP_FOLDER)
	@$(CXX) $(CXXFLAGS) $(LOCAL_CFLAGS) $(LOCAL_CPPFLAGS) -c $< -o $@
	@echo [$(notdir $(CXX))] $(notdir $@)

$(TMP_FOLDER)/%.o: $(SDIR)/%.c | prepare $(TMP_FOLDER)
	@$(CC) $(CFLAGS) $(LOCAL_CFLAGS) -c $< -o $@
	@echo [$(notdir $(CC))] $(notdir $@)

$(TARGET): $(OBJS) $(COBJS)
	@$(CXX) -o $@ $(OBJS) $(COBJS) $(ELFFLAGS) $(LOCAL_LDFLAGS)
	@echo -e $(BLUE)[LINK]$(END)[$(notdir $(CXX))] $(notdir $@)

$(TMP_FOLDER):
	-@mkdir -p $(TMP_FOLDER)

package: $(TARGET)
	@rm -rf install/*
	@mkdir -p install/lib
	@cp $(TARGET) install/
	@cp ../CviIspTool.sh install/
	@cp ../config.ini install/
	@cp ../cfg_64M.json install/
	@cp ../cfg_128M.json install/
	@cp ../vc_param_64M.json install/
	@cp ../vc_param_128M.json install/

	@mkdir install/lib/ai
ifeq "$(AI_SDK_PATH_EXIST)" "noexist"
	$(info cvitek_ai_sdk not exist, please export TPU_REL=1 run build_all !!)
else ifeq "$(TPU_SDK_PATH_EXIST)" "noexist"
	$(info cvitek_tpu_sdk not exist, please export TPU_REL=1 run build_all !!)
else
	@cp $(AI_SDK_PATH)/lib/*.so* install/lib/ai -rf
	@cp $(TPU_SDK_PATH)/lib/*.so* install/lib/ai -rf
	@rm install/lib/ai/libcvi_rtsp.so
endif

	@for so in $(file); \
	do \
		cp -Lrf $$so install/lib > /dev/null 2>&1; \
	done

ifeq ($(STREAM_TYPE), 2)
	@echo "Pack with live555 related files ..."
	@cp -arf $(CVI_RTSP_OUT_PATH)/* install

	@for data in $(rtsp_lib_nonused); \
	do \
		rm -rf install/$$data; \
	done
endif
	@sed -i 's/isEnableSetSnsCfgPath = 0/isEnableSetSnsCfgPath = 1/g' install/config.ini
	@tar -zcf ../$(OUT_TARBALL) install
	@sed -i 's/isEnableSetSnsCfgPath = 1/isEnableSetSnsCfgPath = 0/g' install/config.ini

install:
	@mkdir -p $(DESTDIR)/usr/bin
	@mkdir -p $(DESTDIR)/usr/lib
	@cp -f install/$(TARGET) $(DESTDIR)/usr/bin
	@cp -f install/CviIspTool.sh $(DESTDIR)/usr/bin
	@cp -f install/config.ini $(DESTDIR)/usr/bin
	@cp -f install/cfg_64M.json $(DESTDIR)/usr/bin
	@cp -f install/cfg_128M.json $(DESTDIR)/usr/bin
	@cp -f install/vc_param_64M.json $(DESTDIR)/usr/bin
	@cp -f install/vc_param_128M.json $(DESTDIR)/usr/bin

	-@cp -f install/pqtool_definition.json $(DESTDIR)/usr/bin


uninstall:
	-@rm -f $(DESTDIR)/usr/bin/$(TARGET)
	-@rm -f $(DESTDIR)/usr/bin/CviIspTool.sh
	-@rm -f $(DESTDIR)/usr/bin/config.ini
	-@rm -f $(DESTDIR)/usr/bin/cfg_64M.json
	-@rm -f $(DESTDIR)/usr/bin/cfg_128M.json
	-@rm -f $(DESTDIR)/usr/bin/vc_param_64M.json
	-@rm -f $(DESTDIR)/usr/bin/vc_param_128M.json	
	-@rm -f $(DESTDIR)/usr/bin/pqtool_definition.json

sdk_release: package
	-@rm -rf include src tmp
	-@mv Makefile.sdk_release Makefile
	-@rm -f Makefile.*

	-@rm -f ../cfg_64M.json ../cfg_128M.json ../vc_param_64M.json ../vc_param_128M.json ../config.ini ../CviIspTool.sh ../README.md ../.gitignore
	-@rm -rf ../test

clean:
	@rm -f $(OBJS) $(DEPS) $(COBJS) $(CDEPS) $(TARGET)
	@rm -rf install $(TMP_FOLDER)
	@rm -rf ../$(OUT_TARBALL)
-include $(DEPS) $(CDEPS)
