/*
 * Copyright 2021-2022 NXP
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

struct s32_i2c_driver {
	struct s32_i2c_bus bus;
	int fdt_node;
};

/* From generated file */
extern const unsigned long fip_sd_offset;
extern const unsigned long fip_emmc_offset;
extern const unsigned long fip_qspi_offset;
extern const unsigned long fip_mem_offset;

bool is_lockstep_enabled(void);

void s32_early_plat_init(bool skip_ddr_clk);

void s32_gic_setup(void);
void plat_gic_save(void);
void plat_gic_restore(void);

void update_core_state(uint32_t core, uint32_t state);
bool is_last_core(void);
bool is_cluster0_off(void);
bool is_cluster1_off(void);

struct s32_i2c_driver *s32_add_i2c_module(void *fdt, int fdt_node);

#endif /* S32_BL_COMMON_H */
