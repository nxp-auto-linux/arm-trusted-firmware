/*
 * Copyright 2020-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <clk/s32gen1_scmi_clk.h>
#include <common/debug.h>
#include <common/runtime_svc.h>
#include <drivers/scmi.h>
#include <scmi-msg/common.h>
#include <s32_svc.h>
#include <s32cc_bl_common.h>
#include <s32cc_scp_scmi.h>
#include <s32cc_svc.h>

#define S32_SCMI_ID			0xc20000feU

#define MSG_ID(m)			((m) & 0xffU)
#define MSG_TYPE(m)			(((m) >> 8) & 0x3U)
#define MSG_PRO_ID(m)			(((m) >> 10) & 0xffU)
#define MSG_TOKEN(m)			(((m) >> 18) & 0x3ffU)

static const uint8_t s32_protocols[] = {
	SCMI_PROTOCOL_ID_PERF,
	SCMI_PROTOCOL_ID_CLOCK,
	SCMI_PROTOCOL_ID_RESET_DOMAIN,
	SCMI_PROTOCOL_ID_PINCTRL,
	SCMI_PROTOCOL_ID_GPIO,
	SCMI_PROTOCOL_ID_NVMEM,
	0U,
};

const char *plat_scmi_vendor_name(void)
{
	return "NXP";
}

const char *plat_scmi_sub_vendor_name(void)
{
#if defined(PLAT_s32g2)
	return "S32G274A";
#elif defined(PLAT_s32g3)
	return "S32G399A";
#else
	return "S32R45";
#endif
}

const uint8_t *plat_scmi_protocol_list(unsigned int agent_id)
{
	return s32_protocols;
}

int32_t plat_scmi_reset_agent(unsigned int agent_id)
{
	return plat_scmi_clock_agent_reset(agent_id);
}

size_t plat_scmi_protocol_count(void)
{
	return ARRAY_SIZE(s32_protocols) - 1;
}

static int32_t s32_svc_smc_setup(void)
{
	struct scmi_shared_mem *mem = (void *)S32_OSPM_SCMI_MEM;

	mem->channel_status = SCMI_SHMEM_CHAN_STAT_CHANNEL_FREE;
	return 0;
}

static int scmi_handler(uint32_t smc_fid, u_register_t x1,
			u_register_t x2, u_register_t x3)
{
	struct scmi_shared_mem *mem = (void *)S32_OSPM_SCMI_MEM;
	struct response *response = (struct response *)&mem->msg_payload[0];
	uint32_t msg_header = mem->msg_header;
	struct scmi_msg msg = {
		.in = (char *)&mem->msg_payload[0],
		.in_size = mem->length - 4,
		.agent_id = S32_SCMI_AGENT_OSPM,
		.protocol_id = MSG_PRO_ID(msg_header),
		.message_id = MSG_ID(msg_header),
		.out = (char *)response,
		.out_size = S32_OSPM_SCMI_MEM_SIZE - sizeof(*mem),
	};

	scmi_process_message(&msg);

	mem->length = msg.out_size_out + 4;
	mem->channel_status = 1;

	return 0;
}

static int scp_scmi_handler(uint32_t smc_fid, u_register_t x1,
			    u_register_t x2, u_register_t x3)
{
	struct scmi_shared_mem *mem = (void *)S32_OSPM_SCMI_MEM;
	struct response *response = (struct response *)&mem->msg_payload[0];
	int ret;

	ret = send_scmi_to_scp(S32_OSPM_SCMI_MEM, S32_OSPM_SCMI_MEM_SIZE, OSPM);
	if (ret != SCMI_SUCCESS) {
		response->status = ret;
		mem->channel_status = 1;
		return SMC_UNK;
	}

	return SMC_OK;
}

uintptr_t s32_svc_smc_handler(uint32_t smc_fid,
			       u_register_t x1,
			       u_register_t x2,
			       u_register_t x3,
			       u_register_t x4,
			       void *cookie,
			       void *handle,
			       u_register_t flags)
{
	switch (smc_fid) {
	case S32_SCMI_ID:
		if (is_scp_used()) {
			SMC_RET1(handle, scp_scmi_handler(smc_fid, x1, x2, x3));
		} else {
			SMC_RET1(handle, scmi_handler(smc_fid, x1, x2, x3));
		}
		break;
	default:
		WARN("Unimplemented SIP Service Call: 0x%x\n", smc_fid);
		SMC_RET1(handle, SMC_UNK);
		break;
	}
}

DECLARE_RT_SVC(s32_svc,
	       OEN_SIP_START,
	       OEN_SIP_END,
	       SMC_TYPE_FAST,
	       s32_svc_smc_setup,
	       s32_svc_smc_handler
);


