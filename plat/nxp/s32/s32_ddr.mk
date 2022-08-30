#
# Copyright 2022 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

DDR_DRV = ${S32_DRIVERS}/ddr

COMMON_DDR_DRV	= ${DDR_DRV}
PLAT_DDR_DRV	= ${DDR_DRV}/${S32_PLAT_SOC}

DDR_DRV_SRCS += \
	${PLAT_DDR_DRV}/ddrc_cfg.c \
	${PLAT_DDR_DRV}/dmem_cfg.c \
	${PLAT_DDR_DRV}/dq_swap_cfg.c \
	${PLAT_DDR_DRV}/phy_cfg.c \
	${PLAT_DDR_DRV}/pie_cfg.c \
	${COMMON_DDR_DRV}/ddr_init.c \
	${COMMON_DDR_DRV}/ddr_utils.c \
	${COMMON_DDR_DRV}/ddr_lp.c \
	${COMMON_DDR_DRV}/ddr_lp_csr.c \
	${COMMON_DDR_DRV}/ddrss_cfg.c \
	${COMMON_DDR_DRV}/imem_cfg.c \
	${DDR_DRV}/ddr_density.c \
