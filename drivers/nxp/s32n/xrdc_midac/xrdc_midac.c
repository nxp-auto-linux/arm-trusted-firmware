/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <stdbool.h>

#include <common/debug.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>

#include <drivers/nxp/s32n/s32n_xrdc_common.h>
#include <drivers/nxp/s32n/s32n_xrdc_midac.h>
#include <drivers/nxp/s32n/s32n_xrdc_midac_data.h>
#include <s32n_bl_common.h>

/*
 * Index-variable naming convention (mirrors the XRDC4_MIDAC chapter in the
 * S32N79 Reference Manual, such that register names line up with the code):
 *
 *   x - Slot number                (0 .. NUM_SLOTS-1)
 *   y - Cohort region number       (0 .. NUM_CHRT_REGIONS-1)
 *   z - Domain number (DID)        (0 .. 31 on this instance)
 *   n - Cohort number (CID)        (0 ..  7 on this instance)
 *   m - CIDRm register index       (0 .. NUM_SLOTS/4 - 1)
 *   k - Lane within one CIDRm      (0 .. 3);  slot = 4*m + k
 */

#define XRDC4_MIDAC_SIZE			0x1000U

#define XRDC4_MIDAC_PARTID			0x000U
/* XRDC4_MIDAC instances - ID Register Magic Value */
#define XRDC4_MIDAC_PARTID_VAL			0x31DAC000U

/* CHRT_Ry_*: four 32-bit words per cohort region, stride 0x10. */
#define XRDC4_MIDAC_CHRT_UPPER_H(y)		(0x020U + ((y) * 0x10U))
#define XRDC4_MIDAC_CHRT_UPPER_L(y)		(0x024U + ((y) * 0x10U))
#define XRDC4_MIDAC_CHRT_LOWER_H(y)		(0x028U + ((y) * 0x10U))
#define XRDC4_MIDAC_CHRT_LOWER_L(y)		(0x02CU + ((y) * 0x10U))

#define XRDC4_MIDAC_CNEN_MASK			GENMASK_32(7, 0)
#define XRDC4_MIDAC_ADDRH_MASK			GENMASK_32(7, 0)
#define XRDC4_MIDAC_ADDRL_MASK			GENMASK_32(31, 12)

/* CIDRm: four 4-bit OCID fields per register. */
#define XRDC4_MIDAC_CIDRm(m)			(0x420U + ((m) * 0x4U))
#define XRDC4_MIDAC_OCID_MASK			GENMASK_32(3, 0)

/* DACC_CR_Sx_Ww: eight 32-bit words per slot, stride 0x20. */
#define XRDC4_MIDAC_DACC_W(x, w)		(0x480U + ((x) * 0x20U) + ((w) * 0x4U))

#define XRDC4_MIDAC_W1_REGION_SEL_MASK		GENMASK_32(4, 0)
#define XRDC4_MIDAC_W7_DONE			BIT_32(0)
#define XRDC4_MIDAC_W7_SVLD			BIT_32(1)

#define XRDC4_MIDAC_NUM_SLOTS			64U
#define XRDC4_MIDAC_NUM_CHRT_REGIONS		32U
#define XRDC4_MIDAC_NUM_CIDRm			((XRDC4_MIDAC_NUM_SLOTS) / 4U)
#define XRDC4_MIDAC_NUM_COHORTS			8U

/*
 * The partitioning manager's CID is 0. OCID=0 on a slot means "owned by PM"
 * and must not be touched by a cohort manager.
 */
#define XRDC4_MIDAC_PM_CID			0U

#define XRDC4_MIDAC_SVLD_TIMEOUT_US		10000U /* 10 ms */

struct svld_poll_ctx {
	uintptr_t base;
	uint32_t slot;
};

static int s32n_xrdc_midac_verify_partid(uintptr_t base)
{
	uint32_t partid = mmio_read_32(base + XRDC4_MIDAC_PARTID);

	if (partid != XRDC4_MIDAC_PARTID_VAL) {
		ERROR("XRDC-MIDAC[0x%lx]: bad PARTID 0x%x (expected 0x%x)\n",
		      base, partid, XRDC4_MIDAC_PARTID_VAL);
		return -ENXIO;
	}

	return 0;
}

