SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../../../../$(shell echo $(MW_VER))/Makefile.param
	include $(PARAM_FILE)
endif
UV_DUMMY=y
BASE_DIR = $(PWD)
ISP_DIR = $(BASE_DIR)/..
ISP_COMMON_DIR = $(BASE_DIR)/../../common
ISP_3A_DIR = $(BASE_DIR)/../../algo

# 3rdparty lib
OSS_MODULE_INSTALL_PATH = $(OUTPUT_DIR)/mw_$(SDK_VER)/cvitek_mw_sdk
OSS_MODULE_INCLUDE_PATH = $(OSS_MODULE_INSTALL_PATH)/include
OSS_MODULE_LIB_PATH = $(OSS_MODULE_INSTALL_PATH)/lib

INC_DIRS = $(KERNEL_INC) $(MW_INC) $(ISP_INC) $(ISP_DIR)/isp/inc
ifeq ($(UV_DUMMY), y)
INC_DIRS += $(BASE_DIR)/inc $(BASE_DIR)/src
INC_DIRS += $(OSS_MODULE_INCLUDE_PATH) $(OSS_MODULE_INCLUDE_PATH)/json-c
else
INC_DIRS += $(OSS_MODULE_INCLUDE_PATH) $(OSS_MODULE_INCLUDE_PATH)/json-c
INC_DIRS += $(BASE_DIR)/inc $(BASE_DIR)/src
endif
INC_DIRS += $(ISP_COMMON_DIR)/raw_replay
INC_DIRS += $(ISP_DIR)/isp_bin/src		# isp_json_struct_local.h
INC_DIRS += $(ISP_3A_DIR)

LIB_DIRS = $(MW_LIB) $(OSS_MODULE_LIB_PATH)
ifeq ($(UV_DUMMY), y)
LIB_MODULES = json-c
else
LIB_MODULES = json-c uv
endif

CINCS = $(foreach incdir, $(INC_DIRS), -I$(incdir))
LINCS = $(foreach libdir, $(LIB_DIRS), -L$(libdir))
LIBS = $(foreach libmodule, $(LIB_MODULES), -l$(libmodule))

TMP_FOLDER = tmp
OUT_FOLDER = build
RELEASE_FOLDER_BIN = $(PWD)/bin

#USRDEFS = -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable

TARGET_A = libcvi_ispd2.a
TARGET_SO = libcvi_ispd2.so

LOCAL_CFLAGS = $(CINCS) $(USRDEFS) -rdynamic -fPIC
#LOCAL_CFLAGS += $(DEPFLAGS)
LOCAL_LDFLAGS = $(LINCS) $(LIBS) -shared

# SOC Architecture
ifeq ($(CHIP_ARCH), CV183X)
	LOCAL_CFLAGS += -DCHIP_ARCH_CV183X
else ifeq ($(CHIP_ARCH), CV182X)
	LOCAL_CFLAGS += -DCHIP_ARCH_CV182X
else ifeq ($(CHIP_ARCH), CV181X)
	LOCAL_CFLAGS += -DCHIP_ARCH_CV181X
else ifeq ($(CHIP_ARCH), CV180X)
	LOCAL_CFLAGS += -DCHIP_ARCH_CV180X
else
	$(error Un-support SOC Architecture)
endif

.PHONY: all clean cleanall install uninstall upload prepare_3rdparty_module
.SILENT: clean cleanall install uninstall upload prepare_3rdparty_module
ifeq ($(UV_DUMMY), y)
EXCLUDES=
else
EXCLUDES=$(shell ls src/cvi_ispd2_uv_dummy.c)
endif

