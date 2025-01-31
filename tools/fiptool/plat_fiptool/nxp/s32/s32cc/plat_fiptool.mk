#
# Copyright 2025 NXP. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

# Name of the platform defined source file name,
# which contains platform defined UUID entries populated
# in the plat_def_toc_entries[].

PLAT_DEF_UUID_CONFIG_FILE_NAME	:= plat_def_uuid_config

PLAT_DEF_UUID_OID_CONFIG_PATH := ../../plat/nxp/s32/s32cc

INCLUDE_PATHS		+= -I${PLAT_DEF_UUID_OID_CONFIG_PATH}/include -I./

PLAT_DEF_UUID		:= yes

ifeq (${PLAT_DEF_UUID},yes)
HOSTCCFLAGS += -DPLAT_DEF_FIP_UUID

PLAT_OBJECTS += plat_fiptool/nxp/s32/s32cc/plat_def_uuid_config.o

endif

OBJECTS += ${PLAT_OBJECTS}