/*
 * Iterate through all slots by walking every CIDRm register and the four
 * OCID lanes packed into each one, collecting the slots whose Owning
 * Cohort ID (OCID) matches @cid into @out_slots. Slots owned by the
 * Partitioning Manager (OCID == 0) are skipped. Returns the number of
 * slots collected.
 *
 * The outer loop variable m indexes the CIDRm registers; the inner
 * loop variable k indexes the four OCID lanes packed into one CIDRm.
 * The slot number is reconstructed as 4*m + k.
 */
static uint32_t s32n_xrdc_midac_scan_owned_slots(uintptr_t base, uint32_t cid,
						 uint8_t out_slots[XRDC4_MIDAC_NUM_SLOTS])
{
	uint32_t m, k, cidr, ocid, slot, count = 0U;

	for (m = 0U; m < XRDC4_MIDAC_NUM_CIDRm; m++) {
		cidr = mmio_read_32(base + XRDC4_MIDAC_CIDRm(m));

		for (k = 0U; k < 4U; k++) {
			ocid = (cidr >> (k * 8U)) & XRDC4_MIDAC_OCID_MASK;
			slot = m * 4U + k;

			if (ocid == XRDC4_MIDAC_PM_CID)
				continue;
			if (ocid != cid)
				continue;

			out_slots[count++] = (uint8_t)slot;
		}
	}

	return count;
}

/*
 * Return true if cohort @cid is permitted in cohort region @y, i.e.
 * bit @cid of the region's CnEn bitmap is set in CHRT_Ry_UPPER_ADDRL.
 */
static bool s32n_xrdc_midac_region_allows_cohort(uintptr_t base, uint32_t y,
						 uint32_t cid)
{
	uint32_t upper_l;

	if (cid >= XRDC4_MIDAC_NUM_COHORTS)
		return false;

	upper_l = mmio_read_32(base + XRDC4_MIDAC_CHRT_UPPER_L(y));
	return ((upper_l & XRDC4_MIDAC_CNEN_MASK) & BIT_32(cid)) != 0U;
}

static bool is_svld_set(void *arg)
{
	struct svld_poll_ctx *ctx = arg;
	uint32_t w7 = mmio_read_32(ctx->base +
				   XRDC4_MIDAC_DACC_W(ctx->slot, 7));

	return (w7 & XRDC4_MIDAC_W7_SVLD) != 0U;
}

/*
 * Point slot @slot at cohort region @y with the region's own bounds. The slot's
 * W0..W3 mirror CHRT_Ry_{UPPER_H,UPPER_L,LOWER_H,LOWER_L}, so each word is
 * copied across with only its address field kept.
 *
 * Trusts the auto-init defaults on W4/W5 (DzRP/DzWP = 0xFFFFFFFF, every DID
 * in the cohort allowed). Leaves W6 untouched (no visitor, no lock).
 */
static int s32n_xrdc_midac_copy_region_to_slot(uintptr_t base, uint32_t slot,
					       uint32_t y)
{
	struct svld_poll_ctx ctx = { .base = base, .slot = slot };
	int ret;

	/* W0..W3 writes clear SVLD automatically. */
	mmio_write_32(base + XRDC4_MIDAC_DACC_W(slot, 0),
		      mmio_read_32(base + XRDC4_MIDAC_CHRT_UPPER_H(y)) &
		      XRDC4_MIDAC_ADDRH_MASK);
	mmio_write_32(base + XRDC4_MIDAC_DACC_W(slot, 1),
		      (mmio_read_32(base + XRDC4_MIDAC_CHRT_UPPER_L(y)) &
		       XRDC4_MIDAC_ADDRL_MASK) |
		      (y & XRDC4_MIDAC_W1_REGION_SEL_MASK));
	mmio_write_32(base + XRDC4_MIDAC_DACC_W(slot, 2),
		      mmio_read_32(base + XRDC4_MIDAC_CHRT_LOWER_H(y)) &
		      XRDC4_MIDAC_ADDRH_MASK);
	mmio_write_32(base + XRDC4_MIDAC_DACC_W(slot, 3),
		      mmio_read_32(base + XRDC4_MIDAC_CHRT_LOWER_L(y)) &
		      XRDC4_MIDAC_ADDRL_MASK);

	/* W4/W5/W6 intentionally unchanged — auto-init defaults OK. */

	/* Trigger revalidation and wait for SVLD=1. */
	mmio_write_32(base + XRDC4_MIDAC_DACC_W(slot, 7), XRDC4_MIDAC_W7_DONE);

	ret = wait_for_condition_timeout(is_svld_set, &ctx,
					 XRDC4_MIDAC_SVLD_TIMEOUT_US);
	if (ret) {
		ERROR("XRDC-MIDAC[0x%lx]: SVLD didn't assert (slot %u region %u)\n",
		      base, slot, y);
		return ret;
	}

	return 0;
}

