.PHONY: clean all clean_all
.SILENT: clean all cppcheck

ISP_BASE_PATH = $(CURDIR)/..
ISP_COMMON_DIR = $(ISP_BASE_PATH)/common
ISP_3A_DIR = $(ISP_BASE_PATH)/algo

all:
	@cd isp_algo; $(MAKE) all;
	@cd $(ISP_3A_DIR); $(MAKE) all;
	@cd isp; $(MAKE) all;
	@cd isp_bin; $(MAKE) all;
	@cd $(ISP_COMMON_DIR)/toolJsonGenerator; ./generate_toolJson.sh $(shell echo $(CHIP_ARCH) | tr A-Z a-z)
	@cd isp-daemon2; $(MAKE) -j4; $(MAKE) install;
	@cd $(ISP_COMMON_DIR)/raw_replay; $(MAKE) all;
	@cd $(ISP_COMMON_DIR)/raw_dump; $(MAKE) all;

ifeq ($(ISP_SRC_RELEASE),1)
	# Replace makefile
	-@mv Makefile.sdk_release Makefile
endif

cppcheck:
	@cppcheck --enable=all --quiet --template='{file},{line},{severity},{id}:{message}' -I../include/cv181x -I./isp/inc -I./isp_algo/inc isp/src/ isp_algo/src/ 2>cppcheck_output.txt
	@python3 $(MW_PATH)/tool/cppcheckparser/cppcheckparser.py --input cppcheck_output.txt --output cppcheck_output.xlsx

clean:
	@cd isp-daemon2; $(MAKE) clean;
	@cd isp; $(MAKE) clean;
	@cd isp_bin; $(MAKE) clean;
	@cd $(ISP_3A_DIR); $(MAKE) clean;
	@cd isp_algo; $(MAKE) clean;
	@cd $(ISP_COMMON_DIR)/raw_replay; $(MAKE) clean;
	@cd $(ISP_COMMON_DIR)/raw_dump; $(MAKE) clean;

clean_all:
	@make -C  isp clean_all
	@make -C  isp_bin  clean_all
	@make -C  $(ISP_3A_DIR)  clean_all
	@make -C  isp_algo  clean_all
	@make -C  $(ISP_COMMON_DIR)/raw_replay  clean_all
	@make -C  $(ISP_COMMON_DIR)/raw_dump clean_all
