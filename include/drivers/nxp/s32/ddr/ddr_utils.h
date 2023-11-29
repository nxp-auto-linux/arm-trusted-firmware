/*
 * Copyright 2020-2023 NXP
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DDR_UTILS_H_
#define DDR_UTILS_H_

#include <lib/mmio.h>

#ifdef TRUSTED_BOARD_BOOT
#include <ddr_plat.h>
#else
#include <stdbool.h>
#include <stdlib.h>
#endif

/* Uncomment to store the CSR registers after executing DDR training */
/* #define STORE_CSR_ENABLE */

#ifndef dsb
#define dsb()	__asm("DSB SY")
#endif

/* Possible errors */
#define NO_ERR              0x00000000U
#define TIMEOUT_ERR         0x00000002U
#define TRAINING_FAILED     0x00000003U
#define BITFIELD_EXCEEDED   0x00000004U
#define DEASSERT_FAILED	    0x00000005U

/* DDRC related */
#define DDRC_BASE_ADDR                   ((uint32_t)0x403C0000U)
#define OFFSET_DDRC_SWCTL                ((uint32_t)0x320U)
#define OFFSET_DDRC_DFIMISC              ((uint32_t)0x1b0U)
#define OFFSET_DDRC_DFISTAT              ((uint32_t)0x1bcU)
#define OFFSET_DDRC_PWRCTL               ((uint32_t)0x30U)
#define OFFSET_DDRC_SWSTAT               ((uint32_t)0x324U)
#define OFFSET_DDRC_STAT                 ((uint32_t)0x04U)
#define OFFSET_DDRC_DBG1                 ((uint32_t)0x304U)
#define OFFSET_DDRC_ECCCFG0              ((uint32_t)0x70U)
#define OFFSET_DDRC_ECCCFG1              ((uint32_t)0x74U)
#define OFFSET_DDRC_SBRCTL               ((uint32_t)0xf24U)
#define OFFSET_DDRC_SBRSTAT              ((uint32_t)0xf28U)
#define OFFSET_DDRC_SBRWDATA0            ((uint32_t)0xf2cU)
#define OFFSET_DDRC_MRSTAT               ((uint32_t)0x18U)
#define OFFSET_DDRC_MRCTRL0              ((uint32_t)0x10U)
#define OFFSET_DDRC_MRCTRL1              ((uint32_t)0x14U)

#if (ERRATA_S32_050543 == 1)
#define OFFSET_DDRC_DERATEEN             ((uint32_t)0x20U)
#define OFFSET_DDRC_RFSHTMG              ((uint32_t)0x64U)
#define OFFSET_DDRC_DRAMTMG0             ((uint32_t)0x100U)
#define OFFSET_DDRC_DRAMTMG1             ((uint32_t)0x104U)
#define OFFSET_DDRC_DRAMTMG4             ((uint32_t)0x110U)
#endif

#define OFFSET_DDRC_DRAMTMG2             (uint32_t)0x108
#define OFFSET_DDRC_INIT6                (uint32_t)0xe8
#define OFFSET_DDRC_INIT7                (uint32_t)0xec
#define OFFSET_DDRC_RANKCTL              (uint32_t)0xf4
#define OFFSET_DDRC_DFITMG0              (uint32_t)0x190
#define OFFSET_DDRC_DFITMG1              (uint32_t)0x194

/* DDRC masks and values */
#define MSTR_LPDDR4_VAL		((uint32_t)0x20)
#define SWSTAT_SW_DONE		1U
#define SWSTAT_SW_NOT_DONE	0U
#define SWCTL_SWDONE_DONE	0x1
#define SWCTL_SWDONE_ENABLE	0x0
#define SWSTAT_SWDONE_ACK_MASK	0x1U

