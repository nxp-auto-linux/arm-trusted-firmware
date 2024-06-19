/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <common/debug.h>
#include <common/runtime_svc.h>
#include <drivers/scmi.h>
#include <s32_bl_common.h>
#include <s32_svc.h>
#include <s32n_fss_scmi.h>

#define S32N_SCMI_SIP_ID		0xc2000000U

static int32_t s32n_svc_setup(void)
{
	struct scmi_shared_mem *mem = (void *)S32N_OSPM_SCMI_MEM;
	int ret;

	ret = mmap_add_dynamic_region(S32N_OSPM_SCMI_MEM, S32N_OSPM_SCMI_MEM,
			MMU_ROUND_UP_TO_PAGE(S32N_OSPM_SCMI_MEM_SIZE),
			MT_NON_CACHEABLE | MT_RW | MT_SECURE);
	if (ret) {
		ERROR("Error mapping SCMI OSPM memory region: %d\n", ret);
		panic();
	}

	mem->channel_status = SCMI_SHMEM_CHAN_STAT_CHANNEL_FREE;

	return 0;
}

static int fss_scmi_handler(uint32_t smc_fid, u_register_t x1,
			    u_register_t x2, u_register_t x3)
{
	struct scmi_shared_mem *mem = (void *)S32N_OSPM_SCMI_MEM;
	struct response *response = (struct response *)&mem->msg_payload[0];
	int ret;

	/**
	 * Forward SCMI requests to FSS and get the response.
	 * The channel's ownership is transferred to FSS.
	 * Channel's status should be updated after receiving the response.
	 */
	ret = fss_scmi_send_sync_cmd(S32N_OSPM_SCMI_MEM, S32N_OSPM_SCMI_MEM_SIZE);
	if (ret)
		response->status = ret;

	mem->channel_status = SCMI_SHMEM_CHAN_STAT_CHANNEL_FREE;
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
