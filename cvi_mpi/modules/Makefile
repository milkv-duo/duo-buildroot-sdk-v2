ifeq ($(PARAM_FILE), )
     PARAM_FILE:=../mpi_param.mk
     include $(PARAM_FILE)
endif

.PHONY: all clean clean_all

SUB_DIRS = sys bin vi vo vpss rgn gdc audio mipi_tx venc vdec misc ive isp

all:
	@echo "########################################################################################################"
	@echo "#"
	@echo "#  Compiling 'module libs' Configs as below ..."
	@echo "#    "CROSS_COMPILE=$(CROSS_COMPILE)
	@echo "#"
	@echo "########################################################################################################"
	@$(foreach dir,$(SUB_DIRS), \
		if test -d $(dir) ; then \
			cd $(dir);$(MAKE) all || exit 1;cd -; \
		else \
			exit 1; \
		fi;)

clean:
	@$(foreach dir,$(SUB_DIRS), \
		if test -d $(dir) ; then \
			cd $(dir);$(MAKE) clean || exit 1;cd -; \
		else \
			exit 1; \
		fi;)

clean_all:
	@$(foreach dir,$(SUB_DIRS), \
		if test -d $(dir) ; then \
			cd $(dir);$(MAKE) clean_all || exit 1;cd -; \
		else \
			exit 1; \
		fi;)
