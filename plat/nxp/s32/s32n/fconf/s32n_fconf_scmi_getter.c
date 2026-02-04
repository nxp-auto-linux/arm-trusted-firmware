/*
 * Copyright 2024-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <errno.h>
#include <common/debug.h>
#include <common/fdt_wrappers.h>
#include <s32n_fconf_scmi_getter.h>
#include <s32n_fconf_utils.h>
#include <s32n_fss_mru.h>

#define MRU_MB_NUM_CELL		(4U)
#define SCMI_MAILBOX_NUM	(2U)
#define IRQ_CELL_SIZE		(3U)

struct scmi_ospm_channel_config scmi_ospm_ch_config = {0};
struct scmi_doorbell_config	scmi_doorbell_tx_config = {0};
struct scmi_doorbell_config	scmi_doorbell_rx_config = {0};
struct scmi_fss_channel_config scmi_fss_ch_tx_config = {0};
struct scmi_fss_channel_config scmi_fss_ch_rx_config = {0};
struct scmi_sh_mem_config scmi_sh_mem_config = {0};

static const char *scmi_fss_compatible = "nxp,s32n79-scmi";
static const char *scmi_smc_compatible = "arm,scmi-smc";
static const char *scmi_local_hs_compatible = "nxp,s32n79-scmi-fss-local";
static const char *scmi_remote_hs_compatible = "nxp,s32n79-scmi-fss-remote";

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

static int fconf_set_scmi_mbox_config(const void *hw_conf_dtb,
				      const fdt32_t *mbox, int mbox_size, scmi_dir_t dir)
{
	struct scmi_doorbell_config *db_cfg = &scmi_doorbell_tx_config;
	uintptr_t base = 0, db_addr = 0;
	const char *mbox_name = "TX";
	size_t size = 0;
	int node, ret;

	if (!mbox)
		return -FDT_ERR_BADSTRUCTURE;

	if (dir == SCMI_RX) {
		mbox_name = "RX";
		db_cfg = &scmi_doorbell_rx_config;
	}

	/* first entry is the mailbox phandle */
	node = fdt_node_offset_by_phandle(hw_conf_dtb, fdt32_to_cpu(mbox[0]));
	if (node < 0) {
		ERROR("FCONF: Invalid %s Mailbox phandle\n", mbox_name);
		return node;
	}

	if (!fdt_node_is_enabled(hw_conf_dtb, node)) {
		ERROR("FCONF: %s Mailbox node is disabled\n", mbox_name);
		return -FDT_ERR_BADSTATE;
	}

	ret = fconf_get_node_info(hw_conf_dtb, node, NULL, &base, &size);
	if (ret < 0) {
		ERROR("FCONF: Error retrieving %s Mailbox node info\n",
		      mbox_name);
		return ret;
	}

	db_cfg->base = base;
	db_cfg->size = size;

	/* Validate Mailbox Size */
	ret = check_db_mbox_size(hw_conf_dtb, node, mbox_size);
	if (ret < 0) {
		ERROR("FCONF: %s Mailbox size mismatch\n", mbox_name);
		return ret;
	}

	if (is_mru_mailbox(mbox_size)) {
		ret = get_mru_doorbell_addr(mbox, base, &db_addr);
		if (ret < 0) {
			ERROR("FCONF: Cannot compute %s doorbell address\n",
			      mbox_name);
			return ret;
		}
		db_cfg->db_addr = db_addr;
	} else {
		ERROR("FCONF: Unsupported  %s Mailbox format\n", mbox_name);
		return -FDT_ERR_BADSTRUCTURE;
	}

	return 0;
}

static int fconf_populate_scmi_db_mbox_config(uintptr_t config)
{
	const void *hw_conf_dtb = (const void *)config;
	const fdt32_t *mboxes_prop, *mbox;
	int node, mboxes_len, mbox_size;
	int ret;

	node = fdt_node_offset_by_compatible(hw_conf_dtb, -1, scmi_fss_compatible);
	if (node < 0) {
		ERROR("FCONF: Missing node with %s compatible.\n", scmi_fss_compatible);
		return node;
	}

	mboxes_prop = fdt_getprop(hw_conf_dtb, node, "mboxes", &mboxes_len);
	if (!mboxes_prop) {
		ERROR("FCONF: 'mboxes' property is missing\n");
		return mboxes_len;
	}

	mbox_size = mboxes_len / sizeof(uint32_t) / SCMI_MAILBOX_NUM;

	/* TX mailbox - index 0 */
	mbox = get_mbox_by_index(mboxes_prop, mboxes_len, mbox_size, 0);
	if (!mbox) {
		ERROR("FCONF: Invalid TX Mailbox\n");
		return -FDT_ERR_BADSTRUCTURE;
	}

	ret = fconf_set_scmi_mbox_config(hw_conf_dtb, mbox,
					 mbox_size, SCMI_TX);
	if (ret)
		return ret;

	/* RX mailbox - index 1 */
	mbox = get_mbox_by_index(mboxes_prop, mboxes_len, mbox_size, 1);
	if (!mbox) {
		ERROR("FCONF: Invalid RX Mailbox\n");
		return -FDT_ERR_BADSTRUCTURE;
	}

	return fconf_set_scmi_mbox_config(hw_conf_dtb, mbox,
					 mbox_size, SCMI_RX);
}