OBJS =	$(patsubst src/%.c, $(TMP_FOLDER)/%.o, $(filter-out $(EXCLUDES), $(wildcard src/*.c))) \
		$(patsubst %.c, $(TMP_FOLDER)/%.o, $(wildcard *.c))
DEPS =	$(patsubst src/%.c, $(TMP_FOLDER)/%.d, $(filter-out $(EXCLUDES), $(wildcard src/*.c))) \
		$(patsubst %.c, $(TMP_FOLDER)/%.d, $(wildcard *.c))

#ifeq ($(ISP_SRC_RELEASE),1)
#all: $(TARGET_A) $(TARGET_SO) sdk_release
#else
all: $(TARGET_A) $(TARGET_SO)
#endif

OSS_BUILD_SRCIPT = $(TOP_DIR)/oss/run_build.sh
OSS_TARBALL_PATH = $(OUTPUT_DIR)/tpu_$(SDK_VER)/third_party
OSS_TARBALL_JSONC_PATH = $(OSS_TARBALL_PATH)/json-c.tar.gz
OSS_TARBALL_UV_PATH = $(OSS_TARBALL_PATH)/uv.tar.gz
OSS_MODULE_LIB_JSONC_CHECK_PATH = $(OSS_MODULE_LIB_PATH)/libjson-c.so*
OSS_MODULE_LIB_UV_CHECK_PATH = $(OSS_MODULE_LIB_PATH)/libuv.so*

prepare_3rdparty_module:
# json-c
ifeq "$(wildcard $(OSS_MODULE_LIB_JSONC_CHECK_PATH))" ""
ifeq "$(wildcard $(OSS_TARBALL_JSONC_PATH))" ""
	$(error JSON-C source tarball not exist, please run build_3rd_party first!!)
endif
	@mkdir -p $(OSS_MODULE_INSTALL_PATH)
	@$(OSS_BUILD_SRCIPT) -n json-c -e -t "$(OSS_TARBALL_PATH)" -i "$(OSS_MODULE_INSTALL_PATH)"
endif

# uv
ifneq ($(UV_DUMMY), y)
ifeq "$(wildcard $(OSS_MODULE_LIB_UV_CHECK_PATH))" ""
ifeq "$(wildcard $(OSS_TARBALL_UV_PATH))" ""
	$(error libuv source tarball not exist, please run build_3rd_party first!!)
endif
	@mkdir -p $(OSS_MODULE_INSTALL_PATH)
	@$(OSS_BUILD_SRCIPT) -n uv -e -t "$(OSS_TARBALL_PATH)" -i "$(OSS_MODULE_INSTALL_PATH)"
endif
endif

$(TMP_FOLDER)/%.o: $(BASE_DIR)/src/%.c | prepare_3rdparty_module $(TMP_FOLDER)
	@$(CC) $(CFLAGS) $(LOCAL_CFLAGS) -c $< -o $@
	@echo [$(notdir $(CC))] $(notdir $@)

$(TMP_FOLDER)/%.o: $(BASE_DIR)/%.c | prepare_3rdparty_module $(TMP_FOLDER)
	@$(CC) $(CFLAGS) $(LOCAL_CFLAGS) -c $< -o $@
	@echo [$(notdir $(CC))] $(notdir $@)

$(TARGET_A): $(OBJS) | prepare_3rdparty_module $(OUT_FOLDER)
	@$(AR) $(ARFLAGS) $(OUT_FOLDER)/$(TARGET_A) $(OBJS)
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $(TARGET_A))
	@cp $(BASE_DIR)/inc/cvi_ispd2.h $(OUT_FOLDER)
ifneq ($(UV_DUMMY), y)
	@cp -a $(OSS_MODULE_LIB_PATH)/libuv_a.a $(OUT_FOLDER)
endif
	@cp -a $(OSS_MODULE_LIB_PATH)/libjson-c.a $(OUT_FOLDER)

$(TARGET_SO): $(OBJS) | prepare_3rdparty_module $(OUT_FOLDER)
	@$(LD) $(LDFLAGS) $(LOCAL_LDFLAGS) -o $(OUT_FOLDER)/$(TARGET_SO) --start-group $(OBJS) --end-group
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $(TARGET_SO))
	@cp $(BASE_DIR)/inc/cvi_ispd2.h $(OUT_FOLDER)
ifneq ($(UV_DUMMY), y)
	@cp -a $(OSS_MODULE_LIB_PATH)/libuv.so* $(OUT_FOLDER)
endif
	@cp -a $(OSS_MODULE_LIB_PATH)/libjson-c.so* $(OUT_FOLDER)

$(TMP_FOLDER):
	@echo "Create $(TMP_FOLDER) for object file"
	-@mkdir -p $(TMP_FOLDER)

$(OUT_FOLDER):
	@echo "Create $(OUT_FOLDER) for output"
	-@mkdir -p $(OUT_FOLDER)

upload:
	@sshpass -p "cvitek" scp $(OUT_FOLDER)/$(TARGET_SO) root@192.168.1.3:/mnt/data/install/lib
	@echo "Done"

install:
	@cp $(OUT_FOLDER)/$(TARGET_A) $(MW_LIB)
	@cp $(OUT_FOLDER)/$(TARGET_SO) $(MW_LIB)
ifneq ($(UV_DUMMY), y)
	@cp -a $(OUT_FOLDER)/libuv.so* $(MW_3RD_LIB)
endif
	@cp -a $(OUT_FOLDER)/libjson-c.so* $(MW_3RD_LIB)
	@cp -a $(OUT_FOLDER)/libjson-c.a $(MW_3RD_LIB)

uninstall:
	-@rm -f $(MW_LIB)/$(TARGET_A)
	-@rm -f $(MW_LIB)/$(TARGET_SO)
	-@rm -f $(MW_3RD_LIB)/libuv.so*
	-@rm -f $(MW_3RD_LIB)/libjson-c.so*

sdk_release: $(TARGET_A) $(TARGET_SO)
	@echo "Create $(RELEASE_FOLDER_BIN)"

	-@mkdir -p $(RELEASE_FOLDER_BIN)
	-@rm -rf $(RELEASE_FOLDER_BIN)/*
	-@cp $(OUT_FOLDER)/$(TARGET_A) $(RELEASE_FOLDER_BIN)
	-@cp $(OUT_FOLDER)/$(TARGET_SO) $(RELEASE_FOLDER_BIN)
ifneq ($(UV_DUMMY), y)
	-@cp -a $(OUT_FOLDER)/libuv_a.a $(RELEASE_FOLDER_BIN)
	-@cp -a $(OUT_FOLDER)/libuv.* $(RELEASE_FOLDER_BIN)
endif
	-@cp -a $(OUT_FOLDER)/libjson-c.* $(RELEASE_FOLDER_BIN)

	-@rm -f cppcheck.sh .gitignore .vscode
	-@rm -rf $(TMP_FOLDER)
	-@rm -rf src $(OUT_FOLDER)

	-@mv Makefile.sdk_release Makefile

clean:
	@rm -f $(OBJS) $(DEPS)
	@rm -f $(OUT_FOLDER)/$(TARGET_SO)
	@rm -f $(OUT_FOLDER)/$(TARGET_A)
	@rm -f $(OUT_FOLDER)/*.h
	@rm -f $(OUT_FOLDER)/libuv*.*
	@rm -f $(OUT_FOLDER)/libjson-c.*
	-@rm -f ./src/cvi_pqtool_json.h
	-@rm -f $(MW_LIB)/$(TARGET_SO)
	-@rm -f $(MW_LIB)/$(TARGET_A)
	-@rm -f $(MW_3RD_LIB)/libuv.so*
	-@rm -f $(MW_3RD_LIB)/libjson-c.so*

cleanall: clean
	@rm -rf $(TMP_FOLDER) $(OUT_FOLDER)
	@rm -rf $(RELEASE_FOLDER_BIN)

-include $(DEPS)
