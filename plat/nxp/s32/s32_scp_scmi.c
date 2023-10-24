/*
 * Copyright 2022-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <libc/assert.h>
#include <common/debug.h>
#include <common/fdt_wrappers.h>
#include <drivers/arm/css/scmi.h>
#include <arm/css/scmi/scmi_logger.h>
#include <arm/css/scmi/scmi_private.h>
#include <lib/mmio.h>
#include <platform.h>
#include <libc/errno.h>
#include <libfdt.h>
#include <drivers/scmi.h>
#include <inttypes.h>
#include <s32_bl_common.h>
#include <dt-bindings/power/s32gen1-scmi-pd.h>
#include <s32_interrupt_mgmt.h>
#include <plat/common/platform.h>
#include <s32_dt.h>
#include <s32_scp_scmi.h>

/* M7 Core0, Interrupt 0 will be used to send SCMI messages */
#define TX_CPN		(4u)
#define MSCM_TX_IRQ	(0u)

/* A53 Core0, Interrupt 0 will be used to send SCMI messages */
#define RX_CPN		(0u)
#define MSCM_RX_IRQ	(0u)

#define SCMI_GPIO_ACK_IRQ	(0xFFu)
#define MAX_INTERNAL_MSGS	(1)
#define SCP_GPIO_GIC_NOTIF_IRQ	(332)

#define IRQ_CELL_SIZE (3)

typedef struct scp_mem {
	uintptr_t base;
	size_t size;
} scp_mem_t;

typedef struct scp_irq {
	uint32_t cpn;
	uint32_t mscm_irq;
	int irq_num;
} scp_irq_t;

struct scmi_scp_dt_info {
	scp_mem_t tx_mbs[PLATFORM_CORE_COUNT];
	scp_mem_t tx_mds[PLATFORM_CORE_COUNT];
	scp_irq_t tx_irq;
	scp_mem_t rx_mb;
	scp_mem_t rx_md;
	scp_irq_t rx_irq;
	scp_mem_t ospm_notif_mem;
	int ospm_notif_irq;
};

static struct scmi_scp_dt_info scp_dt;

enum irpc_reg_type {
	IRPC_ISR,
	IRPC_IGR,
};

struct scmi_intern_msg {
	uint32_t proto;
	uint32_t msg_id;
	scmi_msg_callback_t cb;
};

static struct scmi_intern_msg intern_msgs[MAX_INTERNAL_MSGS];
static size_t used_intern_msgs;

static scmi_channel_t scmi_channels[PLATFORM_CORE_COUNT];
static scmi_channel_plat_info_t s32_scmi_plat_info[PLATFORM_CORE_COUNT];
static void *scmi_handles[PLATFORM_CORE_COUNT];
DEFINE_BAKERY_LOCK(s32_scmi_locks[PLATFORM_CORE_COUNT]);

static uintptr_t get_irpc_reg_addr(uintptr_t base, uint32_t cpn, uint32_t irq,
				   enum irpc_reg_type reg_type)
{
	uintptr_t offset = base;

	assert(cpn <= MSCM_MAX_CPN);
	assert(irq <= MSCM_MAX_C2C_IRQ);

	switch (reg_type) {
	case IRPC_IGR:
		assert(!check_uptr_overflow(offset, 0x4u));
		offset += 0x4u;
		/* Fallthrough */
	case IRPC_ISR:
		assert(!check_uptr_overflow(offset, MSCM_IRPC_OFFSET));
		offset += MSCM_IRPC_OFFSET;
		assert(!check_uptr_overflow(offset, cpn * MSCM_CPN_SIZE));
		offset += cpn * MSCM_CPN_SIZE;
		assert(!check_uptr_overflow(offset, irq * 0x8));
		offset += irq * 0x8;
		return offset;
	default:
		return offset;
	}
}

void mscm_ring_doorbell(struct scmi_channel_plat_info *plat_info)
{
	uintptr_t reg = get_irpc_reg_addr(plat_info->db_reg_addr, TX_CPN,
					  MSCM_TX_IRQ, IRPC_IGR);

	mmio_write_32(reg, 1);
}

static bool is_scmi_logger_enabled(void)
{
	return SCMI_LOGGER;
}

static uintptr_t get_mb_addr(uint32_t core)
{
	return S32_SCP_SCMI_MEM + core * S32_SCP_CH_MEM_SIZE;
}

static uintptr_t get_md_addr(uint32_t core)
{
	if (!is_scmi_logger_enabled())
		return 0;

	return S32_SCP_SCMI_META_MEM + core * S32_SCP_CH_META_SIZE;
}

