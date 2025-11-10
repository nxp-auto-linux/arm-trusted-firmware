#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

ifneq (${CUSTOM_DDR_DRV},)
$(eval $(call add_define, CUSTOM_DDR_DRV))
endif
