/*
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Extended Resource Domain Controller configuration. The XRDC is configured
 * early in the boot process to allow the TF-A to install the PSCI handlers
 * in a secure DDR area.
 */
#include <lib/mmio.h>
#include <common/debug.h>
#include "platform_def.h"

/* Master Domains - application core, safety core, jtag, GIC, GMAC, eDMA ... */
#define MDAC_CA53_CCI_0		0	/* Cortex-A53 CCI0 Master ID */
#define MDAC_CA53_CCI_1		1	/* Cortex-A53 CCI1 Master ID */
#define MDAC_JTAG_DEBUG_MID	21	/* JTAG Debugger Master ID */

/* 3-bit Domain IDs to use with MDA[DIDS]=b'00 (which is the reset value) */
#define DOMAIN_DBG	1u	/* Real-Time core and also the JTAG debugger */
#define DOMAIN_CA53	2u	/* Application cores from either cluster */

/*
 * XRDC Hardware Registers
 */

/* CR: global valid */
#define XRDC_CR_GVLD_SHIFT	0
#define XRDC_CR_GVLD_MASK	0x00000001
/* CR: 1-bit lock */
#define XRDC_CR_LK1_SHIFT	30
#define XRDC_CR_LK1_MASK	0x40000000
/* HWCFG0: NMSTR */
#define XRDC_HWCFG0_NMSTR_SHIFT	8
#define XRDC_HWCFG0_NMSTR_MASK	0x0000ff00
/* HWCFG0: NPAC */
#define XRDC_HWCFG0_NPAC_SHIFT	24
#define XRDC_HWCFG0_NPAC_MASK	0x0f000000
/* MDACFG: NMDAR */
#define XRDC_MDACFG_NMDAR_SHIFT	0
#define XRDC_MDACFG_NMDAR_MASK	0x0F
/* MDA: valid */
#define XRDC_MDA_VLD_SHIFT	31
#define XRDC_MDA_VLD_MASK	0x80000000
/* MDA: 1-bit lock*/
#define XRDC_MDA_LK1_SHIFT	30
#define XRDC_MDA_LK1_MASK	0x40000000
/* MDA: did */
#define XRDC_MDA_DID_SHIFT	0
#define XRDC_MDA_DID_MASK	0x00000007
/* PDAC_W1: valid */
#define XRDC_PDAC_W1_VLD_SHIFT	31
#define XRDC_PDAC_W1_VLD_MASK	0x80000000
/* PDAC_W1: lock */
#define XRDC_PDAC_W1_LK2_SHIFT	29
#define XRDC_PDAC_W1_LK2_MASK	0x60000000
/* MRGD_W3: valid */
#define XRDC_MRGD_W3_VLD_SHIFT	31
#define XRDC_MRGD_W3_VLD_MASK	0x80000000
/* MRGD_W3: lock */
#define XRDC_MRGD_W3_LK2_SHIFT	29
#define XRDC_MRGD_W3_LK2_MASK	0x60000000

/* MRC descriptor instances for various blocks; count descriptors with stride 16
 * even though some regions have fewer than 16 of descriptors in use.
 */
#define MRC_DDR0	0		/* XRDC_MRC0 range  */
#define MRC_DDR1	(16 * 1)	/* XRDC_MRC1 range  */
#define MRC_FLEXNOC	(16 * 12)	/* XRDC_MRC12 range */
/* set domain access policy */
#define XRDC_ACP(did, acp)	((acp) << 3 * (did))
/* grant RW access to all domains */
#define XRDC_ACP_RW_ALL		0x00FFFFFF


/* Avoid sub-word-size accesses to registers, either accidentally or because of
 * the compiler
 */
#define XRDC_SET_32(s, m, val) \
	mmio_write_32((uintptr_t)(&((s)->m)), (val))
#define XRDC_GET_32(s, m) \
	mmio_read_32((uintptr_t)&((s)->m))
#define XRDC_GET_8(s, m) \
	mmio_read_8((uintptr_t)&((s)->m))

/**
 * struct xrdc_derr_reg - XRDC Domain Error Register
 * @Wn: Wordn
 */
struct xrdc_derr_reg {
	const uint32_t W0, W1, W2;
	uint32_t W3;
} __attribute__((aligned(4)));

/**
 * struct xrdc_pdac_reg - XRDC Peripheral Domain Access Control Register
 * @Wn: Wordn
 */
