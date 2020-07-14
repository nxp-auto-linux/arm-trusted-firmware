/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32G_RESUME_H
#define S32G_RESUME_H

/* Secondary cores entry point */
void plat_secondary_cold_boot_setup(void);
void s32g_resume_entrypoint(void);

#endif
