.PHONY: clean all
.SILENT: clean all cppcheck

ISP_BASE_PATH = $(PWD)/..
ISP_COMMON_DIR = $(ISP_BASE_PATH)/common
ISP_3A_DIR = $(ISP_BASE_PATH)/algo

all:
	@cd isp; $(MAKE) all;
	@cd isp_bin; $(MAKE) all;
	@cd $(ISP_COMMON_DIR)/toolJsonGenerator; ./generate_toolJson.sh cv181x
	@cd isp-daemon2; $(MAKE) -j4; $(MAKE) install;
	@cd $(ISP_COMMON_DIR)/raw_replay; $(MAKE) all;
	@cd $(ISP_COMMON_DIR)/raw_dump; $(MAKE) all;

ifeq ($(ISP_SRC_RELEASE),1)
	# common base
	# Remove not released folder (in cv182x)
	-@rm -rf $(CV182X_COMMON_BASE_PATH)/inc $(CV182X_COMMON_BASE_PATH)/isp-rpc-tool-daemon $(CV182X_COMMON_BASE_PATH)/rpc
	-@rm -rf $(CV182X_COMMON_BASE_PATH)/raw_dump_test $(CV182X_COMMON_BASE_PATH)/raw_replay_test

	# Remove not released folder (in cv183x)
	-@rm -rf $(CV183X_COMMON_BASE_PATH)/inc $(CV183X_COMMON_BASE_PATH)/isp-rpc-tool-daemon $(CV183X_COMMON_BASE_PATH)/rpc
	-@rm -rf $(CV183X_COMMON_BASE_PATH)/raw_dump_test $(CV183X_COMMON_BASE_PATH)/raw_replay_test

	# Remove other SOCs (cv183x)
	-@rm -f $(CV183X_BASE_PATH)/Makefile*
	-@rm -rf $(CV183X_BASE_PATH)/include $(CV183X_BASE_PATH)/src
	-@rm -rf $(CV183X_BASE_PATH)/isp-daemon $(CV183X_BASE_PATH)/isp-tool-daemon
	-@rm -rf $(CV183X_BASE_PATH)/isp_bin $(CV183X_BASE_PATH)/isp_test
	-@rm -rf $(ISP_BASE_PATH)/include/cv183x

	# Replace makefile
	-@mv Makefile.sdk_release Makefile
endif

cppcheck:
	@cppcheck --enable=all --quiet --template='{file},{line},{severity},{id}:{message}' -I../include/cv182x -I./isp/inc -I./isp_algo/inc isp/src/ isp_algo/src/ 2>cppcheck_output.txt
	@python3 $(MW_PATH)/tool/cppcheckparser/cppcheckparser.py --input cppcheck_output.txt --output cppcheck_output.xlsx

clean:
	@cd isp-daemon2; $(MAKE) clean;
	@cd isp; $(MAKE) clean;
	@cd isp_bin; $(MAKE) clean;
	@cd $(ISP_COMMON_DIR)/raw_replay; $(MAKE) clean;
	@cd $(ISP_COMMON_DIR)/raw_dump; $(MAKE) clean;