struct xrdc_pdac_reg {
	uint32_t W0, W1;
} __attribute__((aligned(4)));

/**
 * struct xrdc_mrgd_reg - XRDC Memory Region Descriptor Register
 * @Wn: Wordn
 */
struct xrdc_mrgd_reg {
	uint32_t W0, W1, W2, W3;
	/* This is technically not part of the MRGD, but it opportunistically
	 * helps simplify the xrdc_regs struct.
	 */
	uint8_t reserved[16];
} __attribute__((aligned(4)));

/**
 * enum xrdc_dacp - XRDC Domain Access Control Policy
 *
 * @ACP_NONE:    Access denied to domain
 * @ACP_SRO:     Secure read-only access
 * @ACP_SRW:     Secure read-write access
 * @ACP_RO:      Non-secure read-only access
 * @ACP_RW:      Non-secure read-write access
 */
enum xrdc_dacp {
	ACP_NONE = 0,
	ACP_SRO = 1,
	ACP_SRW = 3,
	ACP_RO = 5,
	ACP_RW = 7,
};

/**
 * struct xrdc_regs - XRDC Peripheral Register Structure
 *
 * Registers can be accessed via 8-, 16- or 32-bit reads and 32-bit writes.
 */
static volatile struct xrdc_regs {
	uint32_t CR;			/* offset 0x000 */
	uint8_t reserved0[236];
	/*
	 * HWCFG
	 */
	uint32_t HWCFG0;		/* offset 0x0F0 */
	const uint32_t HWCFG1;		/* offset 0x0F4 */
	const uint32_t HWCFG2;		/* offset 0x0F8 */
	uint8_t reserved1[4];
	/*
	 * MDACFG
	 */
	uint8_t MDACFG[22];		/* offset 0x100 */
	uint8_t reserved2[42];
	/*
	 * MRCFG
	 */
	const uint8_t MRCFG[6];		/* offset 0x140 */
	uint8_t reserved3[186];
	/*
	 * DERRLOC
	 */
	const uint32_t DERRLOC[8];	/* offset 0x200 */
	uint8_t reserved4[480];
	/*
	 * DERR
	 */
	struct xrdc_derr_reg DERR[20];	/* offset 0x400 */
	uint8_t reserved5[448];
	/*
	 * PID - for the moment, opaque bits
	 */
	uint32_t PID0;			/* offset 0x700 */
	uint32_t PID1;			/* offset 0x704 */
	uint8_t reserved6[24];
	uint32_t PID8;			/* offset 0x720 */
	uint32_t PID9;			/* offset 0x724 */
	uint32_t PID10;			/* offset 0x728 */
	uint8_t reserved7[20];
	uint32_t PID16;			/* offset 0x740 */
	uint32_t PID17;			/* offset 0x744 */
	uint32_t PID18;			/* offset 0x748 */
	uint8_t reserved8[180];
	/*
	 * MDA
	 */
	uint32_t MDA0[8];		/* offset 0x800 */
	uint32_t MDA1[8];		/* offset 0x820 */
	uint32_t MDA2[1];		/* offset 0x840 */
	uint8_t reserved9[28];
	uint32_t MDA3[1];		/* offset 0x860 */
	uint8_t reserved10[28];
	uint32_t MDA4[1];		/* offset 0x880 */
	uint8_t reserved11[28];
	uint32_t MDA5[1];		/* offset 0x8A0 */
	uint8_t reserved12[28];
	uint32_t MDA6[1];		/* offset 0x8C0 */
	uint8_t reserved13[28];
	uint32_t MDA7[1];		/* offset 0x8E0 */
	uint8_t reserved14[28];
	uint32_t MDA8[8];		/* offset 0x900 */
	uint32_t MDA9[8];		/* offset 0x920 */
	uint32_t MDA10[8];		/* offset 0x940 */
	uint32_t MDA11[1];		/* offset 0x960 */
	uint8_t reserved15[28];
	uint32_t MDA12[1];		/* offset 0x980 */
	uint8_t reserved16[28];
	uint32_t MDA13[1];		/* offset 0x9A0 */
	uint8_t reserved17[28];
	uint32_t MDA14[1];		/* offset 0x9C0 */
	uint8_t reserved18[28];
	uint32_t MDA15[1];		/* offset 0x9E0 */
	uint8_t reserved19[28];
	uint32_t MDA16[8];		/* offset 0xA00 */
	uint32_t MDA17[8];		/* offset 0xA20 */
	uint32_t MDA18[8];		/* offset 0xA40 */
	uint32_t MDA19[1];		/* offset 0xA60 */
	uint8_t reserved20[28];
	uint32_t MDA20[1];		/* offset 0xA80 */
	uint8_t reserved21[28];
	uint32_t MDA21[1];		/* offset 0xAA0 */
	uint8_t reserved22[1372];
	/*
	 * PDAC
	 */
	struct xrdc_pdac_reg PDAC0_31[32];	/* offset 0x1000 */
	uint8_t reserved23[960];
	struct xrdc_pdac_reg PDAC128_158[31];	/* offset 0x1400 */
	uint8_t reserved24[776];
	struct xrdc_pdac_reg PDAC256_284[29];	/* offset 0x1800 */
	uint8_t reserved25[792];
	struct xrdc_pdac_reg PDAC384_408[25];	/* offset 0x1C00 */
	uint8_t reserved26[824];
	/*
	 * MRGD
	 */
	struct xrdc_mrgd_reg MRGD0_187[188];	/* offset 0x2000 */
	uint8_t reserved27[144];
	struct xrdc_mrgd_reg MRGD192_195[4];	/* offset 0x3800 */
	uint8_t reserved28[384];
	struct xrdc_mrgd_reg MRGD208_223[16];	/* offset 0x3A00 */
} __attribute__((packed, aligned(4))) *s32g_xrdc;

