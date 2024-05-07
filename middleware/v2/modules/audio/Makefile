.PHONY: clean all
.SILENT: clean all

include audio.mk

all:
	cp prebuilt/$(SDK_VER)/fdk_aac/lib* $(MW_LIB)
	cp prebuilt/$(SDK_VER)/tinyalsa_lib/lib* $(MW_LIB)
	cp $(shell echo $(CHIP_ARCH) | tr A-Z a-z)/$(SDK_VER)/lib* $(MW_LIB)

clean:
	#rm $(MW_LIB)/libaac*
	#rm $(MW_LIB)/libcvi_audio*
	#rm $(MW_LIB)/libcvi_RES1*
	#rm $(MW_LIB)/libcvi_ssp*
	#rm $(MW_LIB)/libcvi_VoiceEngine*
	#rm $(MW_LIB)/libcvi_vqe*
	#rm $(MW_LIB)/libdnvqe*
	#rm $(MW_LIB)/libtinyalsa*
