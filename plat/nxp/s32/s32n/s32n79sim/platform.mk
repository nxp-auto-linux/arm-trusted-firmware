#
# Copyright 2024-2025 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#
# S32N79SIM Board Specific Definitions

# To be used in targets from common makefile
DTB_FILE_NAME   ?= s32n79-simulator.dtb

include plat/nxp/s32/s32n/s32n79.mk

# Board specific build parameters
PLAT_S32N79SIM		:=  ${PLAT_S32N}/s32n79sim

PLAT_INCLUDES += \
	-I${PLAT_S32N79SIM}/include \