/**
 * Validate MRGD index and get MRGD struct
 */
static volatile struct xrdc_mrgd_reg *
xrdc_get_mrgd_by_idx(volatile struct xrdc_regs *xrdc, uint32_t idx)
{
	if (!xrdc)
		return NULL;
	if (idx <= 187)
		return &xrdc->MRGD0_187[idx];
	if (idx < 192)
		return NULL;
	if (idx <= 195)
		return &xrdc->MRGD192_195[idx - 192];
	if (idx < 208)
		return NULL;
	if (idx <= 223)
		return &xrdc->MRGD208_223[idx - 208];
	return NULL;
}

/**
 * Validate MDA index and get MDA struct
 *
 * @domain: MDAC submodule instance (per Table 10-1 MDACn Configurations)
 * @idx:    Domain id within the MDAC, if supported. E.g. MDAC0 supports
 *          up to 8 configurations, while MDAC21 supports 1.
 */
static volatile uint32_t *xrdc_get_mda_by_idx(volatile struct xrdc_regs *xrdc,
					       uint8_t domain, uint8_t idx)
{
	volatile uint32_t *mda_base;

	if (!xrdc)
		return NULL;
	if (domain > 21) {
		ERROR("%s(): domain %d is invalid\n", __func__, domain);
		return NULL;
	}
	if (idx >= 8) {
		ERROR("%s(): index %d is invalid\n", __func__, idx);
		return NULL;
	}
	/* Prevent access to unimplemented MDAs */
	if (idx >= 1) {
		if ((domain >= 2 && domain <= 7) ||
		    (domain >= 11 && domain <= 15) ||
		    (domain >= 19 && domain <= 21)) {
			ERROR("%s(): invalid idx %d for MDAC%d\n", __func__,
				idx, domain);
			return NULL;
		}
	}

	/* MDAx are spaced with a 32-byte stride, so we can pretend they are all
	 * 8-element arrays
	 */
	mda_base = &(xrdc->MDA0[0]) + 8 * domain;
	return mda_base + idx;
}

/**
 * Validate PDAC index and get PDAC struct
 *
 * This function is needed because of the gaps in the XRDC memory map
 */
static volatile struct xrdc_pdac_reg *
xrdc_get_pdac_by_idx(volatile struct xrdc_regs *xrdc, uint32_t idx)
{
	if (!xrdc)
		return NULL;

	if (idx <= 31)
		return &xrdc->PDAC0_31[idx];
	if (idx < 128)
		goto idx_err;
	if (idx <= 158)
		return &xrdc->PDAC128_158[idx - 128];
	if (idx < 256)
		goto idx_err;
	if (idx <= 284)
		return &xrdc->PDAC256_284[idx - 256];
	if (idx < 384)
		goto idx_err;
	if (idx <= 408)
		return &xrdc->PDAC384_408[idx - 384];

idx_err:
	ERROR("%s(): Invalid PDAC index %d\n", __func__, idx);
	return NULL;
}


/* XRDC-Related Function Definitions */

