#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

include ${S32CC_PLAT}/fconf/fconf.mk

ifeq (${SECBOOT_SUPPORT},1)
HSE_RMEM_SLOTS_NUM = 2
ifeq (${HSE_DDR_RMEM_ONLY},1)
HSE_RMEM_SLOTS_NUM = 1
endif
$(eval $(call add_define,HSE_RMEM_SLOTS_NUM))

PLAT_BL_COMMON_SOURCES	+= ${S32CC_PLAT}/fconf/common/s32cc_rmem_fconf_hse_getter.c
endif
