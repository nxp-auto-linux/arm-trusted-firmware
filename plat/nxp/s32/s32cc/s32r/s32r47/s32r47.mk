#
# Copyright 2024-2025 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

include plat/nxp/s32/s32cc/s32r/s32r_common.mk

PLAT_SOC_PATH	:= ${S32_SOC_FAMILY}/s32r47

PLAT_INCLUDES	+= -I${PLAT_SOC_PATH}/include \

BL2_SOURCES     +=  \
		    ${PLAT_SOC_PATH}/s32r47_bl2_el3_plat_arch_setup.c

S32R47_W_S32R45 ?= 0
S32R47_W_S32G2 ?= 0

S32_LINFLEX_MODULE := 0

ifeq ($(S32R47_W_S32R45),1)
	PLAT_BL_COMMON_SOURCES	+= ${S32_DRIVERS}/clk/s32r45_clk.c \

else ifeq ($(S32R47_W_S32G2),1)
	PLAT_BL_COMMON_SOURCES 	+= ${S32_DRIVERS}/clk/s32g_clk.c \
				   ${S32_DRIVERS}/clk/s32g274a_clk.c\

else
	PLAT_BL_COMMON_SOURCES	+= ${S32_DRIVERS}/clk/s32r47_clk.c \

endif
