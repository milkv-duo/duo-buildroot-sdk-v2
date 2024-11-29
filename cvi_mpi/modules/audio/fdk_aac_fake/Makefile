.PHONY: CREATE_DIR

PARAM_FILE:=../../../mpi_param.mk
include $(PARAM_FILE)
#
AUDIO_PARAM_FILE :=../audio.mk
include $(AUDIO_PARAM_FILE)
#
AUD_INC :=../../include
#
SDIR	:= $(CURDIR)/src
AACENC_SRCS := $(SDIR)/aacenc_interface_empty.cpp
AACDEC_SRCS := $(SDIR)/aacdec_interface_empty.cpp
AACCOMM_SRCS := $(SDIR)/aac_comm_empty.cpp
AACSBRENC_SRCS := $(SDIR)/aac_sbrenc_empty.cpp
AACSBRDEC_SRCS := $(SDIR)/aac_sbrdec_empty.cpp
#
ODIR    := $(CURDIR)/obj
AACENC_OBJS		:= $(AACENC_SRCS:$(SDIR)/%.cpp=$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).o)
AACDEC_OBJS		:= $(AACDEC_SRCS:$(SDIR)/%.cpp=$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).o)
AACCOMM_OBJS	:= $(AACCOMM_SRCS:$(SDIR)/%.cpp=$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).o)
AACSBRENC_OBJS	:= $(AACSBRENC_SRCS:$(SDIR)/%.cpp=$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).o)
AACSBRDEC_OBJS	:= $(AACSBRDEC_SRCS:$(SDIR)/%.cpp=$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).o)
OBJS	:= $(AACENC_OBJS) $(AACDEC_OBJS) $(AACCOMM_OBJS) $(AACSBRENC_OBJS) $(AACSBRDEC_OBJS)
DEPS	:= $(OBJS:%.c=%.d)
#
AACENC_TARGET_A = $(MW_LIB)/libaacenc2.a
AACENC_TARGET_SO = $(MW_LIB)/libaacenc2.so
#
AACDEC_TARGET_A = $(MW_LIB)/libaacdec2.a
AACDEC_TARGET_SO = $(MW_LIB)/libaacdec2.so
#
AACCOMM_TARGET_A = $(MW_LIB)/libaaccomm2.a
AACCOMM_TARGET_SO = $(MW_LIB)/libaaccomm2.so
#
AACSBRENC_TARGET_A = $(MW_LIB)/libaacsbrenc2.a
AACSBRENC_TARGET_SO = $(MW_LIB)/libaacsbrenc2.so
#
AACSBRDEC_TARGET_A = $(MW_LIB)/libaacsbrdec2.a
AACSBRDEC_TARGET_SO = $(MW_LIB)/libaacsbrdec2.so
#
AAC_LDLIBS =
AAC_LDLIBS += -lpthread
AAC_LDLIBS += -lm -ldl
AAC_LDLIBS += -lstdc++
AAC_LDLIBS += -laacdec2
AAC_LDLIBS += -laacenc2

AAC_INCLUDES = -I$(MW_INC)
AAC_INCLUDES += -I$(CURDIR)
DEFINES += -D__FILENAME__=\"$(notdir $<)\"
#DEFINES += -DSUPPORT_EXTERNAL_AAC

AAC_CFLAGS += -I . -fPIC -Wno-unused-result  -Wno-format -Wl,--fatal-warning $(AAC_INCLUDES) $(DEFINES)
#AAC test sample code ----------------------------end

ARFLAGS = rcs
LDFLAGS = -shared -fPIC -o
CFLAGS = $(OPT_LEVEL) -fPIC -c -Os
CFLAGS += -D__FILENAME__=\"$(notdir $<)\"
CFLAGS += -Wall

AAC_TEST_LIB = $(AAC_LDLIBS)
AAC_TEST_LIB += -lcvi_audio -lcvi_vqe -lcvi_VoiceEngine -lcvi_RES
USE_TINYALSA = yes
ifeq ($(USE_ALSA), yes)
AAC_TEST_LIB += -lasound
endif
ifeq ($(USE_TINYALSA), yes)
AAC_TEST_LIB += -ltinyalsa
endif
AAC_TEST_LIB +=  -lcvi_AEC -laec
#
EXTRA_CFLAGS = -I$(MW_INC) -I./include
.PHONY : all clean clean_all

