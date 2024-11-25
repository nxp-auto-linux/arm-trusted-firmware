/*
 * Copyright 2021-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32CC_PLATFORM_DEF_H
#define S32CC_PLATFORM_DEF_H

#include <common_def.h>

#define SIZE_1M                 (0x100000)

#define S32_PLAT_PRIMARY_CPU		0x0u	/* Cluster 0, cpu 0*/

#define S32_NCORE_CAIU0_BASE_ADDR		0x50400000
#define S32_NCORE_CAIU0_BASE_ADDR_H		(S32_NCORE_CAIU0_BASE_ADDR >> 16)
#define NCORE_CAIUTC_OFF				0x0
#define NCORE_CAIUTC_ISOLEN_SHIFT		1
#define NCORE_CAIUTC_ISOLEN_MASK		BIT(NCORE_CAIUTC_ISOLEN_SHIFT)

#define S32_CACHE_WRITEBACK_SHIFT	6
#define CACHE_WRITEBACK_GRANULE		(1 << S32_CACHE_WRITEBACK_SHIFT)
#define PLAT_PHY_ADDR_SPACE_SIZE        (1ull << 36)
/* We'll be doing a 1:1 mapping anyway */
#define PLAT_VIRT_ADDR_SPACE_SIZE	    (1ull << 36)

#define PLATFORM_CLUSTER_COUNT		2
#define PLATFORM_SYSTEM_COUNT		1

/* FIXME I'm not sure this is technically correct. We do NOT have
 * cluster-level power management operations, only core and system.
 */
#define PLAT_NUM_PWR_DOMAINS		(PLATFORM_SYSTEM_COUNT + \
					 PLATFORM_CLUSTER_COUNT + \
					 PLATFORM_CORE_COUNT)

#define PLAT_MAX_OFF_STATE		    U(2)
#define PLAT_MAX_RET_STATE		    U(1)
#define PLAT_MAX_PWR_LVL		    MPIDR_AFFLVL2
#define PLAT_MAX_PWR_LVL_STATES		2

#define PLAT_PRIMARY_CPU			0x0

/* Generic timer frequency; this goes directly into CNTFRQ_EL0.
 * Its end-value is 5MHz; this is based on the assumption that
 * GPR00[CA53_COUNTER_CLK_DIV_VAL] contains the reset value of 0x7, hence
 * producing a divider value of 8, applied to the FXOSC frequency of 40MHz.
 */
#define COUNTER_FREQUENCY	    0x004C4B40

#define SIUL2_0_BASE_ADDR		(0x4009C000UL)
#define SIUL2_0_SIZE			(0x179C)

/* A53 Cluster GPR */
#define GPR_BASE_ADDR			(0x4007C400UL)
#define GPR_BASE_PAGE_ADDR		(0x4007C000UL)
#define GPR_SIZE			(0xb4)
#define GPR06_OFF			0x18U
#define GPR09_OFF			0x24U
#define GPR36_OFF			0x90U
#define CA53_RVBARADDR_MASK	(0xFFUL)
/* GPR09 */
#define CA53_0_0_RVBARADDR_39_32_OFF	(0)
#define CA53_0_1_RVBARADDR_39_32_OFF	(8)
#define CA53_1_0_RVBARADDR_39_32_OFF	(16)
#define CA53_1_1_RVBARADDR_39_32_OFF	(24)

#define BOOT_GPR_BASE		0x4007C900UL
#define BOOT_GPR_BMR1_OFF	0
#define BOOT_RCON_MODE_MASK	0x100
#define BOOT_SOURCE_MASK	0xE0
#define BOOT_SOURCE_OFF		5
#define BOOT_SOURCE_QSPI	0
#define BOOT_SOURCE_SD		2
#define BOOT_SOURCE_MMC		3
#define INVALID_BOOT_SOURCE	1

/* GIC (re)definitions */
#define S32GEN1_GIC_BASE	0x50800000
#define PLAT_GICD_BASE		S32GEN1_GIC_BASE
/* SGI to use for kicking the secondary cores out of wfi */
#define S32_SECONDARY_WAKE_SGI	15
/* Used for SCP notifications */
#define S32CC_MAX_IRQ_NUM		10

#define S32_SRAM_BASE		0x34000000
#define S32_SRAM_END		(S32_SRAM_BASE + S32_SRAM_SIZE)

/* Top of the first 2GB bank of physical memory. */
#ifndef S32_PLATFORM_DDR0_END
#define S32_DDR0_END		0xffffffff
#else
#define S32_DDR0_END		S32_PLATFORM_DDR0_END
#endif

