/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef S32G274A_EDMA_H
#define S32G274A_EDMA_H

#define EDMA0_MP			(0x40144000)
#define EDMA0_TCD			(EDMA0_MP + 0x4000)
#define DMA_CHANNEL_1			(1)

#define EDMA0_MP_ES			(EDMA0_MP + 0x4)
#define EDMA0_CHn_CSR(ch)		(EDMA0_TCD + 0x1000 * (ch))
#define CH_CSR_DONE			BIT(30)
#define EDMA0_CHn_ES(ch)		(EDMA0_TCD + 0x1000 * (ch) + 0x04)
#define CH_ES_ERR			BIT(31)
#define EDMA0_TCDn_SADDR(ch)		(EDMA0_TCD + 0x1000 * (ch) + 0x20)
#define EDMA0_TCDn_SOFF(ch)		(EDMA0_TCD + 0x1000 * (ch) + 0x24)
#define EDMA0_TCDn_ATTR(ch)		(EDMA0_TCD + 0x1000 * (ch) + 0x26)
#define ATTR_SSIZE_SHIFT		(8)
#define ATTR_SSIZE(x)			((x) << ATTR_SSIZE_SHIFT)
#define ATTR_DSIZE(x)			(x)
#define ATTR_SSIZE_MAX_OPTION		(6)
#define EDMA0_TCDn_NBYTES_MLOFFNO(ch)	(EDMA0_TCD + 0x1000 * (ch) + 0x28)
#define NBYTES_MLOFFNO_NBYTES(x)	((x) & 0x3fffffff)
#define EDMA0_TCDn_DADDR(ch)		(EDMA0_TCD + 0x1000 * (ch) + 0x30)
#define EDMA0_TCDn_DOFF(ch)		(EDMA0_TCD + 0x1000 * (ch) + 0x34)
#define EDMA0_TCDn_CITER_ELINKNO(ch)	(EDMA0_TCD + 0x1000 * (ch) + 0x36)
#define CITER_ELINKNO_CITER(x)		((x) & 0x7fff)
#define EDMA0_TCDn_CSR(ch)		(EDMA0_TCD + 0x1000 * (ch) + 0x3c)
#define TCD_CSR_START			BIT(0)
#define EDMA0_TCDn_BITER_ELINKNO(ch)	(EDMA0_TCD + 0x1000 * (ch) + 0x3e)
#define BITER_ELINKNO_BITER(x)		((x) & 0x7fff)

#ifndef __ASSEMBLY__
size_t edma_xfer_sync(uintptr_t to, uintptr_t from, size_t length,
		      uint32_t channel);
#endif

#define IS_ON_32BITS(x)			(!((uint64_t)(x) >> 32))

#endif /* S32G274A_EDMA_H */
