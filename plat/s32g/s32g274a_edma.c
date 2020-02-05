/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <lib/mmio.h>
#include <assert.h>
#include "s32g274a_edma.h"

/* Determine the optimum data transfer size based on
 * the highest common alignment between 'to', 'from' and 'length'.
 * For more information see the S32G Reference Manual, the
 * 'TCD Transfer Attributes Register' section.
 */
static int16_t max_xfer_size(uintptr_t to, uintptr_t from, size_t length,
			     uint16_t *tcd_attr)
{
	int i;
	uint64_t orr_args = (uint64_t)to | (uint64_t)from | (uint64_t)length;

	for (i = ATTR_SSIZE_MAX_OPTION; i >= 0; i--) {
		if (!(orr_args % BIT(i))) {
			*tcd_attr = ATTR_SSIZE(i) | ATTR_DSIZE(i);
			return BIT(i);
		}
	}

	return 0;
}

size_t edma_xfer_sync(uintptr_t to, uintptr_t from, size_t length,
		      uint32_t channel)
{
	uint16_t tcd_attr;
	int16_t xfer_size = max_xfer_size(to, from, length, &tcd_attr);

	assert(IS_ON_32BITS(to));
	assert(IS_ON_32BITS((uint64_t)to + length));
	assert(IS_ON_32BITS(from));
	assert(IS_ON_32BITS((uint64_t)from + length));

	mmio_write_32(EDMA0_TCDn_DADDR(channel), (uint32_t)to);
	mmio_write_32(EDMA0_TCDn_SADDR(channel), (uint32_t)from);
	mmio_write_32(EDMA0_TCDn_NBYTES_MLOFFNO(channel),
					NBYTES_MLOFFNO_NBYTES(length));

	mmio_write_16(EDMA0_TCDn_DOFF(channel), xfer_size);
	mmio_write_16(EDMA0_TCDn_SOFF(channel), xfer_size);
	mmio_write_16(EDMA0_TCDn_ATTR(channel), tcd_attr);
	mmio_write_16(EDMA0_TCDn_CITER_ELINKNO(channel),
						CITER_ELINKNO_CITER(1));
	mmio_write_16(EDMA0_TCDn_BITER_ELINKNO(channel),
						BITER_ELINKNO_BITER(1));

	mmio_write_32(EDMA0_TCDn_CSR(channel), TCD_CSR_START);

	while (true) {
		if (mmio_read_32(EDMA0_CHn_ES(channel)) & CH_ES_ERR) {
			mmio_write_32(EDMA0_CHn_ES(channel), CH_ES_ERR);
			return 0;
		}
		if (mmio_read_32(EDMA0_CHn_CSR(channel)) & CH_CSR_DONE) {
			mmio_write_32(EDMA0_CHn_CSR(channel), CH_CSR_DONE);
			return length;
		}
	}
}
