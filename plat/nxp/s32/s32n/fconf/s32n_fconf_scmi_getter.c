/*
 * Copyright 2024-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <common/debug.h>
#include <common/fdt_wrappers.h>
#include <s32n_fconf_scmi_getter.h>
#include <s32n_fss_mru.h>

/* Default FSS mailbox addresses */
#define FSS_SCMI_SH_MEM_0	(0x2483A000)
#define FSS_SCMI_SH_MEM_1	(0x2483B000)
#define FSS_SCMI_SH_MEM_2	(0x2483C000)
#define FSS_SCMI_SH_MEM_3	(0x2483D000)

#define S32N_FSS_SCMI_SH_MEM_SIZE	(4U * 1024U)

/* Default mailbox configurations */
#if (S32N_SCMI_MBX == 0)
#define S32N_FSS_SCMI_SH_MEM		FSS_SCMI_SH_MEM_0
#elif (S32N_SCMI_MBX == 1)
#define S32N_FSS_SCMI_SH_MEM		FSS_SCMI_SH_MEM_1
#elif (S32N_SCMI_MBX == 2)
#define S32N_FSS_SCMI_SH_MEM		FSS_SCMI_SH_MEM_2
#elif (S32N_SCMI_MBX == 3)
#define S32N_FSS_SCMI_SH_MEM		FSS_SCMI_SH_MEM_3
#else
#error Invalid SCMI mailbox configuration!
#endif

#define MRU_MB_NUM_CELL		(4U)
#define SCMI_MAILBOX_NUM	(1U)

/* Used in the handshake procedure */
#define S32N_FSS_REMOTE_HANDSHAKE	(S32N_FSS_SCMI_SH_MEM + 0x0)
#define S32N_FSS_LOCAL_HANDSHAKE	(S32N_FSS_SCMI_SH_MEM + 0x40)

struct scmi_ospm_channel_config scmi_ospm_ch_config = {0};
struct scmi_fss_channel_config scmi_fss_ch_config = {0};
struct scmi_doorbell_config	scmi_doorbell_config = {0};

/**
 * Do not use a populate function, as the properties
 * are not read from the device tree.
 */
struct scmi_sh_mem_config scmi_sh_mem_config = {
	.sh_mem_base = S32N_FSS_SCMI_SH_MEM,
	.sh_mem_size = S32N_FSS_SCMI_SH_MEM_SIZE,
	.remote_handshake_addr = S32N_FSS_REMOTE_HANDSHAKE,
	.local_handshake_addr = S32N_FSS_LOCAL_HANDSHAKE,
};

static const char *scmi_compatible = "arm,scmi-smc";
static const char *scmi_ospm_compatible = "arm,scmi-shmem";
static const char *scmi_fss_tx_compatible = "nxp,s32n79-scmi-fss-tx";

static inline bool is_mru_mailbox(uint32_t mbx_size)
{
	return mbx_size == MRU_MB_NUM_CELL;
}

static uintptr_t mru_db_addr(uintptr_t base, uint32_t ch, uint32_t mb)
{
	return base + (ch * FSS_MRU_CH_OFFSET) + (mb * FSS_MRU_MB_OFFSET);
}

/* Check '#mbox-cells' value against an expected size that includes the
 * phandle: e.g.: <phandle chan mb>
 */
static int check_db_mbox_size(const void *dtb, int node, uint32_t mbox_size)
{
	const fdt32_t *cells;

	cells = fdt_getprop(dtb, node, "#mbox-cells", NULL);
	if (cells < 0)
		return -FDT_ERR_BADSTRUCTURE;

	if (fdt32_to_cpu(*cells) + 1 != mbox_size)
		return -FDT_ERR_BADSTRUCTURE;

	return 0;
}

static int get_mru_doorbell_addr(const fdt32_t *mbox, uintptr_t base,
	uintptr_t *db_addr)
{
	uint32_t chan, mb;

	if (!mbox)
		return -FDT_ERR_BADSTRUCTURE;

	/* MRU mailbox: <phandle channel mailbox type> */
	chan = fdt32_to_cpu(mbox[1]);
	if (chan >= FSS_MRU_NUM_CHANNELS) {
		ERROR("FCONF: Invalid MRU channel index\n");
		return -FDT_ERR_BADSTRUCTURE;
	}

	mb = fdt32_to_cpu(mbox[2]);
	if (mb >= FSS_MRU_NUM_MB) {
		ERROR("FCONF: Invalid MRU mailbox index\n");
		return -FDT_ERR_BADSTRUCTURE;
	}

	*db_addr = mru_db_addr(base, chan, mb);

	return 0;
}

static const fdt32_t *get_mbox_by_index(const fdt32_t *mboxes_array,
	uint32_t mboxes_len, uint32_t mbox_cell_size, uint32_t index)
{
	if (index * mbox_cell_size >= mboxes_len)
		return NULL;

	return mboxes_array + index * mbox_cell_size;
}

