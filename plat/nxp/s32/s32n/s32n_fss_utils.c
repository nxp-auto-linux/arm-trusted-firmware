/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <arm/css/scmi/scmi_private.h>
#include <drivers/nxp/s32n/s32n_scmi_nxp_core.h>
#include <drivers/nxp/s32n/s32n_scmi_nxp_misc.h>
#include <drivers/nxp/s32n/s32n_scmi_extensions.h>
#include <drivers/nxp/s32n/s32n_scmi_nxp_boot.h>
#include <drivers/scmi.h>
#include <s32n_fss_scmi.h>

#define ADDR_MASK		GENMASK(31, 0)
#define ADDR_SHIFT		U(32)

#define SCMI_NXP_CORE_PROTO_VER		MAKE_SCMI_VERSION(1, 0)

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

static void dump_plat_reset_reason(struct scmi_reset_plat_reason_p2a
		*payload)
{
	const char *rst_type = NULL;

	switch (payload->plat_reset_reason) {
	case POWER_ON_RESET:
		rst_type = "Power On Reset (PoR)";
		break;
	case DESTRUCTIVE_RESET:
		rst_type = "Destructive Reset";
		break;
	case DESTRUCTIVE_RESET_DURING_STANDBY:
		rst_type = "Destructive Reset During Standby";
		break;
	case FUNCTIONAL_RESET:
		rst_type = "Functional Reset";
		break;
	case FUNCTIONAL_RESET_DURING_STANDBY:
		rst_type = "Functional Reset During Standby";
		break;
	case STARTUP_SELFTEST_COMPLETE:
		rst_type = "Startup Selftest Complete";
		break;
	case WAKEUP_TO_RUN:
		rst_type = "Wakeup To Run";
		break;
	case WAKEUP_TO_HK:
		rst_type = "Wakeup to HK";
		break;
	default:
		rst_type = "Undefined";
		break;
	}
	NOTICE("SoC Reset Reason: %s\n", rst_type);
}

static void dump_cohort_reset_reason(struct scmi_nxp_boot_get_reset_reason_info_p2a
		*payload)
{
	uint32_t req_cohort_id, sw_entity_id, reset_type_id;
	const char *sw_entity = NULL, *reset_type = NULL;

	req_cohort_id = (payload->reset_reason & REQ_COHORT_ID_MASK) >>
		REQ_COHORT_ID_SHIFT;
	sw_entity_id = (payload->reset_reason & SW_ENTITY_MASK) >>
		SW_ENTITY_SHIFT;
	reset_type_id = payload->reset_reason & RESET_TYPE_MASK;

	switch (sw_entity_id) {
	case SWENTITY_USER_CID:
		sw_entity = "Another Cohort ID";
		break;
	case SWENTITY_BTM_INIT:
		sw_entity = "Initial Boot Manager";
		break;
	case SWENTITY_SAFETY_MANAGER:
		sw_entity = "Safety Manager";
		break;
	case SWENTITY_SECURITY_MANAGER:
		sw_entity = "Security Manager";
		break;
	default:
		sw_entity = "N/A";
		break;
	}

	switch (reset_type_id) {
	case RESET_TYPE_UNKNOWN:
		reset_type = "Unknown";
		break;
	case RESET_TYPE_BOOT:
		reset_type = "Boot Cohort";
		break;
	case RESET_TYPE_COLDRESET:
		reset_type = "Cold Reset";
		break;
	case RESET_TYPE_WARMRESET:
		reset_type = "Warm Reset";
		break;
	case RESET_TYPE_WAKEUP:
		reset_type = "Wake-up Reset";
		break;
	case RESET_TYPE_WARMRESET_ESCALATED:
		reset_type = "Cold Reset (escalated from Warm Reset)";
		break;
	case RESET_TYPE_WAKEUP_ESCALATED:
		reset_type = "Cold Reset (escalated from Wake-up)";
		break;
	default:
		reset_type = "Invalid";
		break;
	}

	NOTICE("\tRequesting Cohort ID: %d\n", req_cohort_id);
	NOTICE("\tRequesting SW Entity: %s\n", sw_entity);
	NOTICE("\tReset Type: %s\n", reset_type);
}