static int fconf_populate_scmi_sh_mem_config(uintptr_t config)
{
	const void *hw_conf_dtb = (const void *)config;
	uintptr_t base = 0;
	size_t size = 0;
	int ret;

	ret = fconf_get_node_info(hw_conf_dtb, -1, scmi_local_hs_compatible, &base, &size);
	if (ret < 0) {
		ERROR("FCONF: Error retrieving %s node info\n", scmi_local_hs_compatible);
		return ret;
	}

	scmi_sh_mem_config.local_handshake_addr = base;
	scmi_sh_mem_config.local_handshake_size = size;

	ret = fconf_get_node_info(hw_conf_dtb, -1, scmi_remote_hs_compatible, &base, &size);
	if (ret < 0) {
		ERROR("FCONF: Error retrieving %s node info\n", scmi_remote_hs_compatible);
		return ret;
	}

	scmi_sh_mem_config.remote_handshake_addr = base;
	scmi_sh_mem_config.remote_handshake_size = size;

	return 0;
}

static int fconf_get_shmem_by_phandle(const void *hw_conf_dtb,
				      const char *compatible, uint32_t phandle_idx,
				      uintptr_t *base, size_t *size)
{
	const fdt32_t *shmem_prop;
	int node, shmem_len, ret;

	node = fdt_node_offset_by_compatible(hw_conf_dtb, -1, compatible);
	if (node < 0) {
		ERROR("FCONF: Missing node with %s compatible.\n", compatible);
		return node;
	}

	/* Get shmem property which contains phandle(s) */
	shmem_prop = fdt_getprop(hw_conf_dtb, node, "shmem", &shmem_len);
	if (!shmem_prop) {
		ERROR("FCONF: 'shmem' property is missing from %s node\n", compatible);
		return shmem_len;
	}

	if (shmem_len < (int)((phandle_idx + 1) * sizeof(uint32_t))) {
		ERROR("FCONF: Invalid 'shmem' property length\n");
		return -FDT_ERR_BADSTRUCTURE;
	}

	/* Get node by phandle at requested index */
	node = fdt_node_offset_by_phandle(hw_conf_dtb,
					  fdt32_to_cpu(shmem_prop[phandle_idx]));
	if (node < 0) {
		ERROR("FCONF: Invalid shmem phandle at index %u\n", phandle_idx);
		return node;
	}

	/* Get base and size from the shmem node */
	ret = fconf_get_node_info(hw_conf_dtb, node, NULL, base, size);
	if (ret < 0) {
		ERROR("FCONF: Error retrieving shmem node info\n");
		return ret;
	}

	return 0;
}

static int fconf_populate_scmi_fss_mb_config(uintptr_t config)
{
	const void *hw_conf_dtb = (const void *)config;
	int ret;

	/* First phandle is TX mailbox */
	ret = fconf_get_shmem_by_phandle(hw_conf_dtb,
					 scmi_fss_compatible, 0,
					 &scmi_fss_ch_tx_config.mb_base,
					 &scmi_fss_ch_tx_config.mb_size);
	if (ret) {
		ERROR("FCONF: Error retrieving FSS SCMI TX node info\n");
		return ret;
	}

	/* Second phandle is RX mailbox */
	ret = fconf_get_shmem_by_phandle(hw_conf_dtb,
					 scmi_fss_compatible, 1,
					 &scmi_fss_ch_rx_config.mb_base,
					 &scmi_fss_ch_rx_config.mb_size);
	if (ret) {
		ERROR("FCONF: Error retrieving FSS SCMI TX node info\n");
		return ret;
	}

	return 0;
}

static int fconf_populate_scmi_ospm_ch_config(uintptr_t config)
{
	const void *hw_conf_dtb = (const void *)config;
	int ret;

	/* Find ospm-scmi node (TF-A to Linux/OSPM) */
	ret = fconf_get_shmem_by_phandle(hw_conf_dtb,
					 scmi_smc_compatible, 0,
					 &scmi_ospm_ch_config.ospm_mem_base,
					 &scmi_ospm_ch_config.ospm_mem_size);
	if (ret) {
		ERROR("FCONF: Error retrieving OSPM SCMI mailbox node info\n");
		return ret;
	}

	return 0;
}

int fconf_populate_scmi_config(uintptr_t config)
{
	int ret;

	ret = fconf_populate_scmi_sh_mem_config(config);
	if (ret)
		return ret;

	ret = fconf_populate_scmi_fss_mb_config(config);
	if (ret)
		return ret;

	ret = fconf_populate_scmi_db_mbox_config(config);
	if (ret)
		return ret;

	return fconf_populate_scmi_ospm_ch_config(config);
}