static int fconf_set_scmi_doorbell_config(const void *hw_conf_dtb,
	const fdt32_t *mbox, int mbox_size)
{
	uintptr_t base = 0, db_addr = 0;
	size_t size = 0;
	int node, ret;

	if (!mbox)
		return -FDT_ERR_BADSTRUCTURE;

	/* first entry is the mailbox phandle */
	node = fdt_node_offset_by_phandle(hw_conf_dtb, fdt32_to_cpu(mbox[0]));
	if (node < 0) {
		ERROR("FCONF: Invalid mailbox phandle\n");
		return node;
	}

	if (!fdt_node_is_enabled(hw_conf_dtb, node)) {
		ERROR("FCONF: Doorbell node is disabled\n");
		return -FDT_ERR_BADSTATE;
	}

	ret = fdt_get_reg_props_by_index(hw_conf_dtb, node, 0, &base, &size);
	if (ret < 0)
		return ret;

	scmi_doorbell_config.base = base;
	scmi_doorbell_config.size = size;

	/* validate doorbell mailbox size */
	ret = check_db_mbox_size(hw_conf_dtb, node, mbox_size);
	if (ret < 0) {
		ERROR("FCONF: Mailbox size mismatch\n");
		return ret;
	}

	if (is_mru_mailbox(mbox_size)) {
		ret = get_mru_doorbell_addr(mbox, base, &db_addr);
		if (ret < 0) {
			ERROR("FCONF: Cannot compute TX doorbell address\n");
			return ret;
		}

		scmi_doorbell_config.db_addr = db_addr;
	} else {
		ERROR("FCONF: Unsupported mailbox format\n");
		return -FDT_ERR_BADSTRUCTURE;
	}

	return 0;
}

static int fconf_populate_scmi_doorbell_config(uintptr_t config)
{
	const void *hw_conf_dtb = (const void *)config;
	const fdt32_t *mboxes_prop, *mbox;
	int node, mboxes_len, mbox_size;

	node = fdt_node_offset_by_compatible(hw_conf_dtb, -1, scmi_compatible);
	if (node < 0) {
		ERROR("FCONF: Missing node with %s compatible.\n", scmi_compatible);
		return node;
	}

	mboxes_prop = fdt_getprop(hw_conf_dtb, node, "mboxes", &mboxes_len);
	if (!mboxes_prop) {
		ERROR("FCONF: 'mboxes' property is missing\n");
		return mboxes_len;
	}

	mbox_size = mboxes_len / sizeof(uint32_t) / SCMI_MAILBOX_NUM;

	/* TX mailbox */
	mbox = get_mbox_by_index(mboxes_prop, mboxes_len, mbox_size, 0);
	if (!mbox) {
		ERROR("FCONF: Invalid mailbox\n");
		return -FDT_ERR_BADSTRUCTURE;
	}

	return fconf_set_scmi_doorbell_config(hw_conf_dtb, mbox, mbox_size);
}

static int fconf_populate_scmi_fss_mb_config(uintptr_t config)
{
	const void *hw_conf_dtb = (const void *)config;
	uintptr_t base = 0;
	size_t size = 0;
	int node, ret;

	node = fdt_node_offset_by_compatible(hw_conf_dtb, -1, scmi_fss_tx_compatible);
	if (node < 0) {
		ERROR("FCONF: Missing node with %s compatible.\n", scmi_fss_tx_compatible);
		return node;
	}

	ret = fdt_get_reg_props_by_index(hw_conf_dtb, node, 0, &base, &size);
	if (ret < 0) {
		ERROR("FCONF: Fail to read SCMI mailbox reg property\n");
		return ret;
	}

	scmi_fss_ch_config.mb_base = base;
	scmi_fss_ch_config.mb_size = size;

	return 0;
}

static int fconf_populate_scmi_ospm_ch_config(uintptr_t config)
{
	const void *hw_conf_dtb = (const void *)config;
	int scmi_ospm_node, ret;
	uintptr_t base = 0;
	size_t size = 0;

	fdt_for_each_compatible_node(hw_conf_dtb, scmi_ospm_node, scmi_ospm_compatible) {
		if (!fdt_node_is_enabled(hw_conf_dtb, scmi_ospm_node)) {
			continue;
		}

		ret = fdt_get_reg_props_by_index(hw_conf_dtb, scmi_ospm_node, 0,
						 &base, &size);
		if (ret)
			continue;

		scmi_ospm_ch_config.ospm_mem_base = base;
		scmi_ospm_ch_config.ospm_mem_size = size;
		return 0;
	}

	ERROR("FCONF: Can't find %s compatible node in dtb\n", scmi_ospm_compatible);
	return -FDT_ERR_BADSTRUCTURE;
}

int fconf_populate_scmi_config(uintptr_t config)
{
	int ret;

	ret = fconf_populate_scmi_fss_mb_config(config);
	if (ret)
		return ret;

	ret = fconf_populate_scmi_doorbell_config(config);
	if (ret)
		return ret;

	return fconf_populate_scmi_ospm_ch_config(config);
}
