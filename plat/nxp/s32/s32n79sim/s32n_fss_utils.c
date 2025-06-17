/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <arm/css/scmi/scmi_private.h>
#include <drivers/nxp/s32n/s32n_nxp_misc.h>
#include <drivers/scmi.h>
#include <scmi-msg/nxp-core.h>

#include <s32n_fss_scmi.h>

#define ADDR_MASK		GENMASK(31, 0)
#define ADDR_SHIFT		U(32)

/* Display information regarding FSS FW */
static void dump_fss_info(struct scmi_nxp_misc_get_firmware_info_p2a
		*payload)
{
	uint32_t fss_maj_ver, fss_min_ver, fss_patch_ver, fss_cd_ver;
	const char *derivative = NULL;
	const char *fw_type = NULL;
	char fss_version[50];
	int len;

	fss_maj_ver = (payload->version & FSS_FIRMWARE_MAJOR_VER_MASK) >>
		FSS_FIRMWARE_MAJOR_VER_SHIFT;
	fss_min_ver = (payload->version & FSS_FIRMWARE_MINOR_VER_MASK) >>
		FSS_FIRMWARE_MINOR_VER_SHIFT;
	fss_patch_ver = (payload->version & FSS_FIRMWARE_PATCH_VER_MASK) >>
		FSS_FIRMWARE_PATCH_VER_SHIFT;
	fss_cd_ver = payload->version & FSS_FIRMWARE_CODE_DROP_VER_MASK;

	len = snprintf(fss_version, sizeof(fss_version), "%d.%d.%d",
			fss_maj_ver, fss_min_ver, fss_patch_ver);
	assert((len > 0) && (len < sizeof(fss_version)));
	if (fss_cd_ver) {
		len += snprintf(fss_version + len, sizeof(fss_version) - len,
				" CD%d", fss_cd_ver);
		assert((len > 0) && (len < sizeof(fss_version)));
	}
	if (payload->hotfix_version) {
		len += snprintf(fss_version + len, sizeof(fss_version) - len,
				" HF%d", payload->hotfix_version);
		assert((len > 0) && (len < sizeof(fss_version)));
	}

	INFO("FSS Firmware Info:\n");
	INFO("\tVersion: %s\n", fss_version);
	INFO("\tBuild Time & Date: %s, %s\n", payload->build_time,
			payload->build_date);

	switch (payload->firmware_type) {
	case 0:
		fw_type = "Standard";
		break;
	case 1:
		fw_type = "Premium";
		break;
	case 2 ... 7:
		/* Reserved */
		break;
	case 8:
		fw_type = "Custom";
		break;
	default:
		fw_type = "Invalid";
		break;
	}
	if (fw_type)
		INFO("\tFirmware Type: %s\n", fw_type);

	switch (payload->derivative) {
	case 0:
		derivative = "S32N7";
		break;
	case 1:
		derivative = "S32N5";
		break;
	default:
		/* Reserved */
		break;
	}
	if (derivative)
		INFO("\tDerivative: %s\n", derivative);

	INFO("\tHW Support: %d\n", payload->hw_support);
	INFO("\tExternal Device Support: %d\n",
			payload->external_device_support);
}

/* Display versions of the resources */
static void dump_data_version(struct scmi_nxp_misc_get_data_version_p2a
		*payload)
{
	INFO("Device Versions:\n");
	INFO("\tBoot Data: %d\n", payload->boot_data_version);
	INFO("\tClock Data: %d\n", payload->clock_data_version);
	INFO("\tDSC Irq Data: %d\n",
			payload->dsc_irq_data_version);
	INFO("\tEIM Data: %d\n", payload->eim_data_version);
	INFO("\tERM Data: %d\n", payload->erm_data_version);
	INFO("\tFCCU Data: %d\n", payload->fccu_data_version);
	INFO("\tMem Data: %d\n", payload->mem_data_version);
	INFO("\tPin Data: %d\n", payload->pin_data_version);
	INFO("\tPWR Data: %d\n", payload->pwr_data_version);
	INFO("\tRST Data: %d\n", payload->rst_data_version);
	INFO("\tSecman Data: %d\n", payload->secman_data_version);
	INFO("\tSns Data: %d\n", payload->sns_data_version);
	INFO("\tSwpu Data: %d\n", payload->swpu_data_version);
}