#define MSTR_DRAM_MASK		((uint32_t)0x3f)
#define MSTR_ACT_RANKS_MASK ((uint32_t)0x3000000)
#define MSTR_DUAL_RANK_VAL  ((uint32_t)0x3000000)
#define MSTR_BURST_RDWR_POS 16
#define MSTR_BURST_RDWR_MASK ((uint16_t)0xf)
#define DFITMG0_PHY_CLK_POS  15
#define DFITMG0_PHY_CLK_MASK ((uint16_t)0x1)
#define DRAMTMG2_RD_WR_POS  8
#define DRAMTMG2_RD_WR_MASK 0x1f
#define DRAMTMG2_WR_RD_POS  0
#define DRAMTMG2_WR_RD_MASK 0x1f
#define INIT6_MR5_MASK      0xffffU
#define INIT7_MR6_MASK      0xffffU
#define DFITMG1_WRDATA_DELAY_POS 16
#define DFITMG1_WRDATA_DELAY_MASK ((uint32_t)0x1f)
#define RANKCTL_RD_GAP_POS 4
#define RANKCTL_RD_GAP_MASK ((uint32_t)0xf)
#define RANKCTL_WR_GAP_POS 8
#define RANKCTL_WR_GAP_MASK ((uint32_t)0xfU)

#if (ERRATA_S32_050543 == 1)
#define RFSHTMG_VAL_SHIFT           16
#define RFSHTMG_VAL                 ((uint32_t)0xfffU)
#define RFSHTMG_MASK                (RFSHTMG_VAL << \
	RFSHTMG_VAL_SHIFT)
#define RFSHCTL3_UPDATE_SHIFT       1
#define RFSHCTL3_AUTO_REFRESH_VAL   0x1U
#define RFSHCTL3_MASK               (RFSHCTL3_AUTO_REFRESH_VAL \
	<< RFSHCTL3_UPDATE_SHIFT)
#define DERATEEN_ENABLE		0x1U
#define DRAMTMG4_TRCD_POS	24
#define DRAMTMG4_TRCD_MASK	0x1f
#define DRAMTMG4_TRRD_POS	8
#define DRAMTMG4_TRRD_MASK	0xf
#define DRAMTMG0_TRAS_POS	0
#define DRAMTMG0_TRAS_MASK	0x3f
#define DRAMTMG4_TRP_POS	0
#define DRAMTMG4_TRP_MASK	0x1f
#define DRAMTMG1_TRC_POS	0
#define DRAMTMG1_TRC_MASK	0x7f
#define SUCCESSIVE_READ		0x2U
#define	DERATEEN_MASK_DIS	0x1U
#define DERATEEN_DERATE_BYTE_SHIFT	4
#define DERATEEN_DERATE_BYTE_MASK	0xfU
#define DERATE_BYTE_1			0x1U

#define RFSHTMG_UPDATE_SHIFT		2
#define RFSHCTL3_UPDATE_LEVEL_TOGGLE	0x1U
#define DRAMTMG4_TRCD_DELTA_TIME	2
#define DRAMTMG4_TRRD_DELTA_TIME	2
#define DRAMTMG0_TRAS_DELTA_TIME	2
#define DRAMTMG4_TRP_DELTA_TIME		2
#define DRAMTMG1_TRC_DELTA_TIME		3
#define ERRATA_CHANGES_REVERTED		1
#define ERRATA_CHANGES_UNMODIFIED	0
#endif

#define CSS_SELSTAT_MASK		0x3f000000U
#define	CSS_SELSTAT_POS			24
#define	CSS_SWIP_POS			16
#define	CSS_SW_IN_PROGRESS		0x1U
#define	CSS_SW_COMPLETED		0x0U
#define	CSC_SELCTL_MASK			0xC0FFFFFFU
#define CSC_SELCTL_POS			24
#define	CSC_CLK_SWITCH_REQUEST		0x1U
#define	CSC_CLK_SWITCH_POS		2
#define	CSS_SW_AFTER_REQUEST_SUCCEDED	0x1U
#define	CSS_SW_TRIGGER_CAUSE_POS	17

#define DDR_SS_AXI_PARITY_ENABLE_MASK	0x00001FF0U
#define DDR_SS_AXI_PARITY_TYPE_MASK	0x01FF0000U
#define DDR_SS_DFI_1_ENABLED		0x1U
#define FORCED_RESET_ON_PERIPH		0x1U
#define PRST_0_PERIPH_3_RST_POS		3
#define DBG1_DISABLE_DE_QUEUEING	0x0U
#define RFSHCTL3_DISABLE_AUTO_REFRESH	0x1U
#define ENABLE_AXI_PORT			0x000000001

