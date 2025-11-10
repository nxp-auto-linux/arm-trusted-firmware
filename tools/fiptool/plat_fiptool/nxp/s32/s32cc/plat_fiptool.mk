#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

$(eval $(call add_define, HAS_DDR_FW))

ifneq (${CUSTOM_DDR_DRV},)
$(eval $(call add_define, CUSTOM_DDR_DRV))
endif
