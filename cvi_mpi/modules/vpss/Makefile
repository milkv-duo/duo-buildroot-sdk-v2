SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
     PARAM_FILE:=../../Makefile.param
     include $(PARAM_FILE)
endif

SDIR = $(PWD)/src
SRCS = $(wildcard $(SDIR)/*.c)
ifeq ($(CHIP_ARCH), $(filter $(CHIP_ARCH), CV180X CV181X MARS PHOBOS))
	SRCS := $(filter-out $(SDIR)/cvi_gdc.c, $(SRCS))
	SRCS := $(filter-out $(SDIR)/gdc_mesh.c, $(SRCS))
endif

ifeq ($(CHIP_ARCH), $(filter $(CHIP_ARCH), CV180X PHOBOS))
	SRCS := $(filter-out $(SDIR)/cvi_vo.c, $(SRCS))
endif

INCS = -I$(MW_INC) -I$(ISP_INC) -I$(SYS_INC) -I$(BIN_INC) -I./include -I../gdc/include -I$(KERNEL_INC)

OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)
TARGET_A = $(MW_LIB)/libvpss.a
TARGET_SO = $(MW_LIB)/libvpss.so

EXTRA_CFLAGS = $(INCS)


.PHONY : clean all
all : $(TARGET_A) $(TARGET_SO)

$(SDIR)/%.o: $(SDIR)/%.c
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -c $< -o $@
	@echo [$(notdir $(CC))] $(notdir $@)

$(TARGET_A): $(OBJS)
	@$(AR) $(ARFLAGS) $(TARGET_A) $(OBJS)
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $(TARGET_A))

$(TARGET_SO): $(OBJS)
	@$(LD) $(LDFLAGS) -o $(TARGET_SO) --start-group $(OBJS) --end-group
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $(TARGET_SO))

clean:
	@rm -f $(OBJS) $(DEPS) $(TARGET_A) $(TARGET_SO)

-include $(DEPS)
