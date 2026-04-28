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
#include <fconf/s32n_fconf_memory_getter.h>
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

/* MIDAC compares slot/region addresses on a 4 KB granule (bits [11:0] are
 * implicit: 0 for lower bounds, all-ones for inclusive upper bounds).
 */
#define XRDC4_MIDAC_GRANULE			0x1000U
#define XRDC4_MIDAC_GRANULE_MASK		GENMASK_64(11, 0)

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

#define XRDC4_MIDAC_MAX_DT_RANGES		16U

#define SMMS0_XRDC4_MIDAC_BASE			0x46370000U
#define SMMS1_XRDC4_MIDAC_BASE			0x5C370000U

struct xrdc_midac {
	uintptr_t base;
	const char *name;
	bool mapped;
};

enum xrdc_midac_instance {
	SMMS0_MIDAC,
	SMMS1_MIDAC,
};

static struct xrdc_midac s32n_xrdc_midac_instances[] = {
	[SMMS0_MIDAC] = { .base = SMMS0_XRDC4_MIDAC_BASE, .name = "SMMS0", .mapped = false },
	[SMMS1_MIDAC] = { .base = SMMS1_XRDC4_MIDAC_BASE, .name = "SMMS1", .mapped = false },
};

/*
 * 4 KB-aligned, inclusive [lo..hi] DT /memory range plus a 'placed' marker.
 * `placed` becomes true once any SMMS instance accepts and programs the
 * range; subsequent instances skip already-placed ranges. After both
 * instances run, any range with placed==false is a configuration error.
 */
struct xrdc_midac_dt_range {
	uint64_t lo;
	uint64_t hi;
	bool placed;
};

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
 * Walk CIDRm and collect slot indices whose OCID matches @cid. Skips
 * OCID=0 slots (PM-owned, off limits). Returns count.
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

/*
 * Read cohort region @y's [C_LOWER, C_UPPER] 40-bit bounds. Upper end's
 * low 12 bits are implicitly 0xFFF; lower start's low 12 bits are 0.
 */
static void s32n_xrdc_midac_read_region_bounds(uintptr_t base, uint32_t y,
					       uint64_t *lo, uint64_t *hi)
{
	uint32_t uh, ul, lh, ll;

	uh = mmio_read_32(base + XRDC4_MIDAC_CHRT_UPPER_H(y));
	ul = mmio_read_32(base + XRDC4_MIDAC_CHRT_UPPER_L(y));
	lh = mmio_read_32(base + XRDC4_MIDAC_CHRT_LOWER_H(y));
	ll = mmio_read_32(base + XRDC4_MIDAC_CHRT_LOWER_L(y));

	*hi = ((uint64_t)(uh & XRDC4_MIDAC_ADDRH_MASK) << 32) |
	      ((uint64_t)(ul & XRDC4_MIDAC_ADDRL_MASK)) |
	      XRDC4_MIDAC_GRANULE_MASK;

	*lo = ((uint64_t)(lh & XRDC4_MIDAC_ADDRH_MASK) << 32) |
	      ((uint64_t)(ll & XRDC4_MIDAC_ADDRL_MASK));
}

/*
 * Find a region index y such that:
 *   - cohort @cid is allowed in region y (CnEn[cid] set), AND
 *   - region y's [C_LOWER, C_UPPER] fully contains [range_lo, range_hi].
 *
 * Preference: the auto-init "primary" region (index == cid) first, then
 * other regions in ascending order. Returns >= 0 on success, -1 if none.
 */
static int s32n_xrdc_midac_find_region_for_range(uintptr_t base, uint32_t cid,
						 uint64_t range_lo,
						 uint64_t range_hi)
{
	uint32_t y;
	uint64_t lo, hi;

	/* Primary region first (== cid, the auto-init default). */
	if (cid < XRDC4_MIDAC_NUM_CHRT_REGIONS &&
	    s32n_xrdc_midac_region_allows_cohort(base, cid, cid)) {
		s32n_xrdc_midac_read_region_bounds(base, cid, &lo, &hi);
		if (lo <= range_lo && range_hi <= hi)
			return (int)cid;
	}

	/* Any other allowed region that fully contains the range. */
	for (y = 0U; y < XRDC4_MIDAC_NUM_CHRT_REGIONS; y++) {
		if (y == cid)
			continue;
		if (!s32n_xrdc_midac_region_allows_cohort(base, y, cid))
			continue;

		s32n_xrdc_midac_read_region_bounds(base, y, &lo, &hi);
		if (lo <= range_lo && range_hi <= hi)
			return (int)y;
	}

	return -1;
}