#define PWRCTL_POWER_DOWN_ENABLE_MASK		0x00000002U
#define PWRCTL_SELF_REFRESH_ENABLE_MASK		0x00000001U
#define PWRCTL_EN_DFI_DRAM_CLOCK_DIS_MASK	0x00000008U
#define DFIMISC_DFI_INIT_COMPLETE_EN_MASK	0x000000001U

#define MASTER0_CAL_ACTIVE		0x1U
#define MASTER0_CAL_DONE		0x0U
#define	DFIMISC_DFI_INIT_START_MASK	0x00000020U
#define	DFISTAT_DFI_INIT_DONE		0x1U
#define	DFISTAT_DFI_INIT_INCOMPLETE	0x0U
#define	PWRCTL_SELFREF_SW_MASK		0x00000020U
#define	STAT_OPERATING_MODE_MASK	0x7U
#define	STAT_OPERATING_MODE_INIT	0x0U
#define	RFSHCTL3_DIS_AUTO_REFRESH_MASK	0x00000001U
#define	ECCCFG0_ECC_MODE_MASK		0x7U
#define	ECCCFG0_ECC_DISABLED		0x0U
#define	TRAINING_OK_MSG			0x07U
#define	TRAINING_FAILED_MSG		0xFFU
#define	ECCCFG1_REGION_PARITY_LOCKED	((uint32_t)0x1U)
#define	ECCCFG1_REGION_PARITY_LOCK_POS	4
#define	SBRCTL_SCRUB_MODE_WRITE		((uint32_t)0x1U)
#define	SBRCTL_SCRUB_MODE_POS		2

#define	APBONLY_DCTWRITEPROT_ACK_EN              0
#define	APBONLY_DCTWRITEPROT_ACK_DIS             1
#define	SBRCTL_SCRUB_DURING_LOWPOWER_CONTINUED   ((uint32_t)0x1U)
#define	SBRCTL_SCRUB_DURING_LOWPOWER_POS         1

#define	SBRCTL_SCRUB_INTERVAL_FIELD     0x1FFFU
#define	SBRCTL_SCRUB_INTERVAL_POS       8
#define	SBRCTL_SCRUB_EN	                0x1U
#define	SBRSTAT_SCRUB_DONE_MASK         0x2U
#define	SBRSTAT_SCRUBBER_NOT_DONE       0x0U
#define	SBRSTAT_SCRUBBER_BUSY_MASK      0x1U
#define	SBRSTAT_SCRUBBER_NOT_BUSY       0x0U
#define	SBRCTL_SCRUB_INTERVAL_VALUE_1   ((uint32_t)0x1U)
#define	MRR_0_DDR_SEL_REG_MASK          0x1U

#define	MRSTAT_MR_BUSY                  0x1U
#define	MRSTAT_MR_NOT_BUSY              0x0U
#define	MRCTRL0_MR_TYPE_READ            0x1U
#define	MRCTRL0_RANK_ACCESS_POS         4
#define	MRCTRL0_RANK_ACCESS_FIELD       ((uint32_t)0xfU)
#define	MRCTRL0_RANK_0                  0x1U
#define	MRCTRL1_MR_ADDRESS_FIELD        ((uint32_t)0xffU)
#define	MRCTRL1_MR_ADDRESS_POS          8
#define	MRCTRL0_WR_ENGAGE               ((uint32_t)0x1U)
#define	MRCTRL0_WR_ENGAGE_POS           31
#define	MRCTRL1_MR_DATA_ADDRESS_FIELD   ((uint32_t)0xffffU)
#define	MRCTRL1_MR_DATA_ADDRESS_POS     16
#define STORE_CSR_DISABLED              0x0U
#define INIT_MEM_DISABLED               0x0U
#define ADJUST_DDRC_DISABLED            0x0U

