/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <s32_bl2_common.h>
#include <lib/mmio.h>
#include <common/debug.h>

#define AARCH64_UNCOND_BRANCH_MASK		(0x7c000000u)
#define AARCH64_UNCOND_BRANCH_OP		(BIT(26) | BIT(28))
#define AARCH64_UNCOND_BRANCH_OP_BOOT0_HOOK	(BIT(27) | BIT(28) | BIT(30))
#define BL33_DTB_MAGIC				(0xedfe0dd0u)

static bool is_branch_op(uint32_t op)
{
	unsigned int op_mask = op & AARCH64_UNCOND_BRANCH_MASK;

	return op_mask == AARCH64_UNCOND_BRANCH_OP ||
		op_mask == AARCH64_UNCOND_BRANCH_OP_BOOT0_HOOK;
}

int bl2_copy_bl31_dtb(void)
{
	uint32_t magic;
	int ret;

	magic = mmio_read_32(BL33_ENTRYPOINT);
	if (!is_branch_op(magic)) {
		WARN(
		    "Instruction at BL33_ENTRYPOINT (0x%x) is 0x%x, which is not a B or BL!\n",
		    BL33_ENTRYPOINT, magic);
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

