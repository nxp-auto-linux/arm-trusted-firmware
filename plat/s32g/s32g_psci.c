/*
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <string.h>
#include <assert.h>
#include <common/debug.h>	/* printing macros such as INFO() */
#include "platform_def.h"

IMPORT_SYM(unsigned long, __BL31_START__, bl31_start);
IMPORT_SYM(unsigned long, __BL31_END__, bl31_end);

/*
 * Copy the PSCI callbacks (in fact, the entire bl31 binary) to the protected
 * DRAM area only accessible to privileged contexts.
 * This must be called before XRDC is set up, since after that it becomes
 * read-only (even to privileged code) until the next reboot.
 */
void s32g_psci_move_to_pram(void)
{
	INFO("Copying self (0x%lx .. 0x%lx) to DRAM (0x%lx)...",
	     bl31_start, bl31_end, (unsigned long)S32G_PMEM_START);
	/* FIXME this may be too time-consuming; we should do it via DMA and/or
	 * copy only the relevant section(s) of the entire blob.
	 */
	_Static_assert(S32G_PMEM_END >= S32G_PMEM_START,
		       "S32G: PMEM_END < PMEM_START");
	/* TODO Ideally, this would have been a static assert, also; however,
	 * __BL31_START__ and __BL31_END__ are linker symbols, so static assert
	 * isn't possible.
	 */
	assert(bl31_end - bl31_start <= S32G_PMEM_END - S32G_PMEM_START);
	memcpy((unsigned char *)S32G_PMEM_START,
	       (unsigned char *)bl31_start,
	       bl31_end - bl31_start);
	puts(" Done.");
}
