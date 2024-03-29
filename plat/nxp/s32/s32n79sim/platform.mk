#
# Copyright 2024 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

# SoC-specific build parameters
PLAT_COMMON_PATH	:=	plat/nxp/common
PLAT_SOC_PATH		:=	plat/nxp/s32
PLAT_DRIVERS_PATH	:=	drivers/nxp
PLAT_S32N79SIM		:=  ${PLAT_SOC_PATH}/s32n79sim

PLAT_INCLUDES += \
	-Iinclude/plat/common \
	-I${PLAT_S32N79SIM}/include \

S32N_SIM := 1
$(eval $(call add_define,S32N_SIM))

# Default S32N APS topology
PLATFORM_CLUSTER_COUNT	?= 1
$(eval $(call add_define,PLATFORM_CLUSTER_COUNT))
S32N_PLAT_PRIMARY_CPU	?= 0
$(eval $(call add_define,S32N_PLAT_PRIMARY_CPU))

### Devel & Debug options ###
ifeq (${DEBUG},1)
	CFLAGS			+= -O0
else
	CFLAGS			+= -Os
endif
