opensbi: export CROSS_COMPILE=$(CONFIG_CROSS_COMPILE_SDK)
opensbi: u-boot-build
	$(call print_target)
	$(MAKE) -j${NPROC} -C ${OPENSBI_PATH} CONFIG_OPENSBI_FASTBOOT=$(CONFIG_OPENSBI_FASTBOOT) \
		PLATFORM=generic FW_PAYLOAD_PATH=${UBOOT_PATH}/${UBOOT_OUTPUT_FOLDER}/u-boot-raw.bin \
		FW_FDT_PATH=${UBOOT_PATH}/${UBOOT_OUTPUT_FOLDER}/arch/riscv/dts/${CHIP}_${BOARD}.dtb

opensbi-clean:
	$(call print_target)
	${Q}$(MAKE) -C ${OPENSBI_PATH} PLATFORM=generic distclean

FSBL_OUTPUT_PATH = ${FSBL_PATH}/build/${PROJECT_FULLNAME}
ifeq ($(call qstrip,${CONFIG_ARCH}),riscv)
fsbl-build: opensbi
endif
ifeq (${CONFIG_ENABLE_FREERTOS},y)
fsbl-build: rtos
fsbl%: export BLCP_2ND_PATH=${FREERTOS_PATH}/cvitek/install/bin/cvirtos.bin
fsbl%: export RTOS_DUMP_PRINT_ENABLE=$(CONFIG_ENABLE_RTOS_DUMP_PRINT)
fsbl%: export RTOS_DUMP_PRINT_SZ_IDX=$(CONFIG_DUMP_PRINT_SZ_IDX)
fsbl%: export RTOS_FAST_IMAGE_TYPE=${CONFIG_FAST_IMAGE_TYPE}
fsbl%: export RTOS_ENABLE_FREERTOS=${CONFIG_ENABLE_FREERTOS}
endif
fsbl%: export FSBL_SECURE_BOOT_SUPPORT=${CONFIG_FSBL_SECURE_BOOT_SUPPORT}
fsbl%: export ARCH=$(call qstrip,${CONFIG_ARCH})
fsbl%: export OD_CLK_SEL=${CONFIG_OD_CLK_SEL}
fsbl%: export VC_CLK_OVERDRIVE=${CONFIG_VC_CLK_OVERDRIVE}
fsbl%: export SUSPEND=${CONFIG_SUSPEND}
fsbl%: export TPU_PERF_MODE=$(shell if [ "${CONFIG_CHIP_cv1812cp}" = "y" ] || [ "${CONFIG_CHIP_sg2002}" = "y" ] || [ "${CONFIG_CHIP_cv1812cpa}" = "y" ]; then echo "y";else echo "n";fi)
fsbl%: export BUILD_BOOT0=${CONFIG_ENABLE_BOOT0}
fsbl%: export BUILD_FASTBOOT0=${CONFIG_ENABLE_FASTBOOT0}
fsbl%: export FSBL_FASTBOOT=${CONFIG_FSBL_FASTBOOT}
fsbl%: export STORAGE=${STORAGE_TYPE}
ifeq (${FSBL_FASTBOOT},y)
fsbl%: export LOG_LEVEL=0
else
fsbl%: export LOG_LEVEL=2
endif

ifeq (${CONFIG_ENABLE_BOOT0},y)
fsbl-build: u-boot-build memory-map
	$(call print_target)
	${Q}mkdir -p ${FSBL_PATH}/build
	${Q}ln -snrf -t ${FSBL_PATH}/build ${CVI_BOARD_MEMMAP_H_PATH}
	${Q}$(MAKE) -j${NPROC} -C ${FSBL_PATH} O=${FSBL_OUTPUT_PATH} LOG_LEVEL=${LOG_LEVEL}
	${Q}cp ${FSBL_OUTPUT_PATH}/boot0 ${OUTPUT_DIR}/
else
fsbl-build: u-boot-build memory-map
	$(call print_target)
	${Q}mkdir -p ${FSBL_PATH}/build
	${Q}ln -snrf -t ${FSBL_PATH}/build ${CVI_BOARD_MEMMAP_H_PATH}
	${Q}$(MAKE) -j${NPROC} -C ${FSBL_PATH} O=${FSBL_OUTPUT_PATH} LOG_LEVEL=${LOG_LEVEL} BLCP_2ND_PATH=${BLCP_2ND_PATH} \
		LOADER_2ND_PATH=${UBOOT_PATH}/${UBOOT_OUTPUT_FOLDER}/u-boot-raw.bin
	${Q}cp ${FSBL_OUTPUT_PATH}/fip.bin ${OUTPUT_DIR}/
ifeq (${CONFIG_UBOOT_SPL_CUSTOM},y)
	${Q}$(MAKE) -C ${FSBL_PATH} clean O=${FSBL_OUTPUT_PATH}
	${Q}$(MAKE) -j${NPROC} -C ${FSBL_PATH} O=${FSBL_OUTPUT_PATH} LOG_LEVEL=${LOG_LEVEL} BLCP_2ND_PATH=${BLCP_2ND_PATH} \
		CONFIG_SKIP_UBOOT=$(CONFIG_SKIP_UBOOT) LOADER_2ND_PATH=${UBOOT_PATH}/${UBOOT_OUTPUT_FOLDER}/spl/u-boot-spl-raw.bin
	${Q}cp ${FSBL_OUTPUT_PATH}/fip.bin ${OUTPUT_DIR}/fip_spl.bin
else
	${Q}cp ${FSBL_OUTPUT_PATH}/fip.bin ${OUTPUT_DIR}/fip_spl.bin
endif
endif

fsbl-clean: rtos-clean
	$(call print_target)
	${Q}$(MAKE) -C ${FSBL_PATH} clean O=${FSBL_OUTPUT_PATH}

u-boot-dep: fsbl-build ${OUTPUT_DIR}/elf
	$(call print_target)
ifeq ($(call qstrip,${CONFIG_ARCH}),riscv)
	${Q}cp ${OPENSBI_PATH}/build/platform/generic/firmware/fw_payload.bin ${OUTPUT_DIR}/fw_payload_uboot.bin
	${Q}cp ${OPENSBI_PATH}/build/platform/generic/firmware/fw_payload.elf ${OUTPUT_DIR}/elf/fw_payload_uboot.elf
endif

ifeq ($(call qstrip,${CONFIG_ARCH}),riscv)
u-boot-clean: opensbi-clean
endif
u-boot-clean: fsbl-clean