/* RX mailbox is placed right after tx mailboxes */
static uintptr_t get_rx_mb_addr(void)
{
	return get_mb_addr(PLATFORM_CORE_COUNT);
}

uintptr_t get_rx_md_addr(void)
{
	return get_md_addr(PLATFORM_CORE_COUNT);
}

static size_t get_packet_size(uintptr_t scmi_packet)
{
	mailbox_mem_t *mbx_mem = (mailbox_mem_t *)scmi_packet;

	return offsetof(mailbox_mem_t, msg_header) + mbx_mem->len;
}

static int scmi_gpio_eirq_ack(void *payload)
{
	uintptr_t mb_addr = get_rx_mb_addr();
	mailbox_mem_t *mb = (mailbox_mem_t *)mb_addr;

	if (is_scmi_logger_enabled())
		log_scmi_ack(mb, get_rx_md_addr());

	/* Nothing to perform other than marking the channel as free */
	SCMI_MARK_CHANNEL_FREE(mb->status);

	return 0;
}

static void process_gpio_notification(mailbox_mem_t *mb)
{
	uintptr_t mb_addr = (uintptr_t)mb;
	size_t msg_size;

	msg_size = get_packet_size(mb_addr);
	if (msg_size > S32_SCP_CH_MEM_SIZE)
		return;

	memcpy((void *)S32_OSPM_SCMI_NOTIF_MEM, mb, msg_size);

	plat_ic_set_interrupt_pending(SCP_GPIO_GIC_NOTIF_IRQ);
}

static uint64_t mscm_interrupt_handler(uint32_t id, uint32_t flags,
				       void *handle, void *cookie)
{
	uintptr_t mb_addr = get_rx_mb_addr();
	mailbox_mem_t *mb = (mailbox_mem_t *)mb_addr;
	uint32_t proto;

	assert(!SCMI_IS_CHANNEL_FREE(mb->status));
	assert(get_packet_size(mb_addr) <= S32_SCP_CH_MEM_SIZE);

	if (is_scmi_logger_enabled())
		log_scmi_notif(mb, get_rx_md_addr());

	proto = SCMI_MSG_GET_PROTO(mb->msg_header);

	if (proto == SCMI_PROTOCOL_ID_GPIO) {
		process_gpio_notification(mb);
	}

	return 0;
}

static int scp_get_mb(void *fdt, int node, const char *name,
				const fdt32_t *phandles, scp_mem_t *mb)
{
	int mb_node, ret = 0, idx;

	idx = fdt_stringlist_search(fdt, node, "nxp,scp-mbox-names", name);
	if (idx < 0) {
		ERROR("Failed to get SCMI %s mailbox index.\n", name);
		return -FDT_ERR_NOTFOUND;
	}

	mb_node = fdt_node_offset_by_phandle(fdt, fdt32_to_cpu(phandles[idx]));
	if (mb_node < 0) {
		ERROR("Failed to get SCMI %s mailbox node.\n", name);
		return -FDT_ERR_NOTFOUND;
	}

	if (fdt_get_status(mb_node) == DT_ENABLED) {
		/* Get value and size of "reg" property */
		ret = fdt_get_reg_props_by_index(fdt, mb_node, 0, &mb->base, &mb->size);
		if (ret) {
			ERROR("Couldn't get 'reg' property values of %s node\n", name);
			return ret;
		}
	}

	return ret;
}

static int scp_get_ospm_notif_mb(void *fdt, int node, const fdt32_t *mboxes)
{
	return scp_get_mb(fdt, node, "scmi_ospm_notif", mboxes, &scp_dt.ospm_notif_mem);
}

static int scp_get_rx_md(void *fdt, int node, const fdt32_t *mboxes)
{
	return scp_get_mb(fdt, node, "scp_rx_md", mboxes, &scp_dt.rx_md);
}

static int scp_get_tx_md(void *fdt, int node, const fdt32_t *mboxes, uint8_t core)
{
	char tx_md_name[] = "scp_tx_md0";
	unsigned int len = sizeof(tx_md_name) - 1;

	if (core >= PLATFORM_CORE_COUNT || !len)
		return -EINVAL;

	tx_md_name[len - 1] += core;
	return scp_get_mb(fdt, node, tx_md_name, mboxes, &scp_dt.tx_mds[core]);
}

