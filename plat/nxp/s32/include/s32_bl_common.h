/*
 * Copyright 2021-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32_BL_COMMON_H
#define S32_BL_COMMON_H

#include <i2c/s32_i2c.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({			\
	const typeof(((type *)0)->member) * __mptr = (ptr);	\
	(type *)((char *)__mptr - offsetof(type, member)); })

#define UPTR(PTR)			((uintptr_t)(PTR))

#define PAGE_MASK		(PAGE_SIZE_4KB - 1)
#define MMU_ROUND_UP_TO_PAGE(x)	(((x) & ~PAGE_MASK) == (x) ? \
				 (x) : \
				 ((x) & ~PAGE_MASK) + PAGE_SIZE_4KB)

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

struct s32_i2c_driver {
	struct s32_i2c_bus bus;
	int fdt_node;
};

/* From generated file */
extern const unsigned long fip_mmc_offset;
extern const unsigned long fip_qspi_offset;
extern const unsigned long fip_mem_offset;
extern const unsigned int fip_hdr_size;
extern const unsigned int dtb_size;

bool is_lockstep_enabled(void);

void s32_early_plat_init(void);

void s32_gic_setup(void);
void plat_gic_save(void);
void plat_gic_restore(void);

void update_core_state(uint32_t core, uint32_t mask, uint32_t flag);
bool is_core_enabled(uint32_t core);
uint32_t get_core_state(uint32_t core, uint32_t mask);
bool is_last_core(void);
bool is_cluster0_off(void);
bool is_cluster1_off(void);
void __dead2 core_turn_off(void);

struct s32_i2c_driver *s32_add_i2c_module(void *fdt, int fdt_node);

static inline unsigned int get_bl2_dtb_size(void)
{
	if (!dtb_size)
		panic();

	return dtb_size;
}

static inline uintptr_t get_bl2_dtb_base(void)
{
	return BL2_BASE - get_bl2_dtb_size();
}

static inline uintptr_t get_fip_hdr_base(void)
{
	if (!fip_hdr_size)
		panic();

	return get_bl2_dtb_base() - fip_hdr_size;
}

static inline bool is_scp_used(void)
{
	return S32CC_USE_SCP;
}

static inline bool is_pinctrl_over_scmi_used(void)
{
	return is_scp_used() && S32CC_USE_SCMI_PINCTRL;
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

#endif /* S32_BL_COMMON_H */
