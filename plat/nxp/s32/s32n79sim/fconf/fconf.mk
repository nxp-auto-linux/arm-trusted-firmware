#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

PLAT_INCLUDES 	+= \
		-I${PLAT_S32N79SIM}/include/fconf/

PLAT_BL_COMMON_SOURCES	+= ${PLAT_S32N79SIM}/fconf/s32n_fconf_scmi_getter.c
