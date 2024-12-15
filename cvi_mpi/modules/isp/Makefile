.PHONY: clean all clean_all
.SILENT: clean all

include isp.mk

SUB_DIRS := $(isp_chip_dir)

all:
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

clean_all: clean
	@$(foreach dir,$(SUB_DIRS), \
		if test -d $(dir) ; then \
			cd $(dir);$(MAKE) clean_all || exit 1;cd -; \
		else \
			exit 1; \
		fi;)
