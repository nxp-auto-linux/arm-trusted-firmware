#
# Copyright 2024-2025 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

include plat/nxp/s32/s32cc/s32_ddr.mk
include plat/nxp/s32/s32cc/s32r/s32r_common.mk

ERRATA_S32_050481      := 1
ERRATA_S32_050543      := 1

PLAT_SOC_PATH	:= ${S32_SOC_FAMILY}/s32r45

PLAT_INCLUDES	+= -I${PLAT_SOC_PATH}/include \

BL2_SOURCES     +=  \
		    ${PLAT_SOC_PATH}/s32r45_bl2_el3_plat_arch_setup.c \
		    ${S32CC_PLAT}/s32cc_lowlevel_bl2.S \

PLAT_BL_COMMON_SOURCES	+= ${S32_DRIVERS}/clk/s32r45_clk.c \
