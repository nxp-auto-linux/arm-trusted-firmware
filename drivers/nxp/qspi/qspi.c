/*
 * Copyright 2021, 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>

#include <common/debug.h>
#include <lib/mmio.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <drivers/nxp/qspi/qspi.h>

#define QUADSPI_MCR			0x00000000u
#define QUADSPI_MCR_SWRSTSD_MASK	BIT(0)
#define QUADSPI_MCR_SWRSTHD_MASK	BIT(1)
#define QUADSPI_MCR_MDIS_MASK		BIT(14)

static void invalidate_qspi_ahb(uintptr_t s32_qspi_base)
{
	uint32_t reset_mask = QUADSPI_MCR_SWRSTHD_MASK | QUADSPI_MCR_SWRSTSD_MASK;

	mmio_clrbits_32(s32_qspi_base + QUADSPI_MCR, QUADSPI_MCR_MDIS_MASK);
	mmio_setbits_32(s32_qspi_base + QUADSPI_MCR, reset_mask);
	mmio_setbits_32(s32_qspi_base + QUADSPI_MCR, QUADSPI_MCR_MDIS_MASK);
	mmio_clrbits_32(s32_qspi_base + QUADSPI_MCR, reset_mask);
	mmio_clrbits_32(s32_qspi_base + QUADSPI_MCR, QUADSPI_MCR_MDIS_MASK);
}

void s32_qspi_io_setup(uintptr_t nxp_qspi_flash_addr,
		  size_t nxp_qspi_flash_size, uintptr_t s32_qspi_base)
{
	int result;

	invalidate_qspi_ahb(s32_qspi_base);

	if (nxp_qspi_flash_size > SIZE_MAX - PAGE_SIZE)
		panic();

	result = mmap_add_dynamic_region(nxp_qspi_flash_addr, nxp_qspi_flash_addr,
			round_up(nxp_qspi_flash_size, PAGE_SIZE), MT_RO | MT_MEMORY | MT_SECURE);
	if (result < 0) {
		ERROR("QSPI mapping: error %d\n", result);
		panic();
	}
}

int qspi_io_setup(uintptr_t nxp_qspi_flash_addr,
		  size_t nxp_qspi_flash_size,
		  uintptr_t fip_offset)
{
	uint32_t qspi_mcr_val = qspi_in32(CHS_QSPI_MCR);

	/* Enable and change endianness of QSPI IP */
	qspi_out32(CHS_QSPI_MCR, (qspi_mcr_val | CHS_QSPI_64LE));

	/* Adding QSPI Memory Map in XLAT Table */
	mmap_add_region(nxp_qspi_flash_addr, nxp_qspi_flash_addr,
			nxp_qspi_flash_size, MT_MEMORY | MT_RW);

	return 0;
}
