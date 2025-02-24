#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

PLAT_INCLUDES 	+= -I${S32CC_PLAT}/include/
BL2_SOURCES 	+=	common/fdt_wrappers.c

ifeq (${SECBOOT_SUPPORT},1)
PLAT_BL_COMMON_SOURCES	+= ${S32CC_PLAT}/fconf/s32cc_fconf_hse_getter.c
BL2_SOURCES += ${S32CC_PLAT}/fconf/s32cc_fconf_hse_getter.c
endif
