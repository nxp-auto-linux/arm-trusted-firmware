/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <errno.h>
#include <arch_helpers.h>
#include <arm/css/scmi/scmi_private.h>
#include <common/debug.h>
#include <lib/utils_def.h>
#include <platform.h>
#include <s32_scmi.h>
#include <s32n_fconf_scmi_getter.h>
#include <s32n_scmi_local.h>
#include <s32n_scmi_pinctrl.h>

/*
 * Table indexed by the SCMI protocol ID. Only the protocols that can be served
 * locally have a populated entry pointing at their per-message ops table; every
 * other index is left zeroed, which means "no local handling".
 */
static const struct s32n_scmi_msg_table *const s32n_scmi_local_protos[] = {
	[S32N_SCMI_PROTOCOL_ID_PINCTRL] = &s32n_scmi_pinctrl_msg_table,
};

static const struct s32n_scmi_msg_ops *find_ops(uint32_t msg_header)
{
	uint32_t proto_id = SCMI_MSG_GET_PROTO(msg_header);
	uint32_t msg_id = SCMI_MSG_GET_MSG_ID(msg_header);
	const struct s32n_scmi_msg_table *table;

	if (proto_id >= ARRAY_SIZE(s32n_scmi_local_protos)) {
		return NULL;
	}

	table = s32n_scmi_local_protos[proto_id];
	if (table == NULL || msg_id >= table->nr_ops) {
		return NULL;
	}

	return &table->ops[msg_id];
}

bool s32n_scmi_local_handle_request(struct scmi_shared_mem *mem,
				    size_t mem_size)
{
	uint32_t msg_header = mem->msg_header;
	const struct s32n_scmi_msg_ops *ops = find_ops(msg_header);

	if (ops == NULL || ops->handle_request == NULL) {
		return false;
	}

	return ops->handle_request(mem, mem_size);
}

void s32n_scmi_local_snoop_response(const struct scmi_shared_mem *mem,
				    size_t mem_size)
{
	uint32_t msg_header = mem->msg_header;
	const struct s32n_scmi_msg_ops *ops = find_ops(msg_header);

	if (ops == NULL || ops->snoop_response == NULL) {
		return;
	}

	ops->snoop_response(mem, mem_size);
}

static uintptr_t get_ospm_notif_mem(void)
{
	return FCONF_GET_PROPERTY(s32n, scmi_ospm_ch_config, notif_mem_base);
}

static size_t get_ospm_notif_mem_size(void)
{
	return FCONF_GET_PROPERTY(s32n, scmi_ospm_ch_config, notif_mem_size);
}

static uint32_t get_ospm_notif_irq(void)
{
	return FCONF_GET_PROPERTY(s32n, scmi_ospm_ch_config, notif_irq);
}

bool is_ospm_notif_enabled(void)
{
	return get_ospm_notif_mem() != 0U && get_ospm_notif_irq() != 0U;
}

/*
 * Relay an SCMI notification received from FSS up to the OS: copy the message
 * verbatim into the OS-facing buffer and pend the OS doorbell. FSS owns the
 * payload, so the relay is payload-agnostic and serves every protocol.
 */
int s32n_scmi_relay_notif_to_ospm(mailbox_mem_t *mb)
{
	uintptr_t notif_mem = get_ospm_notif_mem();
	uint32_t notif_irq = get_ospm_notif_irq();
	int ret;

	if (!is_ospm_notif_enabled()) {
		WARN("No OS-facing SCMI notification channel. Notification dropped.\n");
		return -ENODEV;
	}

	ret = copy_scmi_msg(notif_mem, (uintptr_t)mb, get_ospm_notif_mem_size());
	if (ret) {
		ERROR("SCMI notification does not fit the OS buffer: %d\n", ret);
		return ret;
	}

	/*
	 * Ensure the message is visible to the OS before the doorbell is
	 * pended.
	 */
	dmbst();

	plat_ic_set_interrupt_pending(notif_irq);

	return 0;
}