static void display_soc_num_cores(uint32_t attributes)
{
	INFO("S32N: SoC core count: %u\n", attributes & SCMI_NXP_CORE_SOC_CORE_COUNT_MASK);
}

static int scmi_nxp_core_proto_attr(void *p, uint32_t *attributes)
{
	scmi_channel_t *ch = (scmi_channel_t *)p;
	mailbox_mem_t *mbx_mem;
	unsigned int token = 0;
	int ret;

	validate_scmi_channel(ch);

	scmi_get_channel(ch);

	mbx_mem = (mailbox_mem_t *)(ch->info->scmi_mbx_mem);
	mbx_mem->msg_header = SCMI_MSG_CREATE(SCMI_PROTOCOL_ID_NXP_CORE, SCMI_PROTO_ATTR_MSG,
							token);
	mbx_mem->len = SCMI_NXP_CORE_PROTO_ATTR_MSG_LEN;
	mbx_mem->flags = SCMI_FLAG_RESP_POLL;

	scmi_send_sync_command(ch);

	/* Get the return values */
	SCMI_PAYLOAD_RET_VAL2(mbx_mem->payload, ret, *attributes);
	assert(mbx_mem->len == SCMI_NXP_CORE_PROTO_ATTR_RESP_LEN);
	assert(token == SCMI_MSG_GET_TOKEN(mbx_mem->msg_header));

	scmi_put_channel(ch);

	return ret;
}