BUILDLIST := $(AACENC_TARGET_A) $(AACENC_TARGET_SO)
BUILDLIST += $(AACDEC_TARGET_A) $(AACDEC_TARGET_SO)
BUILDLIST += $(AACCOMM_TARGET_A) $(AACCOMM_TARGET_SO)
BUILDLIST += $(AACSBRENC_TARGET_A) $(AACSBRENC_TARGET_SO)
BUILDLIST += $(AACSBRDEC_TARGET_A) $(AACSBRDEC_TARGET_SO)

all:$(BUILDLIST)

$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).o : $(SDIR)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -c $<  -o $@
	@echo [$(notdir $(CC))] $(notdir $@)

$(ODIR)/%.$(TARGET_MACHINE).$(CHIP_ARCH_L).o : $(SDIR)/%.cpp
	@mkdir -p $(dir $@)
	@$(CC) $(DEPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -c $<  -o $@
	@echo [$(notdir $(CC))] $(notdir $@)

$(AACENC_TARGET_A) : $(AACENC_OBJS)
	@$(AR) $(ARFLAGS) $(AACENC_TARGET_A) $(AACENC_OBJS)
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $@)
$(AACENC_TARGET_SO) : $(AACENC_OBJS)
	@$(CC) $(LDFLAGS)  $(AACENC_TARGET_SO) $(AACENC_OBJS)
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $@)
#
$(AACDEC_TARGET_A) : $(AACDEC_OBJS)
	@$(AR) $(ARFLAGS) $(AACDEC_TARGET_A) $(AACDEC_OBJS)
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $@)
$(AACDEC_TARGET_SO) : $(AACDEC_OBJS)
	@$(CC) $(LDFLAGS)  $(AACDEC_TARGET_SO) $(AACDEC_OBJS)
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $@)
#
$(AACCOMM_TARGET_A) : $(AACCOMM_OBJS)
	@$(AR) $(ARFLAGS) $(AACCOMM_TARGET_A) $(AACCOMM_OBJS)
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $@)
$(AACCOMM_TARGET_SO) : $(AACCOMM_OBJS)
	@$(CC) $(LDFLAGS)  $(AACCOMM_TARGET_SO) $(AACCOMM_OBJS)
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $@)
#
$(AACSBRENC_TARGET_A) : $(AACSBRENC_OBJS)
	@$(AR) $(ARFLAGS) $(AACSBRENC_TARGET_A) $(AACSBRENC_OBJS)
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $@)
$(AACSBRENC_TARGET_SO) : $(AACSBRENC_OBJS)
	@$(CC) $(LDFLAGS)  $(AACSBRENC_TARGET_SO) $(AACSBRENC_OBJS)
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $@)
#
$(AACSBRDEC_TARGET_A) : $(AACSBRDEC_OBJS)
	@$(AR) $(ARFLAGS) $(AACSBRDEC_TARGET_A) $(AACSBRDEC_OBJS)
	@echo -e $(YELLOW)[LINK]$(END)[$(notdir $(AR))] $(notdir $@)
$(AACSBRDEC_TARGET_SO) : $(AACSBRDEC_OBJS)
	@$(CC) $(LDFLAGS)  $(AACSBRDEC_TARGET_SO) $(AACSBRDEC_OBJS)
	@echo -e $(GREEN)[LINK]$(END)[$(notdir $(LD))] $(notdir $@)
#
$(AAC_TEST):
	$(CC) -o $@ $(AAC_LDFLAGS) $(AAC_TEST_SRC) $(AAC_TEST_LIB) $(AAC_CFLAGS)
##
clean:
	@rm -f $(AACENC_TARGET_A) $(AACENC_TARGET_SO) \
		$(AACDEC_TARGET_A) $(AACDEC_TARGET_SO) \
		$(AACCOMM_TARGET_A) $(AACCOMM_TARGET_SO) \
		$(AACSBRENC_TARGET_A) $(AACSBRENC_TARGET_SO) \
		$(AACSBRDEC_TARGET_A) $(AACSBRDEC_TARGET_SO)

clean_all: clean
	if [ -d "$(SDIR)" ]; then \
		echo "rm -rf $(ODIR)"; \
		rm -rf $(ODIR); \
	fi
