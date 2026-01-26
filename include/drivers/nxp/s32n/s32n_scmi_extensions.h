// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2025-2026 NXP
 */

#ifndef S32N_SCMI_EXTENSIONS_H
#define S32N_SCMI_EXTENSIONS_H

#include <stdint.h>

/*
 * Reset Event Types. Should be kept in sync with FSS.
 */
enum plat_reset_event_type {
	RESET_UNDEFINED,
	POWER_ON_RESET,
	DESTRUCTIVE_RESET,
	DESTRUCTIVE_RESET_DURING_STANDBY,
	FUNCTIONAL_RESET,
	FUNCTIONAL_RESET_DURING_STANDBY,
	STARTUP_SELFTEST_COMPLETE,
	WAKEUP_TO_RUN,
	WAKEUP_TO_HK
};

/*
 * Reset Platform Reason
 */
struct scmi_reset_plat_reason_p2a {
	int32_t status;
	uint32_t plat_reset_reason;
} __packed;

/*
 * Identifiers of the Reset Protocol commands extensions
 */
enum scmi_reset_command_extensions_id {
	SCMI_RESET_GET_PLATFORM_REASON = 0x25,
};

/*
 * System Power State Notifier
 */
#define SYS_PWR_FORCEFUL_REQ_MASK	BIT_32(0)
#define S32_SYS_PWR_SHUTDOWN_VAL		(0x0)
#define S32_SYS_PWR_STOP_VAL			(0x1)
#define S32_SYS_PWR_SUSPEND_VAL			(0x2)
#define S32_SYS_PWR_SUSPEND_TO_RAM_VAL	(0x3)
#define S32_SYS_PWR_WARM_RESET_VAL		(0x4)

struct sys_power_event {
	uint32_t agent_id;
	uint32_t flags;
	uint32_t system_state;
	uint32_t timeout;
};

#endif  /* S32N_SCMI_EXTENSIONS_H */
