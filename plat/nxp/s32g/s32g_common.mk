#
# Copyright 2020-2021 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

include make_helpers/build_macros.mk

# Tools
HEXDUMP ?= xxd
SED ?= sed

S32G_DRAM_INLINE_ECC	?= 1
$(eval $(call add_define_val,S32G_DRAM_INLINE_ECC,$(S32G_DRAM_INLINE_ECC)))

DDR_DRV = drivers/nxp/s32g/ddr

DDR_DRV_SRCS = \
	${DDR_DRV}/ddr_init.c \
	${DDR_DRV}/ddr_utils_mmio.c \
	${DDR_DRV}/ddrc_cfg.c \
	${DDR_DRV}/ddrss.c \
	${DDR_DRV}/ddrss_cfg.c \
	${DDR_DRV}/dmem_cfg.c \
	${DDR_DRV}/dq_swap_cfg.c \
	${DDR_DRV}/imem_cfg.c \
	${DDR_DRV}/phy_cfg.c \
	${DDR_DRV}/pie_cfg.c \
	${DDR_DRV}/rev2/ddrc_cfg_rev2.c \
	${DDR_DRV}/rev2/dmem_cfg_rev2.c \
	${DDR_DRV}/rev2/phy_cfg_rev2.c \
	${DDR_DRV}/rev2/pie_cfg_rev2.c \