static bool is_svld_set(void *arg)
{
	struct svld_poll_ctx *ctx = arg;
	uint32_t w7 = mmio_read_32(ctx->base +
				   XRDC4_MIDAC_DACC_W(ctx->slot, 7));

	return (w7 & XRDC4_MIDAC_W7_SVLD) != 0U;
}

/*
 * Narrow slot @slot to protect [range_lo, range_hi] within cohort region @y.
 * Caller aligns range_lo DOWN and range_hi UP to 4 KB boundaries beforehand.
 *
 * Trusts the auto-init defaults on W4/W5 (DzRP/DzWP = 0xFFFFFFFF, every DID
 * in the cohort allowed). Leaves W6 untouched (no visitor, no lock).
 */
static int s32n_xrdc_midac_program_slot(uintptr_t base, uint32_t slot,
					uint32_t y, uint64_t range_lo,
					uint64_t range_hi)
{
	struct svld_poll_ctx ctx = { .base = base, .slot = slot };
	int ret;

	/* W0..W3 writes clear SVLD automatically. */
	mmio_write_32(base + XRDC4_MIDAC_DACC_W(slot, 0),
		      (uint32_t)((range_hi >> 32) & XRDC4_MIDAC_ADDRH_MASK));
	mmio_write_32(base + XRDC4_MIDAC_DACC_W(slot, 1),
		      ((uint32_t)range_hi & XRDC4_MIDAC_ADDRL_MASK) |
		      (y & XRDC4_MIDAC_W1_REGION_SEL_MASK));
	mmio_write_32(base + XRDC4_MIDAC_DACC_W(slot, 2),
		      (uint32_t)((range_lo >> 32) & XRDC4_MIDAC_ADDRH_MASK));
	mmio_write_32(base + XRDC4_MIDAC_DACC_W(slot, 3),
		      (uint32_t)range_lo & XRDC4_MIDAC_ADDRL_MASK);

	/* W4/W5/W6 intentionally unchanged — auto-init defaults OK. */

	/* Trigger revalidation and wait for SVLD=1. */
	mmio_write_32(base + XRDC4_MIDAC_DACC_W(slot, 7), XRDC4_MIDAC_W7_DONE);

	ret = wait_for_condition_timeout(is_svld_set, &ctx,
					 XRDC4_MIDAC_SVLD_TIMEOUT_US);
	if (ret) {
		ERROR("XRDC-MIDAC[0x%lx]: SVLD didn't assert (slot %u region %u [0x%lx..0x%lx])\n",
		      base, slot, y, range_lo, range_hi);
		return ret;
	}

	return 0;
}

/*
 * Collapse an owned-but-unused slot so it can't match any transaction.
 * Writing W0..W3 clears SVLD; we deliberately do NOT write DONE, so SVLD
 * stays 0 and the slot never re-enters access evaluation. OCID is
 * PM-controlled so we don't touch it.
 */
static void s32n_xrdc_midac_invalidate_slot(uintptr_t base, uint32_t slot)
{
	mmio_write_32(base + XRDC4_MIDAC_DACC_W(slot, 0), 0U);
	mmio_write_32(base + XRDC4_MIDAC_DACC_W(slot, 1), 0U);
	mmio_write_32(base + XRDC4_MIDAC_DACC_W(slot, 2), 0U);
	mmio_write_32(base + XRDC4_MIDAC_DACC_W(slot, 3), 0U);
}

