#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

ifeq ($(ERRATA_S32_050543),1)
DDR_ERRATUM_SOURCES	+= ${COMMON_DDR_DRV}/ddr_utils.c \
					${S32_DRIVERS}/s32cc/ddr/ddr_erratum/ddr_poll.c \
					${S32_DRIVERS}/s32cc/ddr/ddr_erratum/derating_erratum.c \
					bl32/tsp/tsp_interrupt.c \
					bl32/tsp/tsp_timer.c \

endif