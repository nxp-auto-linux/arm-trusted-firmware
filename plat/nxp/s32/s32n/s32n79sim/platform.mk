#
# Copyright 2024-2025 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#
# S32N79SIM Board Specific Definitions

# To be used in targets from common makefile
DTB_FILE_NAME   ?= s32n79-simulator.dtb

include plat/nxp/s32/s32n/s32n_common.mk

# Board specific build parameters
PLAT_S32N79SIM		:=  ${PLAT_S32N}/s32n79sim

PLAT_INCLUDES += \
	-I${PLAT_S32N79SIM}/include \

# Default S32N APS topology
PLATFORM_CLUSTER_COUNT	?= 4
$(eval $(call add_define,PLATFORM_CLUSTER_COUNT))
S32N_PLAT_PRIMARY_CPU	?= 0
$(eval $(call add_define,S32N_PLAT_PRIMARY_CPU))

BL31_SOURCES += \
	${PLAT_S32N79SIM}/s32n_bl31_setup.c \
	${PLAT_S32N79SIM}/s32n_plat_funcs.c \
	${PLAT_S32N79SIM}/s32n_psci.c \
	${PLAT_S32N79SIM}/s32n_svc.c \
	${PLAT_S32N79SIM}/s32n_topology.c \
