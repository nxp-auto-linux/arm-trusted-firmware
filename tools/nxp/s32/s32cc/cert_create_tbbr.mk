#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

HAS_DDR_FW := 1
$(eval $(call add_define,HAS_DDR_FW))

include ../nxp/s32/cert_create_tbbr.mk