/**
 * xrdc_mem_region() - configure a memory region descriptor
 *
 * @start_addr:       Memory region descriptor start address
 * @end_addr:         Memory region descriptor end address
 * @descriptor:       Memory region descriptor index
 * @access_policy:    Memory region descriptor access policy
 */
static int xrdc_mem_region(volatile struct xrdc_regs *xrdc,
			   uint32_t start_addr, uint32_t end_addr,
			   uint32_t descriptor, uint32_t access_policy)
{
	volatile struct xrdc_mrgd_reg *mrgd;
	uint32_t w3;

	mrgd = xrdc_get_mrgd_by_idx(xrdc, descriptor);
	if (!mrgd) {
		ERROR("%s(): MRGD descriptor index %d is invalid. XRDC mem region 0x%x-0x%x will NOT be configured\n",
			__func__, descriptor, start_addr, end_addr);
		return 1;
	}

	/* set start address */
	XRDC_SET_32(mrgd, W0, start_addr);
	/* set end address */
	XRDC_SET_32(mrgd, W1, end_addr);
	/* set domain access policy */
	XRDC_SET_32(mrgd, W2, access_policy);
	/* set validity bit and lock */
	w3 = XRDC_GET_32(mrgd, W3);
	w3 |= (1u << XRDC_MRGD_W3_VLD_SHIFT) & XRDC_MRGD_W3_VLD_MASK;
	w3 |= (3u << XRDC_MRGD_W3_LK2_SHIFT) & XRDC_MRGD_W3_LK2_MASK;
	XRDC_SET_32(mrgd, W3, w3);

	return 0;
}

/**
 * xrdc_init() - Configure XRDC
 *
 * Assign the bus masters to the proper domains and give all domains read-write
 * access to all peripherals, the whole DDR memory and the system interconnect.
 */
