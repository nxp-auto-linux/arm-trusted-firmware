/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <plat/common/platform.h>
#include <tools_share/tbbr_oid.h>
#include <drivers/mmc.h>
#include <drivers/nxp/s32/hse/hse_core.h>
#include <drivers/nxp/s32/hse/hse_mem.h>
#include <errno.h>

#define TRUSTED_FW_NVCOUNTER_OID_LEN (sizeof(TRUSTED_FW_NVCOUNTER_OID) - 1)
#define TRUSTED_OS_NVCOUNTER_OID_LEN (sizeof(TRUSTED_OS_NVCOUNTER_OID) - 1)
#define NON_TRUSTED_FW_NVCOUNTER_OID_LEN (sizeof(NON_TRUSTED_FW_NVCOUNTER_OID) - 1)

struct oid_cnt {
	char *oid;
	uint32_t len;
	uint32_t idx;
};

static const struct oid_cnt oid_cnt_map[] = {
	{.oid = TRUSTED_FW_NVCOUNTER_OID, .len = TRUSTED_FW_NVCOUNTER_OID_LEN, .idx = 0},
	{.oid = TRUSTED_OS_NVCOUNTER_OID, .len = TRUSTED_OS_NVCOUNTER_OID_LEN, .idx = 1},
	{.oid = NON_TRUSTED_FW_NVCOUNTER_OID, .len = NON_TRUSTED_FW_NVCOUNTER_OID_LEN, .idx = 2},
};

static uint32_t nv_counters[3] = {0};

int plat_get_rotpk_info(void *cookie, void **key_ptr, unsigned int *key_len,
			unsigned int *flags)
{
	*flags = ROTPK_NOT_DEPLOYED;
	return 0;
}

static bool has_monotonic_cnt(void)
{
	return S32CC_MONOTONIC_CNT_SUPPORT == 1;
}

static int get_cnt_table_idx(void *oid, uint32_t *table_idx)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(oid_cnt_map); i++) {
		if (!strncmp(oid, oid_cnt_map[i].oid, oid_cnt_map[i].len)) {
			*table_idx = oid_cnt_map[i].idx;
			return 0;
		}
	}
	return -EINVAL;
}

static int hse_get_cnt_value(uint32_t cnt_idx, unsigned int *nv_ctr)
{
	hseSrvDescriptor_t srv_desc = {0};
	uint64_t *hse_nv_ctr;
	int ret;

	hse_nv_ctr = hse_mem_alloc(sizeof(*hse_nv_ctr));
	if (!hse_nv_ctr)
		return -ENOMEM;

	srv_desc.srvId = HSE_SRV_ID_READ_COUNTER;
	srv_desc.hseSrv.readCounterReq.counterIndex = cnt_idx;
	srv_desc.hseSrv.readCounterReq.pCounterVal = hse_virt_to_phys(hse_nv_ctr);

	ret = hse_srv_req_sync(HSE_CHANNEL_CRYPTO, &srv_desc);
	if (ret)
		goto free_hse_nv_ctr;

	hse_memcpy(nv_ctr, hse_nv_ctr, sizeof(*nv_ctr));

free_hse_nv_ctr:
	hse_mem_free(hse_nv_ctr);
	return ret;
}

static int hse_inc_cnt_value(uint32_t cnt_idx, uint32_t inc_val)
{
	hseSrvDescriptor_t srv_desc = {0};
	int ret;

	srv_desc.srvId = HSE_SRV_ID_INCREMENT_COUNTER;
	srv_desc.hseSrv.incCounterReq.counterIndex = cnt_idx;
	srv_desc.hseSrv.incCounterReq.value = inc_val;

	ret = hse_srv_req_sync(HSE_CHANNEL_CRYPTO, &srv_desc);

	return ret;
}

static int hse_publish_cnt_table(void)
{
	hseKeyHandle_t rpmb_key_handle = RPMB_KEY_HSE_KEYHANDLE;
	uint8_t rpmb_cnt_table[RPMB_BLOCK_SIZE] = {0};
	hseSrvDescriptor_t srv_desc = {0};
	void *hse_cnt_table;
	int ret;

	hse_cnt_table = hse_mem_alloc(HSE_NVM_CONTAINER_CNT_TBL_SIZE);
	if (!hse_cnt_table)
		return -ENOMEM;

	srv_desc.srvId = HSE_SRV_ID_PUBLISH_LOAD_CNT_TBL;
	srv_desc.hseSrv.publishLoadCntTblReq.action = HSE_NVM_CNT_TBL_ACTION_PUBLISH;
	srv_desc.hseSrv.publishLoadCntTblReq.pNvmContainerCntTbl = hse_virt_to_phys(hse_cnt_table);

	ret = hse_srv_req_sync(HSE_CHANNEL_CRYPTO, &srv_desc);
	if (ret)
		goto free_hse_cnt_table;

	hse_memcpy(rpmb_cnt_table, hse_cnt_table, HSE_NVM_CONTAINER_CNT_TBL_SIZE);

	ret = mmc_rpmb_write_blocks(RPMB_MONOTONIC_CNT_ADDR, (uintptr_t)rpmb_cnt_table,
				RPMB_BLOCK_SIZE, (void *)&rpmb_key_handle, sizeof(rpmb_key_handle));
	ret = ret ? 0 : -EINVAL;

free_hse_cnt_table:
	hse_mem_free(hse_cnt_table);
	return ret;
}

int plat_get_nv_ctr(void *cookie, unsigned int *nv_ctr)
{
	uint32_t cnt_idx;
	int ret;

	if (!has_monotonic_cnt()) {
		*nv_ctr = 0;
		return 0;
	}

	ret = get_cnt_table_idx(cookie, &cnt_idx);
	if (ret)
		return ret;

	ret = hse_get_cnt_value(cnt_idx, nv_ctr);
	if (!ret)
		nv_counters[cnt_idx] = *nv_ctr;

	return ret;
}

int plat_set_nv_ctr(void *cookie, unsigned int nv_ctr)
{
	uint32_t cnt_idx, hse_cnt_inc;
	int ret;

	if (!has_monotonic_cnt())
		return 0;

	ret = get_cnt_table_idx(cookie, &cnt_idx);
	if (ret)
		return ret;

	hse_cnt_inc = nv_ctr - nv_counters[cnt_idx];
	ret = hse_inc_cnt_value(cnt_idx, hse_cnt_inc);
	if (ret)
		return ret;

	ret = hse_publish_cnt_table();

	return ret;
}

int plat_get_mbedtls_heap(void **heap_addr, size_t *heap_size)
{
	return get_mbedtls_heap_helper(heap_addr, heap_size);
}
