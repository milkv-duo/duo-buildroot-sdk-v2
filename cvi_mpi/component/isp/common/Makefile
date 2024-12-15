SHELL = /bin/bash
#
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../../mpi_param.mk
	include $(PARAM_FILE)
endif
#
SDIR = $(CURDIR)
SRCS = $(wildcard $(SDIR)/*.c)
INCS = -I$(MW_INC) -I$(ISP_INC) -I$(KERNEL_INC) -I.
OBJS = $(patsubst %.c, %.o, $(SRCS))
#
EXTRA_CFLAGS += $(INCS)
#
ifeq ($(DEBUG), 1)
	EXTRA_CFLAGS += -g -O0
endif

# add sensor -D definition
ifneq ($(BUILD_PATH),)
include $(BUILD_PATH)/.config
endif
# include Kbuild
EXTRA_CFLAGS += $(KBUILD_DEFINES)

.PHONY : clean all
all : $(OBJS)

$(SDIR)/%.o: $(SDIR)/%.c
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -c $< -o $@
	@echo [$(notdir $(CC))] $(notdir $@)

clean:
	@rm -f $(OBJS) *.d *.a
