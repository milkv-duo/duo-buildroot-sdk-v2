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

	@for t in $(SUBDIRS); do $(MAKE) -C $$t all || exit 1; done

clean:
	@for t in $(SUBDIRS); do $(MAKE) -C $$t clean || exit 1; done