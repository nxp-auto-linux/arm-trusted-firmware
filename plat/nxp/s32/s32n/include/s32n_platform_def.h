/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32N_PLATFORM_DEF_H
#define S32N_PLATFORM_DEF_H

#include <common_def.h>

/* Default APS topology */
#define PLATFORM_SYSTEM_COUNT           U(1)
#define PLATFORM_MAX_CPUS_PER_CLUSTER   U(2)
#define PLATFORM_CORE_COUNT		(PLATFORM_CLUSTER_COUNT * \
					 PLATFORM_MAX_CPUS_PER_CLUSTER)

#define PLATFORM_STACK_SIZE		SZ_4K

#define S32N_MPIDR_CPU_MASK_BITS	0x1

/* Power Domains */
#define PLAT_MAX_RET_STATE		U(1)
#define PLAT_MAX_OFF_STATE		U(2)
#define PLAT_NUM_PWR_DOMAINS	(PLATFORM_CORE_COUNT + PLATFORM_CLUSTER_COUNT + \
					PLATFORM_SYSTEM_COUNT)
#define PLAT_MAX_PWR_LVL		MPIDR_AFFLVL2

/* Caches */
#define CACHE_WRITEBACK_SHIFT		U(6)
#define CACHE_WRITEBACK_GRANULE		(UL(1) << CACHE_WRITEBACK_SHIFT)

/**
 * Default DDR memory map after BL2 image loading step.
 *
 * BL2 parses the FIP which encapsulates the BL31, BL33 and DTB images.
 * BL31 is further loaded at the very top of the APS DRAM
 * and BL33 just before BL2, in order to avoid memory fragmentation
 * since BL31 is a resident component and the memory occupied by BL2
 * and BL33 can be later reclaimed.
 *
 * The next addresses are calculated at runtime, these are not defines:
 *
 * BL33_DTB = BL33_ENTRYPOINT - MMU_ROUND_UP_TO_PAGE(get_max_bl33_dtb_size());
 * BL33_IMG_LIMIT = BL2_BASE - MMU_ROUND_UP_TO_PAGE(get_max_bl33_dtb_size()) - 1;
 * S32N_IO_BUFFER_BASE = get_bl33_dtb_base() - S32N_IO_BUFFER_SIZE - PAGE_SIZE;
 *
 * +----------------+ DDR_END
 * |     BL31       |
 * +----------------+ BL31_BASE
 * +----------------+ BL2_LIMIT
 * |     BL2        |
 * +----------------+ BL2_BASE
 * +----------------+ BL33_IMG_LIMIT (as defined above)
 * |     BL33       |
 * +----------------+ BL33_ENTRYPOINT
 * |     DTB        |
 * +----------------+ BL33_DTB (as defined above)
 * +----------------+ S32N_IO_BUFFER_BASE (as defined above) + S32N_IO_BUFFER_SIZE
 * |Temporary buffer|
 * |  in io_block   |
 * |     driver     |
 * +----------------+ S32N_IO_BUFFER_BASE (as defined above)
 * |                |
 * |    .......     |
 * |                |
 * +----------------+ DDR_START
 */

/* Start of the reserved memory for all the BL stages.
 * 0xFFF0_0000 by default.
 */
#define S32N_BL_MEM_BASE        (DDR_END - PLAT_BL_MEM_SIZE + 1)

/* BL2 definitions: BL2 is loaded at the beggining of the
 * DDR reserved memory for the BL stages.
 */
#define BL2_BASE    S32N_BL_MEM_BASE
#define BL2_SIZE    SZ_256K
#define BL2_LIMIT   (BL2_BASE + BL2_SIZE - 1)

/* BL31 definitions: load BL31 at the end of the DDR, but
 * leave a 4KB (page size) gap between components.
 */
#define BL31_BASE   (BL2_LIMIT + PAGE_SIZE + 1)
#define BL31_LIMIT  (DDR_END)
#define BL31_SIZE	(BL31_LIMIT - BL31_BASE + 1)

/* Needed by plat_get_ns_image_entrypoint() default implementation. */
#define PLAT_ARM_NS_IMAGE_BASE BL33_ENTRYPOINT

/* Temporary buffer used by the io block layer. */
#define S32N_IO_BUFFER_SIZE	S32N_FIP_SIZE

#define MAX_IO_DEVICES		U(2)
#define MAX_IO_HANDLES		U(2)

/* Access 64-bit memory adresses. */
#define PLAT_PHY_ADDR_SPACE_SIZE    (ULL(1) << 40)
#define PLAT_VIRT_ADDR_SPACE_SIZE   (ULL(1) << 40)

#define MAX_MMAP_REGIONS		U(13)
#define MAX_XLAT_TABLES			U(13)

#define PLAT_XLAT_TABLES_DYNAMIC	1

/* PL011 defintions */
#define PL011_UART0_BASE	0x4A030000
#define PL011_UART1_BASE	0x4A040000
#define PL011_UART2_BASE	0x4AA50000
#define PL011_UART3_BASE	0x4AA60000
#define PL011_UART4_BASE	0x4A050000
#define PL011_UART5_BASE	0x4A060000
#define PL011_UART6_BASE	0x4AA30000
#define PL011_UART7_BASE	0x4AA40000

#if (S32N_PL011_MODULE == 0)
#define PL011_UART_BASE         PL011_UART0_BASE
#elif (S32N_PL011_MODULE == 1)
#define PL011_UART_BASE         PL011_UART1_BASE
#elif (S32N_PL011_MODULE == 2)
#define PL011_UART_BASE         PL011_UART2_BASE
#elif (S32N_PL011_MODULE == 3)
#define PL011_UART_BASE         PL011_UART3_BASE
#elif (S32N_PL011_MODULE == 4)
#define PL011_UART_BASE         PL011_UART4_BASE
#elif (S32N_PL011_MODULE == 5)
#define PL011_UART_BASE         PL011_UART5_BASE
#elif (S32N_PL011_MODULE == 6)
#define PL011_UART_BASE         PL011_UART6_BASE
#elif (S32N_PL011_MODULE == 7)
#define PL011_UART_BASE         PL011_UART7_BASE
#else
#error Invalid PL011 console configuration!
#endif

#define PL011_UART_SIZE		0x10000
#define PL011_UART_BAUDRATE     (115200)

/* XRDC4_CCID_CHI definitions
 * Local addressing: same XRDC4_CCID address for each APS[0-3].
 */
#define XRDC4_CCID_LOCAL_ADDR	(0x3C0000ul)
#define XRDC4_CCID_PERIPH_ADDR	(APS_PERIPH_BASE + XRDC4_CCID_LOCAL_ADDR)
#define XRDC4_CCID_CHI_SIZE		(0x68)

#define MAX_IO_BLOCK_DEVICES	U(1)

#define USDHC_BASE_ADDR		(0x5B490000UL)
#define USDHC_SIZE		(0x160)

#endif /* S32N_PLATFORM_DEF_H */