/* U-boot addresses in DDR.
 * BL33_MAX_DTB_SIZE and BL33_ENTRYPOINT must be kept in sync
 * with U-Boot's CONFIG_S32GEN1_MAX_DTB_SIZE and CONFIG_SYS_TEXT_BASE.
 */
#define S32_BL33_IMAGE_SIZE	        (7 * SIZE_1M)
/* Leave a gap between BL33 and BL31 to avoid MMU entries merge */
#define BL33_BASE		        (S32_DDR0_END - S32_BL33_IMAGE_SIZE - \
						SIZE_1M + 1)
/* U-Boot: CONFIG_S32GEN1_MAX_DTB_SIZE */
#define BL33_MAX_DTB_SIZE	    (0xf000)
/* U-Boot: CONFIG_SYS_TEXT_BASE  */
#define BL33_ENTRYPOINT		    (BL33_BASE + 0xa0000)
#define BL33_DTB		    (BL33_ENTRYPOINT - BL33_MAX_DTB_SIZE)
#define S32_BL33_IMAGE_BASE	    (BL33_ENTRYPOINT)
#define S32_BL33_LIMIT	        (S32_DDR0_END)

#define S32_PMEM_END		(BL33_BASE - 1)
#define S32_PMEM_LEN		(2 * SIZE_1M)	/* conservatively allow 2MB */
#define S32_PMEM_START		(S32_PMEM_END - S32_PMEM_LEN + 1)

/* BL31 location in DDR - physical addresses only, as the MMU is not
 * configured at that point yet
 */
#define BL31_BASE		(S32_PMEM_START)
#define BL31_LIMIT		(S32_PMEM_END)
#define BL31_SIZE		(BL31_LIMIT - BL31_BASE + 1)

/* BL32 location in DDR - 22MB
 * 20 MB for optee_os (optee_os itself + TA mappings during their execution)
 * 2 MB for shared memory between optee and linux kernel
 *
 * Depending on the intensity of usage of TAs and their sizes,
 * these values can be further shrunk. The current values are preliminary.
 */
#define S32_BL32_SIZE		0x01600000
#define S32_BL32_BASE		(BL31_BASE - S32_BL32_SIZE)
#define S32_BL32_LIMIT		(BL31_BASE)

/* Temporary buffer used by the io block layer. */
#define S32_MMC_BUFFER_SIZE	0x100000
#define S32_MMC_BUFFER_BASE (S32_BL32_BASE - S32_MMC_BUFFER_SIZE)

/* FIXME value randomly chosen; should probably be revisited */
#define PLATFORM_STACK_SIZE		0x4000

#define MAX_IO_HANDLES			4
#define MAX_IO_DEVICES			3
#define MAX_IO_BLOCK_DEVICES	1U

#ifndef PLAT_LOG_LEVEL_ASSERT
#define PLAT_LOG_LEVEL_ASSERT		LOG_LEVEL_VERBOSE
#endif

#define S32_LINFLEX0_BASE	(0x401C8000)
#define S32_LINFLEX0_SIZE	(0x4000)
#define S32_LINFLEX1_BASE	(0x401CC000)
#define S32_LINFLEX1_SIZE	(0x4000)

#define S32_UART_CLOCK_HZ	(125000000)

#if S32_LINFLEX_MODULE == 1
#define S32_UART_BASE		S32_LINFLEX1_BASE
#define S32_UART_SIZE		S32_LINFLEX1_SIZE
#else
#define S32_UART_BASE		S32_LINFLEX0_BASE
#define S32_UART_SIZE		S32_LINFLEX0_SIZE
#endif

#ifndef S32_PLATFORM_OSPM_SCMI_MEM
#define S32_OSPM_SCMI_MEM	(0xd0000000U)
#else /* S32_PLATFORM_OSPM_SCMI_MEM */
#define S32_OSPM_SCMI_MEM       S32_PLATFORM_OSPM_SCMI_MEM
#endif /* S32_PLATFORM_OSPM_SCMI_MEM */
#define S32_OSPM_SCMI_MEM_SIZE	(0x80U)

#define S32_QSPI_BASE		(0x40134000ul)
#define S32_QSPI_SIZE		(0x1000)

#define S32_FLASH_BASE		(0x0)

#define USDHC_BASE_ADDR		(0x402f0000ull)
#define USDHC_SIZE		(0x160)

#define MSCM_BASE_ADDR		(0x40198000U)
#define MSCM_SIZE		(0xfa0u)

#if (SCMI_LOGGER == 1)
#define STM6_BASE_ADDR          (0x40224000UL)
#define STM6_SIZE               (0X3000)
#endif

