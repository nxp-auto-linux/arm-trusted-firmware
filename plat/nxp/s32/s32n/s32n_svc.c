/*
 * Copyright 2024-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <common/bl_common.h>
#include <common/debug.h>
#include <common/runtime_svc.h>
#include <drivers/scmi.h>
#include <s32_bl_common.h>
#include <s32_svc.h>
#include <s32n_bl_common.h>
#include <s32n_fconf_scmi_getter.h>
#include <s32n_fss_scmi.h>
#include <s32n_scmi_local.h>

#define S32N_SCMI_SIP_ID		0xc20000feU

static uintptr_t get_scmi_ospm_mem(void)
{
	return FCONF_GET_PROPERTY(s32n, scmi_ospm_ch_config, ospm_mem_base);
}

static size_t get_scmi_ospm_mem_size(void)
{
	return FCONF_GET_PROPERTY(s32n, scmi_ospm_ch_config, ospm_mem_size);
}

static uintptr_t get_scmi_ospm_notif_mem(void)
{
	return FCONF_GET_PROPERTY(s32n, scmi_ospm_ch_config, notif_mem_base);
}

static size_t get_scmi_ospm_notif_mem_size(void)
{
	return FCONF_GET_PROPERTY(s32n, scmi_ospm_ch_config, notif_mem_size);
}

static int map_scmi_ospm_notif_mem(uintptr_t ospm_mem)
{
	uintptr_t notif_mem = get_scmi_ospm_notif_mem();
	struct scmi_shared_mem *mem;
	uintptr_t base;
	size_t size;
	int ret;

	if (!is_ospm_notif_enabled()) {
		return 0;
	}

	/* Skip mapping if the notification buffer shares the OSPM channel's page. */
	if (page_align(notif_mem, DOWN) != page_align(ospm_mem, DOWN)) {
		base = page_align(notif_mem, DOWN);
		size = MMU_ROUND_UP_TO_PAGE(notif_mem - base +
					    get_scmi_ospm_notif_mem_size());

		ret = mmap_add_dynamic_region(base, base, size,
				MT_NON_CACHEABLE | MT_RW | MT_NS);
		if (ret) {
			ERROR("Error mapping SCMI OSPM notification region: %d\n",
			      ret);
			return ret;
		}
	}

	/*
	 * Hand the buffer to the OS: it owns the channel until TF-A places a
	 * notification in it.
	 */
	mem = (void *)notif_mem;
	mem->channel_status = SCMI_SHMEM_CHAN_STAT_CHANNEL_FREE;

	return 0;
}

static int32_t s32n_svc_setup(void)
{
	uintptr_t ospm_mem = get_scmi_ospm_mem();
	struct scmi_shared_mem *mem = (void *)ospm_mem;
	int ret;

	ret = mmap_add_dynamic_region(ospm_mem, ospm_mem,
			MMU_ROUND_UP_TO_PAGE(get_scmi_ospm_mem_size()),
			MT_NON_CACHEABLE | MT_RW | MT_NS);
	if (ret) {
		ERROR("Error mapping SCMI OSPM memory region: %d\n", ret);
		panic();
	}

	mem->channel_status = SCMI_SHMEM_CHAN_STAT_CHANNEL_FREE;

	ret = map_scmi_ospm_notif_mem(ospm_mem);
	if (ret) {
		ERROR("Error mapping SCMI OSPM notification memory region: %d\n",
		      ret);
		panic();
	}

	return 0;
}

static int fss_scmi_handler(uint32_t smc_fid, u_register_t x1,
			    u_register_t x2, u_register_t x3)
{
	uintptr_t ospm_mem = get_scmi_ospm_mem();
	size_t ospm_mem_size = get_scmi_ospm_mem_size();
	struct scmi_shared_mem *mem = (void *)ospm_mem;
	struct response *response = (struct response *)&mem->msg_payload[0];
	int ret;

	/**
	 * Requests describing static platform data are answered without
	 * involving FSS. Everything else is forwarded to FSS and the response
	 * is fetched back. The channel's ownership is transferred to FSS, so
	 * its status is updated only after receiving the response.
	 */
	if (!s32n_scmi_local_handle_request(mem, ospm_mem_size)) {
		ret = fss_scmi_send_sync_cmd(ospm_mem, ospm_mem_size);
		if (ret) {
			response->status = ret;
		} else {
			s32n_scmi_local_snoop_response(mem, ospm_mem_size);
		}
	}

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
