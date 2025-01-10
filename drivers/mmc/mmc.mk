#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

MMC_SOURCES = drivers/mmc/mmc.c

MMC_RPMB_SUPPORT ?= 0

$(eval $(call assert_boolean,MMC_RPMB_SUPPORT))
$(eval $(call add_define,MMC_RPMB_SUPPORT))

ifeq (${MMC_RPMB_SUPPORT},1)
MMC_SOURCES += drivers/auth/crypto_mod.c

# Support MAC calculation
CRYPTO_SUPPORT := $(shell expr $(CRYPTO_SUPPORT) + 4 )
endif
