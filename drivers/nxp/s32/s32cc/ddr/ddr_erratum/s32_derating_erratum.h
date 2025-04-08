/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef S32_DERATING_ERRATUM_H
#define S32_DERATING_ERRATUM_H

#define POLLING_NEEDED_KEY	0x1

#if (ERRATA_S32_050543 == 1)
uint32_t s32_poll_derating_temp_errata(void);
void s32_enable_ddr_erratum_irq(void);
void s32_ddr_erratum_interrupt_config(void);
void s32_ddr_erratum_enable_irq(int pos);
void s32_ddr_erratum_trigger_handler(int intid);

void s32_ddr_erratum_timer_start(void);
void s32_set_ddr_erratum_polling_flag(uint8_t polling_value);
#else
static inline uint32_t s32_poll_derating_temp_errata(void)
{
	return 0;
}
static inline void s32_ddr_erratum_timer_start(void)
{
}
static inline void s32_enable_ddr_erratum_irq(void)
{
}
static inline void s32_ddr_erratum_interrupt_config(void)
{
}
static inline void s32_ddr_erratum_enable_irq(int pos)
{
}
static inline void s32_ddr_erratum_trigger_handler(int intid)
{
}
static inline void s32_set_ddr_erratum_polling_flag(uint8_t polling_value)
{
}
#endif
#endif /* S32_DERATING_ERRATUM_H */
