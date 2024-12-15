
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../../../mpi_param.mk
	include $(PARAM_FILE)
endif

TARGET = isp_tool_daemon

isp_chip_name := $(shell echo $(CHIP_ARCH) | tr A-Z a-z)
ISP_BASE_PATH = $(CURDIR)/../../$(isp_chip_name)

OBJDIR = ./obj

INC_DIR += -I./
INC_DIR += -I$(MW_INC)
INC_DIR += -I$(ISP_INC)
#INC_DIR += -I../../isp-daemon/include
INC_DIR += -I$(ISP_BASE_PATH)/isp-daemon/include

CFLAGS += $(INC_DIR)

LD_OPTS += -lisp_rpc_client
LD_OPTS += -lcvi_ispd -ldl -lz
LD_OPTS += -lboost_system
LD_OPTS += -lsys
LD_OPTS += -ljson-c
LD_OPTS += -lcvi_bin
LD_OPTS += -lisp_bin
LD_OPTS += -lvi
LD_OPTS += -lvpss
LD_OPTS += -lvo
LD_OPTS += -lrgn
LD_OPTS += -lgdc
LD_OPTS += -lcvitracer
LD_OPTS += -lnanomsg
LD_OPTS += -lvenc -lvdec -lcvi_jpeg -lcvi_vcodec
LD_OPTS += -L$(MW_LIB)
LD_OPTS += -L$(MW_LIB)/3rd

ifeq ($(MULTI_PROCESS_SUPPORT), 1)
DEFS += -DRPC_MULTI_PROCESS
LIBS += -lnanomsg
endif

SRC_DIR += .

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
	@cp $(TARGET) install

$(OBJ_C): $(OBJDIR)/%.o : %.c
	@$(CC) $(CFLAGS) -o $@ -c $<

install:
	@echo "TODO@Mason: install"

uninstall:
	@echo "TODO@Mason: uninstall"

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
