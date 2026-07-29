#
# Copyright 2019-2022, 2025-2026 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

S32_PLAT_SOC := s32g2

S32_VR5510 := 1

ERRATA_S32_050481	:= 1
ERRATA_S32_050543     := 1

include plat/nxp/s32/s32cc/s32g/s32g_common.mk

PLAT_SOC_PATH	:= ${S32_SOC_FAMILY}/${S32_PLAT_SOC}

PLAT_INCLUDES		+= -I${PLAT_SOC_PATH}/include \

PLAT_BL_COMMON_SOURCES	+= ${S32_DRIVERS}/clk/s32g274a_clk.c \
			   ${S32_DRIVERS}/clk/s32g2g3_common_clk.c \
			   ${S32CC_PLAT}/s32cc_mc_me.c \
			   ${S32CC_PLAT}/s32cc_mc_rgm.c \
			   ${S32CC_PLAT}/s32cc_sramc.c \
			   lib/cpus/aarch64/cortex_a53.S \

