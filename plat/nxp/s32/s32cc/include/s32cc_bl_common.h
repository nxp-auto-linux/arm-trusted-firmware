/*
 * Copyright 2021-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32CC_BL_COMMON_H
#define S32CC_BL_COMMON_H

#include <i2c/s32_i2c.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <s32_bl_common.h>
#include <s32_bl2_common.h>

/* Flags used to encode a cpu state */
/* 1 - CPU is online, 0 - CPU is offline */
#define CPU_ON			BIT(0)
/* 1 - GIC3 cpuiif on, 0 - GIC3 cpuiif off_ */
#define CPUIF_EN		BIT(1)
/*
 * CPU hot unplug method:
 * 1 - the core will use WFI loop
 * 0 - the core will enter in reset
 */
#define CPU_USE_WFI_FOR_SLEEP	BIT(2)
#define HSE_MU0_GCR (0x40210114UL)
#define HSE_MU0_FSR (0x40210104UL)
#define LOCAL_HSE_STATUS_INIT_OK BIT(24)
#define LOCAL_HSE_HOST_PERIPH_CONFIG_DONE BIT(0)

struct s32_i2c_driver {
	struct s32_i2c_bus bus;
	int fdt_node;
};

/* From generated file */
extern const bool fip_location_mmc;
extern const bool fip_location_qspi;
extern const bool fip_location_mem;
extern const unsigned long fip_mem_offset;

bool is_lockstep_enabled(void);

void s32_early_plat_init(void);
void s32_early_plat_setup(void);
void s32_restore_ncore_caiu(uint8_t caiu);

void s32_gic_setup(void);
void plat_gic_save(void);
void plat_gic_restore(void);
void plat_generic_timer_save(void);
void init_cntvoff_el2(void);

void update_core_state(uint32_t core, uint32_t mask, uint32_t flag);
bool is_core_enabled(uint32_t core);
uint32_t get_core_state(uint32_t core, uint32_t mask);
void __dead2 core_turn_off(void);

#ifdef HSE_SUPPORT
void wait_hse_init(void);
#else
static inline void wait_hse_init(void)
{
}
#endif

#if TRUSTED_BOARD_BOOT
void hse_secboot_setup(void);
void hse_sram_rmem_clear(void);
#else
static inline void hse_secboot_setup(void)
{
}
static inline void hse_sram_rmem_clear(void)
{
}
#endif

int s32_mmu_map_dynamic_regions(const mmap_region_t *regions, size_t num);
struct s32_i2c_driver *s32_add_i2c_module(void *fdt, int fdt_node);

static inline bool is_scp_used(void)
{
	return S32CC_USE_SCP;
}

static inline bool is_pinctrl_over_scmi_used(void)
{
	return is_scp_used() && S32CC_USE_SCMI_PINCTRL;
}

static inline bool is_nvmem_over_scmi_used(void)
{
	return is_scp_used() && S32CC_USE_SCMI_NVMEM;
}

static inline bool is_gpio_scmi_fixup_enabled(void)
{
	return S32CC_SCMI_GPIO_FIXUP;
}

static inline bool is_nvmem_scmi_fixup_enabled(void)
{
	return S32CC_SCMI_NVMEM_FIXUP;
}

static inline uintptr_t get_fip_mem_addr(void)
{
	return fip_mem_offset;
}

static inline bool s32cc_has_pinctrl_access(void)
{
	return (S32CC_HAS_PINCTRL_ACCESS == 1);
}

static inline bool s32cc_has_ncore(void)
{
	return (S32CC_HAS_NCORE == 1);
}

static inline bool s32cc_has_clock_access(void)
{
	return (S32CC_HAS_CLOCK_ACCESS == 1);
}

static inline bool s32cc_has_reset_access(void)
{
	return (S32CC_HAS_RESET_ACCESS == 1);
}

#endif /* S32CC_BL_COMMON_H */