static int scp_get_rx_mb(void *fdt, int node, const fdt32_t *mboxes)
{
	return scp_get_mb(fdt, node, "scp_rx_mb", mboxes, &scp_dt.rx_mb);
}

static int scp_get_tx_mb(void *fdt, int node, const fdt32_t *mboxes, uint8_t core)
{
	char tx_mb_name[] = "scp_tx_mb0";
	unsigned int len = sizeof(tx_mb_name) - 1;

	if (core >= PLATFORM_CORE_COUNT || !len)
		return -EINVAL;

	tx_mb_name[len - 1] += core;
	return scp_get_mb(fdt, node, tx_mb_name, mboxes, &scp_dt.tx_mbs[core]);
}

static int scp_get_mboxes_from_fdt(void *fdt, int node, bool init_rx)
{
	const fdt32_t *mboxes;
	int ret = 0, i;

	mboxes = fdt_getprop(fdt, node, "nxp,scp-mboxes", NULL);
	if (!mboxes) {
		ERROR("Failed to get \"nxp,scp-mboxes\" property\n");
		return -FDT_ERR_NOTFOUND;
	}

	for (i = 0; i < PLATFORM_CORE_COUNT; i++) {
		ret = scp_get_tx_mb(fdt, node, mboxes, i);
		if (ret)
			return ret;
	}

	if (init_rx) {
		ret = scp_get_rx_mb(fdt, node, mboxes);
		if (ret)
			return ret;

		ret = scp_get_ospm_notif_mb(fdt, node, mboxes);
		if (ret)
			return ret;
	}

	if (is_scmi_logger_enabled()) {
		/* Get metadata memory zones */
		for (i = 0; i < PLATFORM_CORE_COUNT; i++) {
			ret = scp_get_tx_md(fdt, node, mboxes, i);
			if (ret)
				return ret;
		}

		if (init_rx)
			ret = scp_get_rx_md(fdt, node, mboxes);
	}

	return ret;
}

static int scp_get_irq(void *fdt, int node, const char *prop_name, const char *string,
			const fdt32_t *irqs, scp_irq_t *irq)
{
	int irq_node, ret, idx;

	idx = fdt_stringlist_search(fdt, node, prop_name, string);
	if (idx < 0) {
		ERROR("Failed to get SCMI %s irq index.\n", string);
		return -FDT_ERR_NOTFOUND;
	}

	irq_node = fdt_node_offset_by_phandle(fdt, fdt32_to_cpu(irqs[IRQ_CELL_SIZE * idx]));
	if (irq_node < 0) {
		ERROR("Failed to get SCMI %s irq node.\n", string);
		return -FDT_ERR_NOTFOUND;
	}

	irq->cpn = fdt32_to_cpu(irqs[IRQ_CELL_SIZE * idx + 1]);
	irq->mscm_irq = fdt32_to_cpu(irqs[IRQ_CELL_SIZE * idx + 2]);

	ret = fdt_get_irq_props_by_index(fdt, irq_node, IRQ_CELL_SIZE,
				irq->mscm_irq, &irq->irq_num);
	if (ret) {
		ERROR("Failed to get %s irq number.\n", string);
		return ret;
	}

	return ret;
}

static int scp_get_irqs_from_fdt(void *fdt, int node, struct scmi_scp_dt_info *scp, bool init_rx)
{
	const fdt32_t *irqs, *ospm_irq;
	int ret, len = 0;

	irqs = fdt_getprop(fdt, node, "nxp,scp-irqs", &len);
	if (!irqs) {
		ERROR("Failed to get \"nxp,scp-irqs\" property.\n");
		return -FDT_ERR_NOTFOUND;
	}

	if (len != IRQ_CELL_SIZE * 2 * (int)sizeof(uint32_t)) {
		ERROR("Invalid \"nxp,scp-irqs\" property.\n");
		return -EIO;
	}

	ret = scp_get_irq(fdt, node, "nxp,scp-irq-names", "scp_tx", irqs, &scp->tx_irq);
	if (ret)
		return ret;

	if (init_rx) {
		ret = scp_get_irq(fdt, node, "nxp,scp-irq-names", "scp_rx", irqs, &scp->rx_irq);
		if (ret)
			return ret;

		ospm_irq = fdt_getprop(fdt, node, "nxp,notif-irq", &len);
		if (!ospm_irq) {
			ERROR("Failed to get \"nxp,notif-irq\" property.\n");
			return -FDT_ERR_NOTFOUND;
		}

		if (len != IRQ_CELL_SIZE * (int)sizeof(uint32_t)) {
			ERROR("Invalid \"nxp,notif-irq\" property.\n");
			return -EIO;
		}

		scp->ospm_notif_irq = fdt_read_irq_cells(ospm_irq, IRQ_CELL_SIZE);
		if (scp->ospm_notif_irq < 0) {
			ERROR("Failed to get \"nxp,notif-irq\" IRQ.\n");
			return -FDT_ERR_BADVALUE;
		}
	}

	return ret;
}