/* Performance monitoring registers */
#define PERF_BASE_ADDR                   ((uint32_t)0x403E0000U)
#define OFFSET_MRR_0_DATA_REG_ADDR       ((uint32_t)0x40U)
#define OFFSET_MRR_1_DATA_REG_ADDR       ((uint32_t)0x44U)

/* uMCTL2 Multi-Port Registers */
#define DDRC_UMCTL2_MP_BASE_ADDR         ((uint32_t)0x403C03F8U)
#define OFFSET_DDRC_PCTRL_0              ((uint32_t)0x98U)
#define OFFSET_DDRC_PCTRL_1              ((uint32_t)0x148U)
#define OFFSET_DDRC_PCTRL_2              ((uint32_t)0x1f8U)

/* PHY related */
#define DDR_PHYA_MASTER0_CALBUSY		0x4038165C
#define DDR_PHYA_APBONLY_UCTSHADOWREGS		0x40380404U
#define UCT_WRITE_PROT_SHADOW_MASK              0x1U
#define DDR_PHYA_DCTWRITEPROT			0x4038040C
#define DDR_PHYA_APBONLY_UCTWRITEONLYSHADOW	0x40380410
#define OFFSET_DDRC_RFSHCTL3			((uint32_t)0x60U)
#define DDR_PHYA_UCCLKHCLKENABLES		0x40380BEC
#define UCT_WRITE_PROT_SHADOW_ACK		0x0U
#define TXDQDLY_COARSE				6
#define DDRPHY_PIPE_DFI_MISC			1U
#define ARDPTR_INITVAL_ADDR			0x40381494

#define CDD_CHA_RR_1_0    0x403b004c
#define CDD_CHA_RR_0_1    0x403b004d
#define CDD_CHA_RW_1_1    0x403b0050
#define CDD_CHA_RW_1_0    0x403b0051
#define CDD_CHA_RW_0_1    0x403b0054
#define CDD_CHA_RW_0_0    0x403b0055
#define CDD_CHA_WR_1_1    0x403b0058
#define CDD_CHA_WR_1_0    0x403b0059
#define CDD_CHA_WR_0_1    0x403b005c
#define CDD_CHA_WR_0_0    0x403b005d
#define CDD_CHA_WW_1_0    0x403b0060
#define CDD_CHA_WW_0_1    0x403b0061

#define CDD_CHB_RR_1_0    0x403b00b1
#define CDD_CHB_RR_0_1    0x403b00b4
#define CDD_CHB_RW_1_1    0x403b00b5
#define CDD_CHB_RW_1_0    0x403b00b8
#define CDD_CHB_RW_0_1    0x403b00b9
#define CDD_CHB_RW_0_0    0x403b00bc
#define CDD_CHB_WR_1_1    0x403b00bd
#define CDD_CHB_WR_1_0    0x403b00c0
#define CDD_CHB_WR_0_1    0x403b00c1
#define CDD_CHB_WR_0_0    0x403b00c4
#define CDD_CHB_WW_1_0    0x403b00c5
#define CDD_CHB_WW_0_1    0x403b00c8

#define CDD_CHA_RR_1_0_DDR3   0x403b0059
#define CDD_CHA_RR_0_1_DDR3   0x403b0060
#define CDD_CHA_RW_1_1_DDR3   0x403b008d
#define CDD_CHA_RW_1_0_DDR3   0x403b0090
#define CDD_CHA_RW_0_1_DDR3   0x403b0095
#define CDD_CHA_RW_0_0_DDR3   0x403b0098
#define CDD_CHA_WR_1_1_DDR3   0x403b00ad
#define CDD_CHA_WR_1_0_DDR3   0x403b00b0
#define CDD_CHA_WR_0_1_DDR3   0x403b00b5
#define CDD_CHA_WR_0_0_DDR3   0x403b00b8
#define CDD_CHA_WW_1_0_DDR3   0x403b0071
#define CDD_CHA_WW_0_1_DDR3   0x403b0078