int scmi_nxp_core_init(void *p)
{
	uint32_t version = 0, attr = 0;
	int ret;

	ret = scmi_proto_version(p, SCMI_PROTOCOL_ID_NXP_CORE, &version);
	if (ret != SCMI_E_SUCCESS) {
		WARN("SCMI NXP Core protocol version message failed\n");
		return ret;
	}

	if (!is_scmi_version_compatible(SCMI_NXP_CORE_PROTO_VER, version)) {
		WARN("SCMI NXP Core protocol version 0x%x incompatible with driver version 0x%x\n",
			version, SCMI_NXP_CORE_PROTO_VER);
		return ret;
	}

	INFO("SCMI NXP Core protocol version 0x%x detected\n", version);

	ret = scmi_nxp_core_proto_attr(p, &attr);
	if (ret != SCMI_E_SUCCESS) {
		ERROR("SCMI NXP Core protocol attributes command failed\n");
		return ret;
	}

	display_soc_num_cores(attr);

	/* Check that NXP CORE Configure Core message is supported */
	ret = scmi_proto_msg_attr(p, SCMI_PROTOCOL_ID_NXP_CORE,
				SCMI_NXP_CORE_CONFIGURE_CORE, &attr);
	if (ret != SCMI_E_SUCCESS)
		ERROR("SCMI NXP Core Configure Core command is not supported.\n");

	return ret;
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

int scmi_nxp_misc_get_gpo_signal(void *p, uint8_t *gic_mode)
{
	struct scmi_nxp_misc_gpo_signal_a2p *payload_args;
	struct scmi_nxp_misc_gpo_signal_p2a *payload_resp;
	scmi_channel_t *ch = (scmi_channel_t *)p;
	unsigned int token = 0;
	mailbox_mem_t *mbx_mem;
	int ret;

	validate_scmi_channel(ch);
	scmi_get_channel(ch);

	mbx_mem = (mailbox_mem_t *)(ch->info->scmi_mbx_mem);
	mbx_mem->flags = SCMI_FLAG_RESP_POLL;
	mbx_mem->len = 4U + sizeof(struct scmi_nxp_misc_gpo_signal_a2p);
	mbx_mem->msg_header = SCMI_MSG_CREATE(SCMI_PROTOCOL_ID_NXP_MISC,
			SCMI_NXP_GET_GPO_SIGNAL, token);

	/* Currently the A2P data are ignored.
	 * They are only needed for a length check.
	 */
	payload_args = (struct scmi_nxp_misc_gpo_signal_a2p *)
		mbx_mem->payload;
	payload_args->subsystem = 0;
	payload_args->gpo_signal = 0;

	scmi_send_sync_command(ch);

	/* Get the return values */
	SCMI_PAYLOAD_RET_VAL1(mbx_mem->payload, ret);
	assert(token == SCMI_MSG_GET_TOKEN(mbx_mem->msg_header));

	if (ret != SCMI_SUCCESS) {
		ERROR("Failed to get GIC Mode, err: %d\n", ret);
		goto free_scmi_channel;
	}

	payload_resp = (struct scmi_nxp_misc_gpo_signal_p2a *)
		mbx_mem->payload;
	*gic_mode = (uint8_t)payload_resp->gic_mode;

free_scmi_channel:
	scmi_put_channel(ch);
	return ret;
}

int scmi_reset_display_plat_reason(void *p)
{
	struct scmi_reset_plat_reason_p2a *payload_resp;
	scmi_channel_t *ch = (scmi_channel_t *)p;
	mailbox_mem_t *mbx_mem;
	unsigned int token = 0;
	int ret;

	validate_scmi_channel(ch);
	scmi_get_channel(ch);

	mbx_mem = (mailbox_mem_t *)(ch->info->scmi_mbx_mem);
	mbx_mem->flags = SCMI_FLAG_RESP_POLL;
	mbx_mem->len = 4U;
	mbx_mem->msg_header = SCMI_MSG_CREATE(SCMI_PROTOCOL_ID_RESET_DOMAIN,
			SCMI_RESET_GET_PLATFORM_REASON, token);

	scmi_send_sync_command(ch);

	/* Get the return values */
	SCMI_PAYLOAD_RET_VAL1(mbx_mem->payload, ret);
	assert(token == SCMI_MSG_GET_TOKEN(mbx_mem->msg_header));

	if (ret != SCMI_SUCCESS) {
		ERROR("Failed to get FSS Platform Reset cause, err: %d\n", ret);
		goto free_scmi_channel;
	}

	payload_resp = (struct scmi_reset_plat_reason_p2a *)
		mbx_mem->payload;
	dump_plat_reset_reason(payload_resp);

free_scmi_channel:
	scmi_put_channel(ch);
	return ret;
}

int scmi_nxp_boot_display_reset_reason(void *p, uint32_t cohort_id)
{
	struct scmi_nxp_boot_get_reset_reason_info_a2p *payload_args;
	struct scmi_nxp_boot_get_reset_reason_info_p2a *payload_resp;
	scmi_channel_t *ch = (scmi_channel_t *)p;
	unsigned int token = 0;
	mailbox_mem_t *mbx_mem;
	int ret;

	validate_scmi_channel(ch);
	scmi_get_channel(ch);

	mbx_mem = (mailbox_mem_t *)(ch->info->scmi_mbx_mem);
	mbx_mem->flags = SCMI_FLAG_RESP_POLL;
	mbx_mem->len = 4U + sizeof(struct scmi_nxp_boot_get_reset_reason_info_a2p);
	mbx_mem->msg_header = SCMI_MSG_CREATE(SCMI_PROTOCOL_ID_NXP_BOOT,
			SCMI_NXP_GET_RESET_REASON_INFO, token);

	payload_args = (struct scmi_nxp_boot_get_reset_reason_info_a2p *)
		mbx_mem->payload;
	payload_args->cohort_id = cohort_id;

	scmi_send_sync_command(ch);

	/* Get the return values */
	SCMI_PAYLOAD_RET_VAL1(mbx_mem->payload, ret);
	assert(token == SCMI_MSG_GET_TOKEN(mbx_mem->msg_header));

	if (ret != SCMI_SUCCESS) {
		ERROR("Failed to get Cohort Reset Reason, err: %d\n", ret);
		goto free_scmi_channel;
	}

	NOTICE("Reset Info for Cohort %d:\n", cohort_id);
	payload_resp = (struct scmi_nxp_boot_get_reset_reason_info_p2a *)
		mbx_mem->payload;
	dump_cohort_reset_reason(payload_resp);

free_scmi_channel:
	scmi_put_channel(ch);
	return ret;
}
