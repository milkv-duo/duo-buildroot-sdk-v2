ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../../../mpi_param.mk
	include $(PARAM_FILE)
endif

TARGET = raw_dump_test

OBJDIR = ./obj

INC_DIR += -I$(MW_INC) -I$(ISP_INC) -I$(KERNEL_INC)
INC_DIR += -I$(MW_INC)/../modules/sys/include
INC_DIR += -I../raw_dump/src

CFLAGS += $(INC_DIR)

LD_OPTS += -lsample -lsns_full -lz -ldl -lm -lraw_dump
LD_OPTS += -lcvitracer -lpthread -lini
#LD_OPTS += -ldl -lm -lraw_dump
LD_OPTS += -lvi -lvpss -lvo -lrgn -lgdc -lvenc -lvdec -lcvi_jpeg -lcvi_vcodec $(ISP_LIB) -lsys $(SNS_LIB)
LD_OPTS += -L$(MW_LIB) -L$(MW_3RD_LIB) $(CVI_TARGET_PACKAGES_LIBDIR)
ifeq ($(MULTI_PROCESS_SUPPORT), 1)
DEFS += -DRPC_MULTI_PROCESS
LD_OPTS += -lnanomsg
endif

SRC_DIR += .
#SRC_DIR += ./src

VPATH = $(SRC_DIR)

SRC_C = $(shell find $(SRC_DIR) -maxdepth 1 -name '*.c')

OBJ_C_0 = $(notdir $(patsubst %.c, %.o, $(SRC_C)))

OBJ_C    = $(foreach file, $(OBJ_C_0), $(OBJDIR)/$(file))

DEPS_C   = $(OBJ_C:%.o=%.d)

.PHONY: all setup target install uninstall clean

all: setup target

setup:
	@mkdir -p $(OBJDIR)

target: setup $(OBJ_C)
	@$(CC) $(OBJ_C) $(LD_OPTS) -o $(TARGET)
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(TARGET)

$(OBJ_C): $(OBJDIR)/%.o : %.c
	@$(CC) $(CFLAGS) -o $@ -c $<
	@echo [$(notdir $(CC))] $(notdir $@)

install:
	@echo "install"

uninstall:
	@echo "uninstall"

clean:
	@rm -rf $(OBJDIR)
	@rm -rf $(TARGET)

$(DEPS_C): $(OBJDIR)/%.d : %.c
	@mkdir -p $(OBJDIR); \
	set -e; rm -f $@; \
	$(CC) -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,$(OBJDIR)\/\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

-include $(DEPS_C)
