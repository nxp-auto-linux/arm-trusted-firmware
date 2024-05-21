// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2024 NXP
 */

#include <common/debug.h>
#include <lib/mmio.h>
#include <s32cc_bl_common.h>
#include <drivers/nxp/s32/rtc/s32g/s32g_rtc.h>
#include <lib/xlat_tables/xlat_tables_v2.h>

/* RTC definitions space */
#define S32G_RTC_BASE		(0x40060000ul)
#define S32G_RTC_SIZE		(0x18)
#define RTC_RTCC_OFFSET		0x4
#define RTC_RTCS_OFFSET		0x8
#define RTC_RTCS_RTCF		BIT(29)
#define RTC_APIVAL_OFFSET	0x10
#define RTC_RTCVAL_OFFSET	0x14

static int s32g_rtc_init(void)
{
	size_t reg_size = round_up(S32G_RTC_SIZE, PAGE_SIZE);
	static bool mmap_added;

	return s32_mmap_dynamic_region(S32G_RTC_BASE, reg_size, MT_DEVICE | MT_RW, &mmap_added);
}

int s32g_reset_rtc(void)
{
	const uint32_t rtc = S32G_RTC_BASE;
	int ret;

	ret = s32g_rtc_init();
	if (ret)
		return ret;

	mmio_write_32(rtc + RTC_APIVAL_OFFSET, 0x0);
	mmio_write_32(rtc + RTC_RTCVAL_OFFSET, 0x0);

	mmio_write_32(rtc + RTC_RTCC_OFFSET, 0x0);

	mmio_setbits_32(rtc + RTC_RTCS_OFFSET, 0x0);

	return 0;
}
