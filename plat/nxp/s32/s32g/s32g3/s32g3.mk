#
# Copyright 2021-2022 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

S32_PLAT_SOC := s32g3

include plat/nxp/s32/s32g/s32g_common.mk

PLAT_SOC_PATH	:= ${S32_SOC_FAMILY}/${S32_PLAT_SOC}

PLAT_INCLUDES		+= -I${PLAT_SOC_PATH}/include \

PLAT_BL_COMMON_SOURCES	+= ${PLAT_SOC_PATH}/s32g3_mc_me.c \
			   ${PLAT_SOC_PATH}/s32g3_mc_rgm.c \
			   ${PLAT_SOC_PATH}/s32g3_sramc.c \
			   ${PLAT_SOC_PATH}/s32g3_vr5510.c \
			   ${S32_DRIVERS}/clk/s32g3_clk.c \
			   lib/cpus/aarch64/cortex_a53.S \