static int xrdc_init(void *vaddr)
{
	unsigned int i;
	volatile uint32_t *mda;
	volatile struct xrdc_pdac_reg *pdac;
	uint8_t nmstr, npac;	/* hwcfg0 */
	uint8_t nmdar;		/* mdacdfg */
	uint32_t val;
	int ret = 0, res = 0;

	/* Global initialization */
	s32g_xrdc = (volatile struct xrdc_regs *)(vaddr);

	/* TODO LK1, LK2 bits to be set by xrdc_enable(), in case the
	 *      initial config grows more complex over time
	 */

	/* assign the debugger to a domain */
	/* TODO conditionally-compile debugger access */
	mda = xrdc_get_mda_by_idx(s32g_xrdc, MDAC_JTAG_DEBUG_MID, 0);
	if (!mda) {
		ERROR("%s(): Error getting MDA for domain %d and index %d; "
		      "XRDC is NOT configured.\n",
		      __func__, MDAC_JTAG_DEBUG_MID, 0);
		ret = 1;
		goto jtag_mda_error;
	}
	val = mmio_read_32((uintptr_t)mda);
	val |= (DOMAIN_DBG << XRDC_MDA_DID_SHIFT) & XRDC_MDA_DID_MASK;
	val |= (1u << XRDC_MDA_VLD_SHIFT) & XRDC_MDA_VLD_MASK;
	val |= (1u << XRDC_MDA_LK1_SHIFT) & XRDC_MDA_LK1_MASK;
	mmio_write_32((uintptr_t)mda, val);

	/* assign every other bus master (in particular, the CA53 clusters)
	 * to the Linux domain
	 * FIXME this is too loose an approach; use a separate domain for the
	 *       CA53 clusters;
	 */
	nmstr = (XRDC_GET_32(s32g_xrdc, HWCFG0) & XRDC_HWCFG0_NMSTR_MASK) >>
		 XRDC_HWCFG0_NMSTR_SHIFT;
	for (i = 0; i <= nmstr; i++) {
		if (i == MDAC_JTAG_DEBUG_MID)
			continue;
		/* no master domain assignment registers for this bus master */
		nmdar = (XRDC_GET_8(s32g_xrdc, MDACFG[i]) &
				XRDC_MDACFG_NMDAR_MASK) >>
			 XRDC_MDACFG_NMDAR_SHIFT;
		if (nmdar == 0)
			continue;

		mda = xrdc_get_mda_by_idx(s32g_xrdc, i, 0);
		if (!mda) {
			ERROR("%s(): Error getting MDA for domain %d and index %d; XRDC is NOT configured.\n",
			      __func__, i, 1);
			ret = 2;
			goto nmstr_mda_error;
		}
		val = mmio_read_32((uintptr_t)mda);
		val |= (DOMAIN_CA53 << XRDC_MDA_DID_SHIFT) &
			XRDC_MDA_DID_MASK;
		val |= (1u << XRDC_MDA_VLD_SHIFT) & XRDC_MDA_VLD_MASK;
		val |= (1u << XRDC_MDA_LK1_SHIFT) & XRDC_MDA_LK1_MASK;
		mmio_write_32((uintptr_t)mda, val);
	}

	/* grant all domains RW access to all peripherals */
	npac = (XRDC_GET_32(s32g_xrdc, HWCFG0) & XRDC_HWCFG0_NPAC_MASK) >>
		XRDC_HWCFG0_NPAC_SHIFT;
	for (i = 0; i <= npac; i++) {
		pdac = xrdc_get_pdac_by_idx(s32g_xrdc, i);
		if (!pdac) {
			ERROR("%s(): PDAC idx %d error; XRDC NOT configured\n",
				__func__, i);
			ret = 3;
			goto pdac_error;
		}
		val = XRDC_GET_32(pdac, W0);
		val |= XRDC_ACP_RW_ALL;
		XRDC_SET_32(pdac, W0, val);

		val = XRDC_GET_32(pdac, W1);
		val |= (1u << XRDC_PDAC_W1_VLD_SHIFT) & XRDC_PDAC_W1_VLD_MASK;
		/* lock register until the next reset */
		val |= (3u << XRDC_PDAC_W1_LK2_SHIFT) &
			    XRDC_PDAC_W1_LK2_MASK;
		XRDC_SET_32(pdac, W1, val);
		/* no semaphore enable, though */
	}

	/* configure FlexNOC descriptor */
	if (xrdc_mem_region(s32g_xrdc, 0x00000000ul, 0xFFFFFFFFul, MRC_FLEXNOC,
			    XRDC_ACP_RW_ALL)) {
		ERROR("%s(): Error configuring mem region 0x%x..0x%x for FlexNOC\n",
		      __func__, 0x00000000, 0xFFFFFFFF);
		ret = 4;
		goto flexnoc_error;
	}

	/* RefMan says (p. 476):
	 * Special care is needed if none of the conditional terms hit in
	 * any Wr evaluation; for this case, the generated DID=0 and software
	 * needs to be aware of any potential access rights granted for this DID
	 * TODO Should add explicit rules for DID=0.
	 */
	i = 0;
	/* DDR0_start ... PMEM_START: allow */
	res = xrdc_mem_region(s32g_xrdc, S32G_DDR0_BASE, S32G_PMEM_START - 1,
			      MRC_DDR0 + i++, XRDC_ACP_RW_ALL);
	/* PMEM_START ... PMEM_END: only allow Secure access */
	res |= xrdc_mem_region(s32g_xrdc, S32G_PMEM_START, S32G_PMEM_END - 1,
			       MRC_DDR0 + i++, XRDC_ACP(DOMAIN_CA53, ACP_SRW));
	/* PMEM_END ... DDR0_end: allow */
	res |= xrdc_mem_region(s32g_xrdc, S32G_PMEM_END, S32G_DDR0_END,
			       MRC_DDR0 + i++, XRDC_ACP_RW_ALL);
	/* FIXME put PMEM at the end of DDR1 instead of DDR0 */
	if (res) {
		ERROR("%s(): Error configuring reserved DRAM\n", __func__);
		ret = 5;
		goto pmem_error;
	}

pmem_error:
flexnoc_error:
pdac_error:
nmstr_mda_error:
jtag_mda_error:
	return ret;
}

/**
 * xrdc_enable() - enable the memory protection mechanism
 *
 * @return 0 for successful initialization, a non-zero value otherwise
 */
int xrdc_enable(void *vaddr)
{
	int ret;
	uint32_t val;

	ret = xrdc_init(vaddr);
	if (ret)
		return ret;

	/* XRDC global enable */
	val = XRDC_GET_32(s32g_xrdc, CR);
	INFO("Not enabling XRDC yet.\n");
	//val |= (1 << XRDC_CR_GVLD_SHIFT) & XRDC_CR_GVLD_MASK;
	val |= (1 << XRDC_CR_LK1_SHIFT) & XRDC_CR_LK1_MASK;
	XRDC_SET_32(s32g_xrdc, CR, val);

	return 0;
}
