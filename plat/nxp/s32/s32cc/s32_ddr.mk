#
# Copyright 2022-2023, 2025 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

DDR_DRV = ${S32_DRIVERS}/ddr

ifneq (${CUSTOM_DDR_DRV},)
COMMON_DDR_DRV	:= ${CUSTOM_DDR_DRV}
INCLUDE_DDR_DRV := ${CUSTOM_DDR_DRV}
DDR_UTILS_FILE	:= ${CUSTOM_DDR_DRV}/ddr_utils.h
PLAT_DDR_DRV	:= ${CUSTOM_DDR_DRV}
$(eval $(call add_define,CUSTOM_DDR_DRV))
else
COMMON_DDR_DRV	= ${DDR_DRV}
PLAT_DDR_DRV	= ${DDR_DRV}/${S32_PLAT_SOC}/${PLAT_BOARD}
INCLUDE_DDR_DRV = include/${DDR_DRV}
endif

PLAT_INCLUDES += \
	-I ${INCLUDE_DDR_DRV}

DDR_DRV_SRCS += \
	${COMMON_DDR_DRV}/ddr_init.c \
	${COMMON_DDR_DRV}/ddr_utils.c \
	${COMMON_DDR_DRV}/ddr_lp.c \
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

DDR_DRV_SRCS += \
	${PLAT_DDR_DRV}/ddrc_cfg.c \
	${PLAT_DDR_DRV}/dmem_cfg.c \
	${PLAT_DDR_DRV}/dq_swap_cfg.c \
	${PLAT_DDR_DRV}/phy_cfg.c \
	${PLAT_DDR_DRV}/pie_cfg.c \
	${COMMON_DDR_DRV}/ddr_lp_csr.c \
	${COMMON_DDR_DRV}/ddrss_cfg.c \
	${COMMON_DDR_DRV}/imem_cfg.c

HSE_DDR_RMEM_ONLY = 1
endif

ifeq ($(MAKECMDGOALS), build)
ifdef DDR_FW_BIN_PATH
DDR_FW_BIN_PLAT := $(shell xxd ${DDR_FW_BIN_PATH} | sed -n 's/.*\(S32G[23]\|S32R\).*/\1/p')
ifneq (${DDR_FW_BIN_PLAT}, $(call uppercase,${S32_PLAT_SOC}))
$(error The provided firmware binary was generated for a different platform, \
not the one expected by the TF-A. Please provide the DDR firmware binary for ${S32_PLAT_SOC})
endif
else
ifeq (${CUSTOM_DDR_DRV},)
$(error DDR_FW_BIN_PATH was not provided. Please set the path to the DDR firmware binary)
endif
endif
endif

ifeq (${CUSTOM_DDR_DRV},)
$(eval $(call TOOL_ADD_IMG,DDR_FW_BIN_PATH,--ddr-fw))
endif
