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

RESET_TO_BL2 := 1
COLD_BOOT_SINGLE_CPU := 0

PROGRAMMABLE_RESET_ADDRESS := 1

# Mandatory for Cortex-A78AE
HW_ASSISTED_COHERENCY := 1
USE_COHERENT_MEM := 0

# Default S32N APS topology
PLATFORM_CLUSTER_COUNT	?= 1
$(eval $(call add_define,PLATFORM_CLUSTER_COUNT))
S32N_PLAT_PRIMARY_CPU	?= 0
$(eval $(call add_define,S32N_PLAT_PRIMARY_CPU))

# DDR definitions
DDR_START ?= 0x80000000
$(eval $(call add_define,DDR_START))
DDR_END ?= 0xFFFFFFFF
$(eval $(call add_define,DDR_END))
# Total memory reserved for the BL stages
PLAT_BL_MEM_SIZE ?= 0x100000
$(eval $(call add_define,PLAT_BL_MEM_SIZE))

PLAT_BL_COMMON_SOURCES += \
	${PLAT_S32N79SIM}/plat_helpers.S \

BL2_SOURCES += \
	common/desc_image_load.c \
	drivers/io/io_storage.c \
	lib/cpus/aarch64/cortex_a78_ae.S \
	${PLAT_S32N79SIM}/s32n_bl2_el3_setup.c \
	${PLAT_S32N79SIM}/s32n_bl2_mem_params_desc.c \
	${PLAT_S32N79SIM}/s32n_io_storage.c \

### Devel & Debug options ###
ifeq (${DEBUG},1)
	CFLAGS			+= -O0
else
	CFLAGS			+= -Os
endif
