obj-y += cvsnfc_common.o
cvsvfc-y += cvsnfc.o cvsnfc_spi_ids.o
ifdef CONFIG_SPL_BUILD
cvsvfc-$(CONFIG_NAND_FLASH_CVSNFC_SPL) += cvsnfc_spl.o
endif
obj-$(CONFIG_NAND_FLASH_CVSNFC)	+= cvsvfc.o