#define DBYTE0_TXDQSDLYTG0_U0 0x40394b4c
#define DBYTE0_TXDQSDLYTG0_U1 0x40394b50
#define DBYTE1_TXDQSDLYTG0_U0 0x40396b4c
#define DBYTE1_TXDQSDLYTG0_U1 0x40396b50
#define DBYTE2_TXDQSDLYTG0_U0 0x40398b4c
#define DBYTE2_TXDQSDLYTG0_U1 0x40398b50
#define DBYTE3_TXDQSDLYTG0_U0 0x4039ab4c
#define DBYTE3_TXDQSDLYTG0_U1 0x4039ab50

#define DBYTE0_TXDQSDLYTG1_U0 0x40394b6c
#define DBYTE0_TXDQSDLYTG1_U1 0x40394b70
#define DBYTE1_TXDQSDLYTG1_U0 0x40396b6c
#define DBYTE1_TXDQSDLYTG1_U1 0x40396b70
#define DBYTE2_TXDQSDLYTG1_U0 0x40398b6c
#define DBYTE2_TXDQSDLYTG1_U1 0x40398b70
#define DBYTE3_TXDQSDLYTG1_U0 0x4039ab6c
#define DBYTE3_TXDQSDLYTG1_U1 0x4039ab70

#define VREF_CA_A0 0x403b0095
#define VREF_CA_A1 0x403b0098
#define VREF_CA_B0 0x403b00fc
#define VREF_CA_B1 0x403b00fd

#define VREF_DQ_A0 0x403b0099
#define VREF_DQ_A1 0x403b009c
#define VREF_DQ_B0 0x403b0100
#define VREF_DQ_B1 0x403b0101

#define SHIFT_BIT(nr)             (((uint32_t)0x1U) << (nr))
#define UCCLKEN_MASK              SHIFT_BIT(0)
#define HCLKEN_MASK               SHIFT_BIT(1)
#define OFFSET_DDRC_INIT0         ((uint32_t)0xd0U)

#define STORE_CSR_MASK            SHIFT_BIT(0)
#define INIT_MEM_MASK             SHIFT_BIT(1)
#define ADJUST_DDRC_MASK          SHIFT_BIT(2)
#define SCRUB_EN_MASK             SHIFT_BIT(0)
#define SCRUB_BUSY_MASK           SHIFT_BIT(0)
#define SELFREF_SW_MASK           SHIFT_BIT(5)
#define SELFREF_TYPE_MASK         (SHIFT_BIT(4) | SHIFT_BIT(5))
#define OPERATING_MODE_MASK       (SHIFT_BIT(0) | SHIFT_BIT(1) | SHIFT_BIT(2))
#define DFI_INIT_COMPLETE_MASK    SHIFT_BIT(0)
#define DFI_INIT_START_MASK       SHIFT_BIT(5)
#define SW_DONE_ACK_MASK          SHIFT_BIT(0)
#define SW_DONE_MASK              SHIFT_BIT(0)
#define SKIP_DRAM_INIT_MASK       (SHIFT_BIT(30) | SHIFT_BIT(31))

#if !defined(PLAT_s32r)
/* Standby SRAM */
#define STNDBY_RAM_BASE           0x24000000

/*
* This should be overwritten to store the configuration registers at different
* address. Default one is the beginning of standby RAM.
*/
#ifndef RETENTION_ADDR
#define RETENTION_ADDR            STNDBY_RAM_BASE
#endif
#endif

/* DDR Subsystem */
#define DDR_SS_REG                0x403D0000

/* Reset Generation Module */
#define MC_RGM_PRST_0             0x40078040
#define MG_RGM_PSTAT_0            0x40078140
#ifndef MC_CGM5_BASE_ADDR
#define MC_CGM5_BASE_ADDR         ((uint32_t)0x40068000U)
#endif
#define OFFSET_MUX_0_CSS          ((uint32_t)0x304U)
#define OFFSET_MUX_0_CSC          ((uint32_t)0x300U)
#define FIRC_CLK_SRC              0x0U
#define DDR_PHI0_PLL              0x24U

/* Default timeout for DDR PHY operations */
#define DEFAULT_TIMEOUT 1000000

