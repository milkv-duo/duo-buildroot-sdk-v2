SHELL = /bin/bash
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../../mpi_param.mk
	include $(PARAM_FILE)
endif

SDIR = $(CURDIR)
SRCS = $(wildcard $(SDIR)/*.c)
INCS = -I$(KERNEL_INC) -I$(MW_INC) -I$(ISP_INC) -I../../common -I$(SENSOR_LIST_INC)
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)

EXTRA_CFLAGS = $(INCS)
LIBS = -lsys
EXTRA_LDFLAGS = $(LIBS) -lm -latomic -lpthread

TARGET = lt9611

.PHONY : clean all
all: $(TARGET)

$(SDIR)/%.o: $(SDIR)/%.c
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<
	@echo [$(notdir $(CC))] $(notdir $@)

$(TARGET): $(OBJS)
	@$(CXX) -o $@ $(OBJS) $(ELFFLAGS) $(EXTRA_LDFLAGS) -static
	@echo -e $(BLUE)[LINK]$(END)[$(notdir $(CC))] $(notdir $@)

clean:
	@rm -f $(OBJS) $(DEPS) $(TARGET)

-include $(DEPS)