int scp_scmi_dt_init(bool init_rx)
{
	void *fdt = NULL;
	int scmi_node;
	int ret = 0;

	if (dt_open_and_check() < 0)
		return -EINVAL;

	if (fdt_get_address(&fdt) == 0)
		return -EINVAL;

	scmi_node = fdt_node_offset_by_compatible(fdt, -1, "arm,scmi-smc");
	if (scmi_node == -FDT_ERR_NOTFOUND)
		return -ENODEV;

	ret = scp_get_mboxes_from_fdt(fdt, scmi_node, init_rx);
	if (ret) {
		ERROR("Could not initialize SCP mailboxes from device tree.\n");
		return ret;
	}

	ret = scp_get_irqs_from_fdt(fdt, scmi_node, &scp_dt, init_rx);
	if (ret)
		ERROR("Could not initialize SCP irqs from device tree.\n");

	return ret;
}

void scp_scmi_init(bool request_irq)
{
	size_t i;
	int ret;

	assert(ARRAY_SIZE(scmi_channels) == ARRAY_SIZE(s32_scmi_locks));
	assert(ARRAY_SIZE(scmi_channels) == ARRAY_SIZE(s32_scmi_plat_info));

	for (i = 0u; i < ARRAY_SIZE(scmi_channels); i++) {
		s32_scmi_plat_info[i] = (scmi_channel_plat_info_t) {
			.scmi_mbx_mem = get_mb_addr(i),
			.scmi_mbx_size = S32_SCP_CH_MEM_SIZE,
			.scmi_md_mem = get_md_addr(i),
			.db_reg_addr = MSCM_BASE_ADDR,
			.db_preserve_mask = 0xfffffffe,
			.db_modify_mask = 0x1,
			.ring_doorbell = &mscm_ring_doorbell,
		};

		scmi_channels[i] = (scmi_channel_t) {
			.info = &s32_scmi_plat_info[i],
			.lock = &s32_scmi_locks[i],
		};
	}

	if (is_scmi_logger_enabled())
		log_scmi_init();

	if (!request_irq)
		return;

	ret = request_intr_type_el3(S32CC_MSCM_CORE_0_IRQ,
				    mscm_interrupt_handler);
	if (ret) {
		ERROR("Failed to request MSCM interrupt\n");
		panic();
	}

	ret = register_scmi_internal_msg_handler(SCMI_PROTOCOL_ID_GPIO,
						 SCMI_GPIO_ACK_IRQ,
						 scmi_gpio_eirq_ack);
	if (ret)
		panic();
}

static scmi_channel_t *get_scmi_channel(unsigned int *ch_id)
{
	int core = plat_core_pos_by_mpidr(read_mpidr());
	scmi_channel_t *ch;

	if (core < 0 || core >= (ssize_t)ARRAY_SIZE(scmi_channels)) {
		ERROR("Failed to get SCMI channel for core %d\n",
		      core);
		return NULL;
	}

	ch = &scmi_channels[core];
	if (!ch->is_initialized) {
		scmi_handles[core] = scmi_init(ch);
		if (scmi_handles[core] == NULL) {
			ERROR("Failed to initialize SCMI channel for core %d\n",
			      core);
			return NULL;
		}
	}

	if (ch_id)
		*ch_id = core;

	return ch;
}

static void *get_scmi_handle(void)
{
	unsigned int ch_id;
	scmi_channel_t *ch = get_scmi_channel(&ch_id);

	if (!ch)
		return NULL;

	return scmi_handles[ch_id];
}

static int copy_scmi_msg(uintptr_t to, uintptr_t from, size_t to_size)
{
	size_t copy_len;

	copy_len = get_packet_size(from);
	if (copy_len > to_size)
		return -E2BIG;

	memcpy((void *)to, (const void *)from, copy_len);

	return 0;
}

void scp_set_core_reset_addr(uintptr_t addr)
{
	int ret;
	void *handle = get_scmi_handle();

	if (!handle) {
		ERROR("%s: Failed to get SCMI handle", __func__);
		panic();
	}

	ret = scmi_ap_core_set_reset_addr(handle, addr, 0x0u);
	if (ret) {
		ERROR("Failed to set core reset address\n");
		panic();
	}
}

