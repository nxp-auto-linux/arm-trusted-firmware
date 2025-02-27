#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

PLAT_INCLUDES 	+= \
		-I${PLAT_S32N79SIM}/include/fconf/

PLAT_BL_COMMON_SOURCES	+= common/fdt_wrappers.c \
		${PLAT_S32N79SIM}/fconf/s32n_fconf_scmi_getter.c \

BL2_SOURCES += \
		${PLAT_S32N79SIM}/fconf/s32n_fconf_ufs_getter.c

BL31_SOURCES += ${PLAT_S32N79SIM}/fconf/s32n_fconf_gic_getter.c
