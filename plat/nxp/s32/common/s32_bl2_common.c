/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <inttypes.h>
#include <s32_bl2_common.h>
#include <lib/mmio.h>
#include <common/debug.h>

#define AARCH64_UNCOND_BRANCH_MASK		(0x7c000000u)
#define AARCH64_ADRP_MASK			(0x9f000000u)
#define AARCH64_UNCOND_BRANCH_OP		(BIT(26) | BIT(28))
#define AARCH64_OP_BOOT0_HOOK			(BIT(28) | BIT(31))

static bool is_branch_op(uint32_t op)
{
	if ((op & AARCH64_UNCOND_BRANCH_MASK) == AARCH64_UNCOND_BRANCH_OP)
		return true;

	if ((op & AARCH64_ADRP_MASK) == AARCH64_OP_BOOT0_HOOK)
		return true;

	return false;
}

int bl2_copy_bl31_dtb(void)
{
	uint32_t magic;
	int ret;

	magic = mmio_read_32(BL33_ENTRYPOINT);
	if (!is_branch_op(magic)) {
		WARN(
		    "Instruction at BL33_ENTRYPOINT (0x%" PRIxPTR
		    ") is 0x%x, which is not the expected one!\n",
		    (uintptr_t)BL33_ENTRYPOINT, magic);
	}

	if (get_bl2_dtb_size() > BL33_MAX_DTB_SIZE) {
		ERROR("The DTB exceeds max BL31 DTB size: 0x%x\n",
		      BL33_MAX_DTB_SIZE);
		return -EIO;
	}

	memcpy((void *)BL33_DTB, (void *)get_bl2_dtb_base(),
	       get_bl2_dtb_size());

	ret = apply_bl2_fixups((void *)BL33_DTB);
	if (ret)
		return ret;

	return 0;
}

