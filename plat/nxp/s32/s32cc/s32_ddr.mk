#
# Copyright 2022-2023, 2025 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

DDR_DRV = ${S32_DRIVERS}/ddr

ifneq (${CUSTOM_DDR_DRV},)
COMMON_DDR_DRV	:= ${CUSTOM_DDR_DRV}
DDR_UTILS_FILE	:= ${CUSTOM_DDR_DRV}/ddr_utils.h
PLAT_DDR_DRV	:= ${CUSTOM_DDR_DRV}
$(eval $(call add_define,CUSTOM_DDR_DRV))
else
COMMON_DDR_DRV	= ${DDR_DRV}
PLAT_DDR_DRV	= ${DDR_DRV}/${S32_PLAT_SOC}/${PLAT_BOARD}
endif

DDR_DRV_SRCS += \
	${COMMON_DDR_DRV}/ddr_init.c \
	${COMMON_DDR_DRV}/ddr_utils.c \
	${COMMON_DDR_DRV}/ddr_lp.c \
	${COMMON_DDR_DRV}/ddrss_cfg.c \
	${DDR_DRV}/ddr_density.c \

# If CUSTOM_DDR_DRV is set, this target modifies the ddr_utils.h file
# to include the necessary headers and macros and remove the other ones
ifneq (${CUSTOM_DDR_DRV},)
import_ddr_drv: ${DDR_UTILS_FILE}
	@${SED} -ie "/#ifndef CONFIG_S32_GEN1/,/#endif/d" $<
	@${SED} -ie "/#include <stdbool.h>/d" $<
	@${SED} -ie "/#include <stdlib.h>/d" $<
	@${SED} -ie "/#include <ddr_plat.h>/d" $<
	@${SED} -ie "/#include <lib\/mmio.h>/d" $<
	@${SED} -ie "/#define DDR_UTILS_H_/a #include <lib\/mmio.h>" $<
	@${SED} -ie "/#define DDR_UTILS_H_/a #include <ddr_plat.h>" $<
	@${SED} -ie "s/\(#define RETENTION_ADDR\).*STNDBY_RAM_BASE/\1 BL31SSRAM_CSR_BASE/g" $<
	@${ECHO} "  EDITED  $<"
endif

ifneq (${CUSTOM_DDR_DRV},)
PLAT_INCLUDES += \
	-I ${CUSTOM_DDR_DRV}

DDR_DRV_SRCS += \
	${PLAT_DDR_DRV}/ddrc_cfg.c \
	${PLAT_DDR_DRV}/dmem_cfg.c \
	${PLAT_DDR_DRV}/dq_swap_cfg.c \
	${PLAT_DDR_DRV}/phy_cfg.c \
	${PLAT_DDR_DRV}/pie_cfg.c \
	${COMMON_DDR_DRV}/ddr_lp_csr.c \
	${COMMON_DDR_DRV}/imem_cfg.c

HSE_DDR_RMEM_ONLY = 1
endif

DDR_FW_NAME		?= ddr_fw.bin
DDR_FW_BIN		?= ${BUILD_PLAT}/${DDR_FW_NAME}

DDRTOOLPATH		?=	tools/nxp/ddrtool
DDRTOOL			?=	${DDRTOOLPATH}/ddrtool${BIN_EXT}

ifeq ($(S32GEN1_DRAM_INLINE_ECC), 1)
	ENABLE_ECC = --enable-ecc
endif

ifeq ($(S32CC_DRAM_RW_SPEED_PERF), 1)
	ENABLE_PERF = --enable-perf
endif

${DDRTOOL}:
	${Q}$(MAKE) --no-print-directory -C ${DDRTOOLPATH}

${DDR_FW_BIN}: ${DDRTOOL}
	@echo "Creating ${DDR_FW_NAME}"
	${Q}${DDRTOOL} --plat ${PLAT} ${ENABLE_ECC} ${ENABLE_PERF} --output ${DDR_FW_BIN}
	@${ECHO_BLANK_LINE}
	@echo "Built $@ successfully"
	@${ECHO_BLANK_LINE}

ifeq (${CUSTOM_DDR_DRV},)
FIP_ARGS += --ddr-fw ${DDR_FW_BIN}

fip: ${FIPTOOL} ${DDR_FW_BIN} ${FIP_BIN}
	${Q}${FIPTOOL} update ${FIP_ARGS} ${FIP_BIN}
endif

ddr_fw_clean:
	${Q}${MAKE} --no-print-directory -C ${DDRTOOLPATH} clean

clean: ddr_fw_clean
