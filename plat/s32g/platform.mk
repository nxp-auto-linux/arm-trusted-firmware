#
# Copyright 2019 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

include lib/xlat_tables_v2/xlat_tables.mk

PLAT_INCLUDES		+= -Iplat/s32g/include \
			   -Iinclude/common/tbbr \
			   -Iinclude/plat/arm/common \
			   -Iinclude/plat/arm/soc/common \
			   -Iinclude/lib/psci
PLAT_BL_COMMON_SOURCES	+= plat/s32g/s32g_helpers.S \
			   plat/s32g/include/plat_macros.S \
			   plat/s32g/s32g_xrdc.c \
			   drivers/console/aarch64/console.S
PLAT_BL_COMMON_SOURCES	+= ${XLAT_TABLES_LIB_SRCS}

BL31_SOURCES		+= plat/s32g/s32g275_bl31.c \
			   plat/s32g/s32g_psci.c \
			   plat/common/plat_psci_common.c \
			   plat/common/plat_gicv3.c \
			   drivers/arm/gic/v3/gicv3_main.c \
			   drivers/arm/gic/v3/gicv3_helpers.c \
			   drivers/arm/gic/common/gic_common.c
BL31_SOURCES		+= lib/cpus/aarch64/cortex_a53.S


# Disable the PSCI platform compatibility layer
ENABLE_PLAT_COMPAT	:= 0

MULTI_CONSOLE_API	:= 1
LOAD_IMAGE_V2		:= 1
USE_COHERENT_MEM	:= 0

# Prepare the stage for BL31-only boot
PROGRAMMABLE_RESET_ADDRESS	:= 1
RESET_TO_BL31			:= 1
COLD_BOOT_SINGLE_CPU		:= 0

### Devel & Debug options ###
CFLAGS			+= -O2	# -O0 is likely to overflow the SRAM layout
# Enable dump of processor register state upon exceptions while running BL31
CRASH_REPORTING		:= 1
# As verbose as it can be
LOG_LEVEL		:= 50
# Enable simulator-specific workarounds. To be removed after hw is available.
S32G_VIRTUAL_PLATFORM	:= 1
$(eval $(call add_define_val,S32G_VIRTUAL_PLATFORM,1))
