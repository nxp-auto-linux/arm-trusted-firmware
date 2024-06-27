#
# Copyright 2021-2022, 2024 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

S32_PLAT_SOC := s32g3

S32_VR5510 := 1

ifneq "$(origin ERRATA_ERR052269)" "command line"
	ifeq ($(S32CC_USE_SCP),1)
		ERRATA_ERR052269 := 0
	else
		ERRATA_ERR052269 := 1
	endif
endif

include plat/nxp/s32/s32cc/s32g/s32g_common.mk

PLAT_SOC_PATH	:= ${S32_SOC_FAMILY}/${S32_PLAT_SOC}

PLAT_INCLUDES		+= -I${PLAT_SOC_PATH}/include \

PLAT_BL_COMMON_SOURCES	+= ${PLAT_SOC_PATH}/s32g3_mc_me.c \
			   ${PLAT_SOC_PATH}/s32g3_mc_rgm.c \
			   ${PLAT_SOC_PATH}/s32g3_sramc.c \
			   ${PLAT_SOC_PATH}/s32g3_vr5510.c \
			   ${S32_DRIVERS}/clk/s32g3_clk.c \
			   lib/cpus/aarch64/cortex_a53.S \

ifeq ($(ERRATA_ERR052269),1)
PLAT_BL_COMMON_SOURCES	+= ${PLAT_SOC_PATH}/s32g3_flexnoc.c
endif
