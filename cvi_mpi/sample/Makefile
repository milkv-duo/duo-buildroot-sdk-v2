ifeq ($(CHIP_ARCH), $(filter $(CHIP_ARCH), CV181X))
	SUBDIRS := common sensor_test venc vdecvo vio sample_panel sample_ir_auto region audio ive vdec
else ifeq ($(CHIP_ARCH), $(filter $(CHIP_ARCH), CV180X))
	SUBDIRS := common sensor_test venc sample_ir_auto region audio vdec
endif

all:
	@echo "##############################"
	@echo "#                            #"
	@echo "#   Compiling 'samples'...   #"
	@echo "#                            #"
	@echo "##############################"
	@echo $(SUBDIRS) $(MAKECMDGOALS)

	@$(foreach t,$(SUBDIRS), \
		cd $(t) && $(MAKE) all || exit 1 && cd -; )

clean:
	@$(foreach t,$(SUBDIRS), \
		cd $(t) && $(MAKE) clean || exit 1 && cd -; )