/**
 * Default memory map used for SCP SCMI communication:
 *
 * -----------------  S32_SCP_SCMI_MEM
 * |  Mailboxes     |
 * |  for each core | cores * S32_SCP_CH_MEM_SIZE
 * |                |
 * ------------------
 * | RX channel for | S32_SCP_CH_MEM_SIZE
 * | notifications  |
 * ------------------ S32_SCP_SCMI_META_MEM
 * | Metadata for   |
 * | each TX        | cores * S32_SCP_CH_META_SIZE
 * | mailbox        |
 * ------------------
 * | Metadata for   | S32_SCP_CH_META_SIZE
 * | RX mailbox		|
 * ------------------
 */

/* Placed at 5MB offset to avoid overlaps, as some drivers require
 * reserved areas at the beginning of the SRAM memory. Note that if
 * custom channel addresses are used, entries in the MMU table might
 * need updating with the new addresses used or new entries should be
 * added.
 */
#define S32_SCP_SCMI_MEM		(0x34500000U)
#define S32_SCP_CH_MEM_SIZE		(128)

#if (S32CC_SCMI_SPLIT_CHAN == 1)
#define S32_SCP_CH_NUM			(2) /* OSPM + PSCI */
#else
/* Each core has its own PSCI channel */
#define S32_SCP_CH_NUM			(PLATFORM_CORE_COUNT)
#endif
/* Keep the same SCP SCMI memory size even if using SCMI split channels, so
 * the same addresses for RX mailbox and metadata can be used.
 */
#define S32_SCP_SCMI_MEM_SIZE	(S32_SCP_CH_MEM_SIZE * PLATFORM_CORE_COUNT)

/*
 * Default memory map used for SCP SCMI communication when
 * S32CC_SCMI_SPLIT_CHAN is set (PSCI + OSPM channels).
 *
 * For simplicity, addresses of the first two TX mailboxes and their corresponding
 * metadata are used, as well as the same RX channel and RX metadata:
 *  - PSCI channel <-> TX mailbox #0
 *  - OSPM channel <-> TX mailbox #1
 * Note that if custom channel addresses are used, entries in the MMU table might
 * need updating with the new addresses used or new entries should be added.
 *
 * ------------------ S32_SCP_SCMI_MEM
 * | PSCI channel	|
 * |(TX mailbox #0) |
 * ------------------
 * | OSPM channel   |
 * |(TX mailbox #1) |
 * ------------------
 * ..................
 * ------------------ S32_SCP_SCMI_MEM +
 * | RX channel for |	cores * S32_SCP_CH_MEM_SIZE
 * | OSPM           |
 * | notifications  |
 * ------------------ S32_SCP_SCMI_META_MEM
 * | Metadata for   |
 * | each TX mailbox| S32_SCP_CH_NUM *
 * | (TX metadata   |	S32_SCP_CH_META_SIZE
 * |   #0 + #1)     |
 * ------------------
 * ..................
 * ------------------ S32_SCP_SCMI_META_MEM +
 * | Metadata for   |	cores * S32_SCP_CH_META_SIZE
 * | RX mailbox     |
 * ------------------
 */

/*
 * Metadata memory region is placed after the actual mailboxes
 */
#define S32_SCP_SCMI_META_MEM		((uintptr_t)S32_SCP_SCMI_MEM + S32_SCP_SCMI_MEM_SIZE + \
						S32_SCP_CH_MEM_SIZE)
#if (SCMI_LOGGER == 1)
#define S32_SCP_CH_META_SIZE		(128)
#else
#define S32_SCP_CH_META_SIZE		(0)
#endif

#define S32_SCP_SCMI_META_ADDR(X)	(((uintptr_t)S32_SCP_SCMI_META_MEM + \
						S32_SCP_CH_META_SIZE * (X)))
/* Used by SCMI Logger, update these in case of metadata customization
 * in device tree.
 */
#define S32_SCP_PSCI_META			(S32_SCP_SCMI_META_ADDR(0))
#define S32_SCP_OSPM_META			(S32_SCP_SCMI_META_ADDR(1))

#define S32_SCP_SCMI_META_MEM_SIZE	(S32_SCP_CH_META_SIZE * (PLATFORM_CORE_COUNT + 1))

/* TX channels + RX channel + metadata */
#define S32_SCP_SCMI_SIZE			(S32_SCP_SCMI_MEM_SIZE + S32_SCP_CH_MEM_SIZE + \
						S32_SCP_SCMI_META_MEM_SIZE)

#define SIUL2_0_MSCR_START	0
#define SIUL2_0_MSCR_END	101
#define SIUL2_0_IMCR_START	512
#define SIUL2_0_IMCR_END	595

#define PLAT_XLAT_TABLES_DYNAMIC	1

#endif /* S32_PLATFORM_H */

