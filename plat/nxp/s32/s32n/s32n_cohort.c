/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <common/debug.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>
#include <lib/xlat_tables/xlat_tables_v2.h>

#include <platform_def.h>
#include <s32_bl_common.h>
#include <s32n_cohort.h>
#include <s32n_fss_tlv.h>

#define XRDC4_CCID_ENTRY_OFFSET		(0x30)
#define CCID_ENTRY_MASK			GENMASK_32(3, 0)

static uint32_t cached_cohort_id;
static bool cohort_id_valid;

/*
 * Returns the APS peripheral base address as provided by the FSS TLV
 * area (0xFF_F00E entry). Falls back to the build-time APS_PERIPH_BASE
 * value if the TLV entry is not available.
 */
static uintptr_t s32n_get_aps_periph_base(void)
{
	const struct fss_aps_local_addr *aps_local_addr = NULL;
	int ret;

	ret = fss_tlv_get_aps_local_addr(&aps_local_addr);
	if ((ret != 0) || (aps_local_addr == NULL) ||
	    (aps_local_addr->aps_count == 0U) ||
	    (aps_local_addr->ranges[0].base == 0U)) {
		WARN("Could not retrieve APS peripheral base from FSS TLV (ret=%d), using default 0x%lx\n",
		     ret, (uintptr_t)APS_PERIPH_BASE);
		return (uintptr_t)APS_PERIPH_BASE;
	}

	return (uintptr_t)aps_local_addr->ranges[0].base;
}

int s32n_get_cohort_id(uint32_t *cohort_id)
{
	uintptr_t ccid_periph_addr;
	int ret = 0;

	if (cohort_id == NULL) {
		return -EINVAL;
	}

	if (cohort_id_valid) {
		goto out;
	}

	ccid_periph_addr = s32n_get_aps_periph_base() + XRDC4_CCID_LOCAL_ADDR;

	ret = mmap_add_dynamic_region(ccid_periph_addr, ccid_periph_addr,
				      MMU_ROUND_UP_TO_PAGE(XRDC4_CCID_CHI_SIZE),
				      MT_DEVICE | MT_RO | MT_SECURE);
	if (ret != 0) {
		ERROR("XRDC4_CCID_CHI mapping: error %d\n", ret);
		goto out;
	}

	cached_cohort_id = mmio_read_32(ccid_periph_addr + XRDC4_CCID_ENTRY_OFFSET) &
			   CCID_ENTRY_MASK;
	cohort_id_valid = true;

out:
	if (ret == 0) {
		*cohort_id = cached_cohort_id;
	}

	return ret;
}