static int scp_cpu_set_state(uint32_t core, uint32_t state)
{
	void *handle = get_scmi_handle();
	uint32_t domain_id = S32GEN1_SCMI_PD_A53(core);
	uint32_t pwr_state;
	int ret;

	if (!handle) {
		ERROR("%s: Failed to get SCMI handle", __func__);
		panic();
	}

	pwr_state = S32GEN1_SCMI_PD_SET_LEVEL0_STATE(state);
	pwr_state |= S32GEN1_SCMI_PD_SET_LEVEL1_STATE(S32GEN1_SCMI_PD_ON);
	pwr_state |= S32GEN1_SCMI_PD_SET_MAX_LEVEL_STATE(S32GEN1_SCMI_PD_ON);

	ret = scmi_pwr_state_set(handle, domain_id, pwr_state);

	if (ret != SCMI_E_QUEUED && ret != SCMI_E_SUCCESS) {
		ERROR("Failed to set core%" PRIu32 " power state to '%" PRIu32 "'",
		      core, state);
		return -EINVAL;
	}

	if (ret == SCMI_E_SUCCESS)
		return 0;

	return -EINVAL;
}

int scp_cpu_on(uint32_t core)
{
	return scp_cpu_set_state(core, S32GEN1_SCMI_PD_ON);
}

int scp_cpu_off(uint32_t core)
{
	int ret;

	ret = scp_cpu_set_state(core, S32GEN1_SCMI_PD_OFF);
	if (ret)
		return ret;

	core_turn_off();
}

int scp_get_cpu_state(uint32_t core)
{
	void *handle = get_scmi_handle();
	uint32_t pwr_state = 0u;
	uint32_t domain_id = S32GEN1_SCMI_PD_A53(core);
	int ret;

	if (!handle) {
		ERROR("%s: Failed to get SCMI handle", __func__);
		panic();
	}

	ret = scmi_pwr_state_get(handle, domain_id, &pwr_state);
	if (ret != SCMI_E_SUCCESS) {
		ERROR("Failed to get core%" PRIu32 " power state\n", core);
		return -EINVAL;
	}

	return S32GEN1_SCMI_PD_GET_LEVEL0_STATE(pwr_state);
}

void scp_suspend_platform(void)
{
	int ret;
	void *handle = get_scmi_handle();

	if (!handle) {
		ERROR("%s: Failed to get SCMI handle", __func__);
		panic();
	}

	ret = scmi_sys_pwr_state_set(handle, SCMI_SYS_PWR_FORCEFUL_REQ,
				     SCMI_SYS_PWR_SUSPEND);
	if (ret != SCMI_E_SUCCESS) {
		ERROR("Failed to transition the system to suspend state\n");
		panic();
	}

	core_turn_off();
}

static void __dead2 scp_set_sys_pwr_state(uint32_t state)
{
	int ret;
	void *handle = get_scmi_handle();

	if (!handle) {
		ERROR("%s: Failed to get SCMI handle", __func__);
		panic();
	}

	ret = scmi_sys_pwr_state_set(handle, SCMI_SYS_PWR_FORCEFUL_REQ, state);
	if (ret != SCMI_E_SUCCESS) {
		ERROR("Failed to change power state of the system\n");
		panic();
	}

	ERROR("Could not handle state change: %" PRIu32, ret);
	panic();
}

void __dead2 scp_shutdown_platform(void)
{
	scp_set_sys_pwr_state(SCMI_SYS_PWR_SHUTDOWN);
}

void __dead2 scp_reset_platform(void)
{
	scp_set_sys_pwr_state(SCMI_SYS_PWR_COLD_RESET);
}

static bool is_proto_allowed(mailbox_mem_t *mbx_mem)
{
	uint32_t proto = SCMI_MSG_GET_PROTO(mbx_mem->msg_header);

	switch (proto) {
	case SCMI_PROTOCOL_ID_BASE:
	case SCMI_PROTOCOL_ID_PERF:
	case SCMI_PROTOCOL_ID_CLOCK:
	case SCMI_PROTOCOL_ID_RESET_DOMAIN:
	case SCMI_PROTOCOL_ID_PINCTRL:
	case SCMI_PROTOCOL_ID_GPIO:
	case SCMI_PROTOCOL_ID_NVMEM:
		return true;
	}

	return false;
}

