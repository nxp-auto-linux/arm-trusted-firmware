#
# Copyright 2024 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

include drivers/arm/gic/v3/gicv3.mk
include lib/xlat_tables_v2/xlat_tables.mk

# SoC-specific build parameters
PLAT_COMMON_PATH	:=	plat/nxp/common
PLAT_SOC_PATH		:=	plat/nxp/s32
PLAT_DRIVERS_PATH	:=	drivers/nxp
PLAT_S32N79SIM		:=  ${PLAT_SOC_PATH}/s32n79sim

PLAT_INCLUDES += \
	-Iinclude/common \
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
CTX_INCLUDE_AARCH32_REGS := 0

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

# U-Boot: CONFIG_SYS_DATA_BASE
BL33_IMG_BASE ?= 0xFF700000
$(eval $(call add_define,BL33_BASE))
# U-Boot: CONFIG_SYS_TEXT_BASE
BL33_ENTRYPOINT ?= 0xFF7A0000
$(eval $(call add_define,BL33_ENTRYPOINT))

# Location of FIP image in memory
S32N_FIP_BASE ?= 0x80000000
$(eval $(call add_define,S32N_FIP_BASE))
S32N_FIP_SIZE ?= 0x100000
$(eval $(call add_define,S32N_FIP_SIZE))

# Choose which PL011 module to use
S32N_PL011_MODULE ?= 0
$(eval $(call add_define,S32N_PL011_MODULE))

# Enable dump of processor register state upon exceptions while running BL31
CRASH_REPORTING		:= 1
# As verbose as it can be
LOG_LEVEL		?= 50

PLAT_BL_COMMON_SOURCES += \
	drivers/arm/pl011/aarch64/pl011_console.S	\
	lib/cpus/aarch64/cortex_a78_ae.S \
	${PLAT_S32N79SIM}/plat_helpers.S \

BL2_SOURCES += \
	common/desc_image_load.c \
	drivers/io/io_fip.c \
	drivers/io/io_memmap.c \
	drivers/io/io_storage.c \
	lib/cpus/aarch64/cortex_a78_ae.S \
	${PLAT_S32N79SIM}/s32n_bl2_el3_setup.c \
	${PLAT_S32N79SIM}/s32n_bl2_mem_params_desc.c \
	${PLAT_S32N79SIM}/s32n_io_storage.c \

BL31_SOURCES += \
	${GICV3_SOURCES} \
	plat/common/plat_gicv3.c \
	plat/common/plat_psci_common.c \
	${PLAT_S32N79SIM}/s32n_bl31_setup.c \
	${PLAT_S32N79SIM}/s32n_psci.c \
	${PLAT_S32N79SIM}/s32n_topology.c \
	${XLAT_TABLES_LIB_SRCS} \

### Devel & Debug options ###
ifeq (${DEBUG},1)
	CFLAGS			+= -O0
else
	CFLAGS			+= -Os
endif

all: fip

