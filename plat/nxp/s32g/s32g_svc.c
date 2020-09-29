/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <common/debug.h>
#include <common/runtime_svc.h>
#include <drivers/st/scmi.h>
#include <scmi-msg/common.h>

#define S32G_SCMI_ID			0xc20000feU

#define MSG_ID(m)			((m) & 0xffU)
#define MSG_TYPE(m)			(((m) >> 8) & 0x3U)
#define MSG_PRO_ID(m)			(((m) >> 10) & 0xffU)
#define MSG_TOKEN(m)			(((m) >> 18) & 0x3ffU)

struct scmi_shared_mem {
	uint32_t reserved;
	uint32_t channel_status;
#define SCMI_SHMEM_CHAN_STAT_CHANNEL_ERROR      BIT(1)
#define SCMI_SHMEM_CHAN_STAT_CHANNEL_FREE       BIT(0)
	uint32_t reserved1[2];
	uint32_t flags;
#define SCMI_SHMEM_FLAG_INTR_ENABLED    BIT(0)
	uint32_t length;
	uint32_t msg_header;
	uint8_t msg_payload[0];
};

/* Corresponding to msg_payload */
struct response {
	uint32_t status;
	uint32_t data[0];
};

static const uint8_t s32g_protocols[] = {
	SCMI_PROTOCOL_ID_BASE,
	SCMI_PROTOCOL_ID_CLOCK,
};

const char *plat_scmi_vendor_name(void)
{
	return "NXP";
}

const char *plat_scmi_sub_vendor_name(void)
{
	return "S32G274A";
}

const uint8_t *plat_scmi_protocol_list(unsigned int agent_id)
{
	return s32g_protocols;
}

size_t plat_scmi_protocol_count(void)
{
	return sizeof(s32g_protocols);
}

static int32_t s32g_svc_smc_setup(void)
{
	struct scmi_shared_mem *mem = (void *)S32G_SCMI_SHARED_MEM;

	mem->channel_status = SCMI_SHMEM_CHAN_STAT_CHANNEL_FREE;
	return 0;
}

static int scmi_handler(uint32_t smc_fid, u_register_t x1,
			u_register_t x2, u_register_t x3)
{
	struct scmi_shared_mem *mem = (void *)S32G_SCMI_SHARED_MEM;
	struct response *response = (struct response *)&mem->msg_payload[0];
	uint32_t msg_header = mem->msg_header;
	struct scmi_msg msg = {
		.in = (char *)&mem->msg_payload[0],
		.in_size = mem->length - 4,
		.agent_id = 0,
		.protocol_id = MSG_PRO_ID(msg_header),
		.message_id = MSG_ID(msg_header),
		.out = (char *)response,
		.out_size = S32G_SCMI_SHARED_MEM_SIZE - sizeof(*mem),
	};

	scmi_process_message(&msg);

	mem->length = msg.out_size_out + 4;
	mem->channel_status = 1;

	return 0;
}

uintptr_t s32g_svc_smc_handler(uint32_t smc_fid,
			       u_register_t x1,
			       u_register_t x2,
			       u_register_t x3,
			       u_register_t x4,
			       void *cookie,
			       void *handle,
			       u_register_t flags)
{
	switch (smc_fid) {
	case S32G_SCMI_ID:
		SMC_RET1(handle, scmi_handler(smc_fid, x1, x2, x3));
		break;
	default:
		WARN("Unimplemented SIP Service Call: 0x%x\n", smc_fid);
		SMC_RET1(handle, SMC_UNK);
		break;
	}
}

DECLARE_RT_SVC(s32g_svc,
	       OEN_SIP_START,
	       OEN_SIP_END,
	       SMC_TYPE_FAST,
	       s32g_svc_smc_setup,
	       s32g_svc_smc_handler
);


