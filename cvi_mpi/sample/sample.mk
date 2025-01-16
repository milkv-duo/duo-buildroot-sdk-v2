################# select sensor type for your sample ###############################
ifeq ($(BUILD_PATH),)
ifeq ("$(CHIP_ARCH)", "CV181X")
CONFIG_SENSOR_GCORE_GC2093 ?= y
CONFIG_SENSOR_GCORE_GC2053 ?= y
CONFIG_SENSOR_GCORE_GC2053_1L ?= y
CONFIG_SENSOR_GCORE_GC4653 ?= y
else ifeq ("$(CHIP_ARCH)", "CV180X")
CONFIG_SENSOR_SMS_SC2336 ?= y
CONFIG_SENSOR_GCORE_GC4653 ?= y
else
$(error "unknown soc type.")
endif
endif

SNS_LIB = -lsns_full

COMMON_DIR := $(CURDIR)/../common
PANEL_DIR := $(CURDIR)/../../component/panel/$(shell echo $(CHIP_ARCH) | tr A-Z a-z)

ifeq ($(BUILD_PATH),)
ifeq ("$(CHIP_ARCH)", "CV181X")
CFLAGS += -DCONFIG_SENSOR_GCORE_GC2093=$(CONFIG_SENSOR_GCORE_GC2093)
CFLAGS += -DCONFIG_SENSOR_GCORE_GC2053=$(CONFIG_SENSOR_GCORE_GC2053)
CFLAGS += -DCONFIG_SENSOR_GCORE_GC2053_1L=$(CONFIG_SENSOR_GCORE_GC2053_1L)
CFLAGS += -DCONFIG_SENSOR_GCORE_GC4653=$(CONFIG_SENSOR_GCORE_GC4653)
else ifeq ("$(CHIP_ARCH)", "CV180X")
CFLAGS += -DCONFIG_SENSOR_SMS_SC2336=$(CONFIG_SENSOR_SMS_SC2336)
CFLAGS += -DCONFIG_SENSOR_GCORE_GC4653=$(CONFIG_SENSOR_GCORE_GC4653)
else
$(error "unknown soc type.")
endif
endif

ifeq ($(DEBUG), 1)
CFLAGS += -g -O0
endif

ifeq ($(SAMPLE_STATIC), 1)
ELFFLAGS += -static
endif

#########################################################################
COMM_SRC := $(wildcard $(COMMON_DIR)/*.c)
COMM_OBJ := $(COMM_SRC:%.c=%.o)
COMM_INC := $(COMMON_DIR)
COMM_DEPS = $(COMM_SRC:.c=.d)
PANEL_INC := $(PANEL_DIR)