/*
 * Give the cohort every region on @inst that PM granted it, one owned slot per
 * region. Writing the OCID auto-initializes every owned slot to the cohort
 * region whose index is the OCID (CHRT_REGION_SEL = OCID), so keep one slot
 * there and repoint the rest at the other regions. The bounds are copied from
 * the region registers, which already hold the addresses in the SMMS-side
 * domain whatever the DDR interleaving mode.
 */
static int s32n_xrdc_midac_init_instance(struct xrdc_midac *inst, uint32_t cid)
{
	uint8_t owned_slots[XRDC4_MIDAC_NUM_SLOTS];
	uint32_t n_owned, next_slot, n_regions = 0U, y;
	bool auto_init_ok;
	int ret;

	ret = s32n_xrdc_mmap_instance(inst->base, XRDC4_MIDAC_SIZE,
				      &inst->mapped, inst->name);
	if (ret)
		return ret;

	ret = s32n_xrdc_midac_verify_partid(inst->base);
	if (ret)
		return ret;

	n_owned = s32n_xrdc_midac_scan_owned_slots(inst->base, cid, owned_slots);
	if (n_owned == 0U) {
		WARN("XRDC-MIDAC[%s]: PM granted cohort %u no slots\n",
		     inst->name, cid);
		return 0;
	}

	/* Region number cid is where auto-init pointed the owned slots. */
	auto_init_ok = s32n_xrdc_midac_region_allows_cohort(inst->base, cid, cid);
	next_slot = auto_init_ok ? 1U : 0U;

	for (y = 0U; y < XRDC4_MIDAC_NUM_CHRT_REGIONS; y++) {
		if (!s32n_xrdc_midac_region_allows_cohort(inst->base, y, cid))
			continue;

		n_regions++;

		if (auto_init_ok && y == cid) {
			INFO("XRDC-MIDAC[%s]: slot %u region %u (PM auto-init)\n",
			     inst->name, owned_slots[0], y);
			continue;
		}

		if (next_slot >= n_owned) {
			ERROR("XRDC-MIDAC[%s]: no slot left for region %u (%u owned)\n",
			      inst->name, y, n_owned);
			return -ENOSPC;
		}

		ret = s32n_xrdc_midac_copy_region_to_slot(inst->base,
							  owned_slots[next_slot],
							  y);
		if (ret)
			return ret;

		INFO("XRDC-MIDAC[%s]: slot %u region %u\n",
		     inst->name, owned_slots[next_slot], y);
		next_slot++;
	}

	if (n_regions == 0U)
		WARN("XRDC-MIDAC[%s]: cohort %u owns %u slot(s) but no region permits it\n",
		     inst->name, cid, n_owned);

	return 0;
}

int s32n_xrdc_midac_init(uint32_t cohort_id)
{
	uint32_t i;
	int ret;

	for (i = 0U; i < s32n_xrdc_midac_n_instances; i++) {
		ret = s32n_xrdc_midac_init_instance(&s32n_xrdc_midac_instances[i],
						    cohort_id);
		if (ret)
			return ret;
	}

	return 0;
}