static int s32n_xrdc_midac_init_instance(struct xrdc_midac *inst, uint32_t cid,
					 struct xrdc_midac_dt_range *ranges,
					 uint32_t n_ranges)
{
	uint8_t owned_slots[XRDC4_MIDAC_NUM_SLOTS];
	uint32_t n_owned, i, slot, next_slot = 0U;
	int region_y, ret;

	ret = s32n_xrdc_mmap_instance(inst->base, XRDC4_MIDAC_SIZE,
				      &inst->mapped, inst->name);
	if (ret)
		return ret;

	ret = s32n_xrdc_midac_verify_partid(inst->base);
	if (ret)
		return ret;

	n_owned = s32n_xrdc_midac_scan_owned_slots(inst->base, cid, owned_slots);

	for (i = 0U; i < n_ranges; i++) {
		if (ranges[i].placed)
			continue;

		region_y = s32n_xrdc_midac_find_region_for_range(inst->base, cid,
								 ranges[i].lo,
								 ranges[i].hi);
		if (region_y < 0)
			continue;

		if (next_slot >= n_owned) {
			ERROR("XRDC-MIDAC[%s]:\tno slots left (total: %u) for /memory ranges\n",
			      inst->name, n_owned);
			return -ENOSPC;
		}

		slot = owned_slots[next_slot++];
		VERBOSE("XRDC-MIDAC[%s]:\tslot %u -> region %u [0x%lx..0x%lx]\n",
			inst->name, slot, (uint32_t)region_y,
			ranges[i].lo, ranges[i].hi);

		ret = s32n_xrdc_midac_program_slot(inst->base, slot,
						   (uint32_t)region_y,
						   ranges[i].lo,
						   ranges[i].hi);
		if (ret)
			return ret;

		ranges[i].placed = true;
	}

	/*
	 * Collapse any owned slots we didn't need so their auto-init
	 * "wide-open" defaults don't expose memory DT never declared.
	 */
	for (i = next_slot; i < n_owned; i++) {
		s32n_xrdc_midac_invalidate_slot(inst->base, owned_slots[i]);
		VERBOSE("XRDC-MIDAC[%s]:\tslot %u invalidated (unused)\n",
			inst->name, owned_slots[i]);
	}

	INFO("XRDC-MIDAC[%s]: programmed %u/%u owned slot(s)\n",
	     inst->name, next_slot, n_owned);
	return 0;
}

int s32n_xrdc_midac_init(uint32_t cohort_id)
{
	struct xrdc_midac_dt_range ranges[XRDC4_MIDAC_MAX_DT_RANGES];
	struct dt_memory_regions mem;
	uintptr_t dt_base;
	uint32_t i, n_ranges = 0U;
	uint64_t lo, hi;
	size_t dt_size;
	int ret;

	mem.entries = FCONF_GET_PROPERTY(hw_config, dt_memory_regions, entries);
	mem.entries_n_elem = FCONF_GET_PROPERTY(hw_config, dt_memory_regions,
						entries_n_elem);

	if (mem.entries_n_elem == 0U) {
		WARN("XRDC-MIDAC: no /memory entries found in DT\n");
		return 0;
	}

	if (mem.entries_n_elem > ARRAY_SIZE(ranges)) {
		ERROR("XRDC-MIDAC: %u DT /memory entries exceed max %zu\n",
		      mem.entries_n_elem, ARRAY_SIZE(ranges));
		return -E2BIG;
	}

	/*
	 * Build a single 4 KB-aligned, inclusive-bound view of the DT ranges.
	 * Each instance will mark the entries it claims; whatever stays
	 * unclaimed after both instances run is a configuration error.
	 */
	for (i = 0U; i < mem.entries_n_elem; i++) {
		dt_base = mem.entries[i].base_addr;
		dt_size = mem.entries[i].size;

		/*
		 * Align inward to 4 KB: never protect a byte DT didn't
		 * declare. hi is the inclusive end (low 12 bits implicitly
		 * 0xFFF at the hardware level).
		 */
		lo = round_up((uint64_t)dt_base, XRDC4_MIDAC_GRANULE);
		hi = round_down((uint64_t)dt_base + dt_size, XRDC4_MIDAC_GRANULE);
		if (hi <= lo) {
			ERROR("XRDC-MIDAC: invalid DT /memory entry %u [0x%lx+0x%zx]\n",
			      i, dt_base, dt_size);
			return -EINVAL;
		}

		ranges[n_ranges].lo = lo;
		ranges[n_ranges].hi = hi - 1U;
		ranges[n_ranges].placed = false;
		n_ranges++;
	}

	for (i = 0U; i < ARRAY_SIZE(s32n_xrdc_midac_instances); i++) {
		ret = s32n_xrdc_midac_init_instance(&s32n_xrdc_midac_instances[i],
						    cohort_id, ranges, n_ranges);
		if (ret)
			return ret;
	}

	for (i = 0U; i < n_ranges; i++) {
		if (!ranges[i].placed) {
			ERROR("XRDC-MIDAC: range %u [0x%lx..0x%lx] not placed in any region\n",
			      i, ranges[i].lo, ranges[i].hi);
			return -ENOENT;
		}
	}

	return 0;
}