int register_scmi_internal_msg_handler(uint32_t protocol, uint32_t msg_id,
				       scmi_msg_callback_t callback)
{
	if (used_intern_msgs >= ARRAY_SIZE(intern_msgs))
		return -ENOMEM;

	intern_msgs[used_intern_msgs] = (struct scmi_intern_msg) {
		.proto = protocol,
		.msg_id = msg_id,
		.cb = callback,
	};
	used_intern_msgs++;

	return 0;
}

static struct scmi_intern_msg *get_internal_msg(uint32_t protocol,
						uint32_t msg_id)
{
	struct scmi_intern_msg *msg;
	size_t i;

	for (i = 0u; i < used_intern_msgs; i++) {
		msg = &intern_msgs[i];

		if (msg->proto == protocol && msg->msg_id == msg_id)
			return msg;
	}

	return NULL;
}

static bool is_internal_msg(mailbox_mem_t *mbx_mem)
{
	uint32_t proto = SCMI_MSG_GET_PROTO(mbx_mem->msg_header);
	uint32_t msg_id = SCMI_MSG_GET_MSG_ID(mbx_mem->msg_header);

	return (get_internal_msg(proto, msg_id) != NULL);
}

static int handle_internal_msg(uintptr_t scmi_mem)
{
	mailbox_mem_t *mbx_mem = (mailbox_mem_t *)scmi_mem;
	uint32_t proto = SCMI_MSG_GET_PROTO(mbx_mem->msg_header);
	uint32_t msg_id = SCMI_MSG_GET_MSG_ID(mbx_mem->msg_header);
	struct scmi_intern_msg *msg = get_internal_msg(proto, msg_id);
	int ret;

	if (!msg)
		return SCMI_GENERIC_ERROR;

	ret = msg->cb(&mbx_mem->payload[0]);
	if (ret)
		return SCMI_GENERIC_ERROR;

	SCMI_MARK_CHANNEL_FREE(mbx_mem->status);
	return SCMI_SUCCESS;
}

static int forward_to_scp(uintptr_t scmi_mem, size_t scmi_mem_size)
{
	unsigned int ch_id;
	scmi_channel_plat_info_t *ch_info;
	scmi_channel_t *ch = get_scmi_channel(&ch_id);
	mailbox_mem_t *mbx_mem;
	size_t packet_size;
	int ret;

	if (!ch)
		return SCMI_GENERIC_ERROR;

	ch_info = ch->info;
	mbx_mem = (mailbox_mem_t *)(ch_info->scmi_mbx_mem);

	while (!SCMI_IS_CHANNEL_FREE(mbx_mem->status))
		;

	packet_size = get_packet_size(scmi_mem);
	assert(!check_uptr_overflow(ch_info->scmi_mbx_mem, packet_size));

	/* Transfer request into SRAM mailbox */
	if (ch_info->scmi_mbx_mem + packet_size >
	    S32_SCP_SCMI_MEM + S32_SCP_SCMI_MEM_SIZE)
		return SCMI_OUT_OF_RANGE;

	ret = copy_scmi_msg((uintptr_t)mbx_mem, scmi_mem,
			    S32_SCP_CH_MEM_SIZE);
	if (ret)
		return SCMI_OUT_OF_RANGE;

	SCMI_MARK_CHANNEL_FREE(mbx_mem->status);

	/*
	 * All commands must complete with a poll, not
	 * an interrupt, even if the agent requested this.
	 */
	mbx_mem->flags = SCMI_FLAG_RESP_POLL;

	validate_scmi_channel(ch);

	scmi_get_channel(ch);

	scmi_send_sync_command(ch);

	scmi_put_channel(ch);

	/* Copy the result to agent's space */
	ret = copy_scmi_msg(scmi_mem, (uintptr_t)mbx_mem, scmi_mem_size);
	if (ret)
		return SCMI_OUT_OF_RANGE;

	return SCMI_SUCCESS;
}

int send_scmi_to_scp(uintptr_t scmi_mem, size_t scmi_mem_size)
{
	/* Filter OSPM specific call */
	if (!is_proto_allowed((mailbox_mem_t *)scmi_mem))
		return SCMI_DENIED;

	if (get_packet_size(scmi_mem) > S32_SCP_CH_MEM_SIZE)
		return SCMI_OUT_OF_RANGE;

	if (is_internal_msg((mailbox_mem_t *)scmi_mem))
		return handle_internal_msg(scmi_mem);

	return forward_to_scp(scmi_mem, scmi_mem_size);
}