int scmi_nxp_core_configure_core(void *p, uint32_t core_id, uintptr_t entrypoint)
{
	struct scmi_nxp_core_configure_core_a2p *payload_args;
	scmi_channel_t *ch = (scmi_channel_t *)p;
	mailbox_mem_t *mbx_mem;
	unsigned int token = 0;
	int ret;

	validate_scmi_channel(ch);

	scmi_get_channel(ch);

	mbx_mem = (mailbox_mem_t *)(ch->info->scmi_mbx_mem);
	mbx_mem->flags = SCMI_FLAG_RESP_POLL;
	mbx_mem->len = 4U + sizeof(struct scmi_nxp_core_configure_core_a2p);
	mbx_mem->msg_header = SCMI_MSG_CREATE(SCMI_PROTOCOL_ID_NXP_CORE,
					      SCMI_NXP_CORE_CONFIGURE_CORE,
					      token);

	payload_args = (struct scmi_nxp_core_configure_core_a2p *)mbx_mem->payload;
	payload_args->core_id = core_id;
	payload_args->low_entrypoint = entrypoint & ADDR_MASK;
	payload_args->high_entrypoint = entrypoint >> ADDR_SHIFT;

	scmi_send_sync_command(ch);

	/* Get the return values */
	SCMI_PAYLOAD_RET_VAL1(mbx_mem->payload, ret);
	assert(token == SCMI_MSG_GET_TOKEN(mbx_mem->msg_header));

	scmi_put_channel(ch);

	return ret;
}

int scmi_nxp_misc_display_firmware_info(void *p)
{
	struct scmi_nxp_misc_get_firmware_info_p2a *payload_resp;
	scmi_channel_t *ch = (scmi_channel_t *)p;
	mailbox_mem_t *mbx_mem;
	unsigned int token = 0;
	int ret;

	validate_scmi_channel(ch);
	scmi_get_channel(ch);

	mbx_mem = (mailbox_mem_t *)(ch->info->scmi_mbx_mem);
	mbx_mem->flags = SCMI_FLAG_RESP_POLL;
	mbx_mem->len = 4U;
	mbx_mem->msg_header = SCMI_MSG_CREATE(SCMI_PROTOCOL_ID_NXP_MISC,
					      SCMI_NXP_GET_FIRMWAREINFO, token);

	scmi_send_sync_command(ch);

	/* Get the return values */
	SCMI_PAYLOAD_RET_VAL1(mbx_mem->payload, ret);
	assert(token == SCMI_MSG_GET_TOKEN(mbx_mem->msg_header));

	if (ret != SCMI_SUCCESS) {
		ERROR("Failed to get FSS Firmware Info\n");
		goto free_scmi_channel;
	}

	payload_resp = (struct scmi_nxp_misc_get_firmware_info_p2a *)
		mbx_mem->payload;
	dump_fss_info(payload_resp);

free_scmi_channel:
	scmi_put_channel(ch);
	return ret;
}

int scmi_nxp_misc_display_data_version(void *p)
{
	struct scmi_nxp_misc_get_data_version_p2a *payload_resp;
	scmi_channel_t *ch = (scmi_channel_t *)p;
	mailbox_mem_t *mbx_mem;
	unsigned int token = 0;
	int ret;

	validate_scmi_channel(ch);
	scmi_get_channel(ch);

	mbx_mem = (mailbox_mem_t *)(ch->info->scmi_mbx_mem);
	mbx_mem->flags = SCMI_FLAG_RESP_POLL;
	mbx_mem->len = 4U;
	mbx_mem->msg_header = SCMI_MSG_CREATE(SCMI_PROTOCOL_ID_NXP_MISC,
					      SCMI_NXP_GET_DATA_VERSION, token);

	scmi_send_sync_command(ch);

	/* Get the return values */
	SCMI_PAYLOAD_RET_VAL1(mbx_mem->payload, ret);
	assert(token == SCMI_MSG_GET_TOKEN(mbx_mem->msg_header));

	if (ret != SCMI_SUCCESS) {
		ERROR("Failed to get FSS Misc Data version\n");
		goto free_scmi_channel;
	}

	payload_resp = (struct scmi_nxp_misc_get_data_version_p2a *)
		mbx_mem->payload;
	dump_data_version(payload_resp);

free_scmi_channel:
	scmi_put_channel(ch);
	return ret;
}
