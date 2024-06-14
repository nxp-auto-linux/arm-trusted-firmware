/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <common/debug.h>
#include <common/runtime_svc.h>

#define S32N_SCMI_SIP_ID		0xc2000000U

static int32_t s32n_svc_setup(void)
{
	return 0;
}

static int fss_scmi_handler(uint32_t smc_fid, u_register_t x1,
			    u_register_t x2, u_register_t x3)
{
	/* Forward SCMI requests to FSS */
	return SMC_OK;
}

uintptr_t s32n_svc_smc_handler(uint32_t smc_fid,
			       u_register_t x1,
			       u_register_t x2,
			       u_register_t x3,
			       u_register_t x4,
			       void *cookie,
			       void *handle,
			       u_register_t flags)
{
	switch (smc_fid) {
	case S32N_SCMI_SIP_ID:
		SMC_RET1(handle, fss_scmi_handler(smc_fid, x1, x2, x3));
		break;
	default:
		WARN("Unimplemented SIP Service Call: 0x%x\n", smc_fid);
		SMC_RET1(handle, SMC_UNK);
		break;
	}
}

/* Register Standard Service Calls as runtime service */
DECLARE_RT_SVC(s32n_svc,
	       OEN_SIP_START,
	       OEN_SIP_END,
	       SMC_TYPE_FAST,
	       s32n_svc_setup,
	       s32n_svc_smc_handler
);