/* Start addresses of IMEM and DMEM memory areas */
#define IMEM_START_ADDR 0x403A0000
#define DMEM_START_ADDR 0x403B0000

#if (ERRATA_S32_050543 == 1)
/* ERR050543 related defines */
#define MR4_IDX            4
#define MR4_MASK           0xFFU
#define REF_RATE_MASK      0x7U
#define BYTE_SHIFT         8
#define TUF_THRESHOLD      0x3U
#define REQUIRED_OK_CHECKS 0x3U
#endif

#if !defined(PLAT_s32g3)
#define OFFSET_DFIPHYMSTR   ((uint32_t)0x1C4U)
#define DFIPHYMSTR_ENABLE   0x1U
#define DFIPHYMSTR_DISABLED 0x0U
#define SELFREF_TYPE_POS    4
#define PHY_MASTER_REQUEST  0x1U
#endif

/* ERR050760 related defines */
#define REQUIRED_MRSTAT_READS 0x2U

/* Compute the number of elements in the given array */
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

struct cdd_type {
	uint8_t rr;
	uint8_t rw;
	uint8_t wr;
	uint8_t ww;
};

struct space_timing_params {
	struct cdd_type cdd;
	uint8_t vref_ca;
	uint8_t vref_dq;
	uint16_t tphy_wrdata_delay;
};

#if (ERRATA_S32_050543 == 1)
extern uint8_t polling_needed;
#endif

/* Set default AXI parity. */
uint32_t set_axi_parity(void);

/* Deassert DDR controller and AXI ports reset signal */
uint32_t deassert_ddr_reset(void);

/*
 * Post PHY train setup - complementary settings
 * that needs to be performed after running the firmware.
 * @param options - various flags controlling post training actions
 * (whether to init memory with ECC scrubber / whether to store CSR)
 */
uint32_t post_train_setup(uint8_t options);

/* Wait until firmware finishes execution and return training result */
uint32_t wait_firmware_execution(void);

/* Read lpddr4 mode register.
 * @param mr_rank - rank access
 * @param mr_index - index of mode register to be read
 */
uint32_t read_lpddr4_mr(uint8_t mr_rank, uint8_t mr_index);

/* Write lpddr4 mode register
 * @param mr_rank - rank access
 * @param mr_index - index of mode register to be read
 * @param mr_data - data to be written
 */
uint32_t write_lpddr4_mr(uint8_t mr_rank, uint8_t mr_index, uint8_t mr_data);

/* Modify bitfield value with delta, given bitfield position and mask */
bool update_bf(uint32_t *v, uint8_t pos, uint32_t mask, int32_t delta);

/* Read Critical Delay Differences from message block and store max values */
void read_cdds(void);

/* Read trained VrefCA from message block and store average value */
void read_vref_ca(void);

/* Read trained VrefDQ from message block and store average value */
void read_vref_dq(void);

/* Calculate DFITMG1.dfi_t_wrdata_delay */
void compute_tphy_wrdata_delay(void);

#if (ERRATA_S32_050543 == 1)
/* Read Temperature Update Flag from lpddr4 MR4 register. */
uint8_t read_tuf(void);

/*
 * Enable ERR050543 errata workaround.
 * If the system is hot or cold prior to enabling derating, Temperature Update
 * Flag might not be set in MR4 register, causing incorrect refresh period and
 * derated timing parameters (tRCD, tRAS, rRP, tRRD being used.
 * Software workaround requires reading MR register and adjusting timing
 * parameters, if necessary.
 */
uint32_t enable_derating_temp_errata(void);

/*
 * Periodically read Temperature Update Flag in MR4 and undo changes made by
 * ERR050543 workaround if no longer needed. Refresh rate is updated and auto
 * derating is turned on.
 * @param traffic_halted - if ddr traffic was halted, restore also timing
 * parameters
 * @return - Returns 1, if the errata changes are reverted, 0 otherwise
 */
uint32_t poll_derating_temp_errata(bool traffic_halted);

#endif

#endif /* DDR_UTILS_H_ */
