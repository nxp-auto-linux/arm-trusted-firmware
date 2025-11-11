/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <string.h>

#include <common/debug.h>
#include <lib/utils_def.h>
#include <lib/xlat_tables/xlat_tables_v2.h>

#include <fconf/s32n_fconf_tlv_getter.h>
#include <s32_bl_common.h>
#include <s32n_fss_tlv.h>

#define FSS_TLV_MAGIC		0x53484D49U /* 'S''H''M''I' */
#define FSS_TLV_VERSION		1U

/*
 * The TLV contains a TLV header followed by one or more TLV entries.
 * The TLV header contains metadata and is represented by the fss_tlv_header
 * structure.
 * Each TLV entry contains an entry header and data following each other.
 * The TLV entry header is represented by the fss_tlv_entry_header structure.
 * The TLV entry data layout varies depending on the TLV entry type:
 *     * 0xFFF001 - see struct fss_soc_product and struct fss_soc_family
 *     * 0xFFF003 - see struct fss_boot_info
 *     * 0xFFF00B - see struct fss_boot_info
 *     * 0xFFF00C - XSPI controller memory map copy
 *     * 0xFFF00E - see struct fss_aps_local_addr
 */

/* TLV Header */
struct fss_tlv_header {
	uint32_t magic;		/* FSS_TLV_MAGIC */
	uint8_t flags;		/* 0 (not used) */
	uint8_t version;	/* 1 */
	uint8_t header_size;	/* expected 24 */
	uint8_t alignment;	/* specified as power of 2. */
	uint32_t checksum;	/* CRC32 of entries area (excludes this header) */
	uint32_t used_size;	/* bytes used by entries (excludes this header) */
	uint32_t total_size;	/* bytes used by entire TLV region (incl. header) */
	uint32_t reserved;	/* 0 */
} __packed;

/* TLV Entry Header */
struct fss_tlv_entry_header {
	uint8_t type[3];      /* type identifier, unique for each TLV entry. */
	uint8_t header_len;   /* typically 8 */
	uint32_t data_len;     /* data length in bytes */
} __packed;

/* Known TLV types */
enum {
	FSS_TLV_FF_F001_SOC_INFO              = 0xFFF001U,
	FSS_TLV_FF_F003_BOOT_INFO             = 0xFFF003U,
	FSS_TLV_FF_F00B_ARM_GENERIC_TIMER_HZ  = 0xFFF00BU,
	FSS_TLV_FF_F00C_XSPI_CTRL_SETTINGS    = 0xFFF00CU,
	FSS_TLV_FF_F00E_APS_PERIPH_ADDRESS    = 0xFFF00EU,
};

static inline uintptr_t fss_get_tlv_base(void)
{
	return FCONF_GET_PROPERTY(hw_config, tlv_config, tlv_base);
}

static inline size_t fss_get_tlv_size(void)
{
	return FCONF_GET_PROPERTY(hw_config, tlv_config, tlv_size);
}

static inline const struct fss_tlv_entry_header *
fss_tlv_first_entry(const struct fss_tlv_header *tlv_header)
{
	const uint8_t *base = (const uint8_t *)tlv_header;

	return (const struct fss_tlv_entry_header *)(base + tlv_header->header_size);
}

/* Returns SIZE_MAX in error case. */
static size_t
fss_tlv_entry_span_aligned(const struct fss_tlv_header *tlv_header,
			   const struct fss_tlv_entry_header *eh)
{
	size_t span = (size_t)eh->header_len + (size_t)eh->data_len;
	size_t align = 1U << tlv_header->alignment;
	size_t rem = span & (align - 1U);

	if (rem != 0U) {
		if (check_uptr_overflow(span, align - rem)) {
			ERROR("%s overflow detected\n", __func__);
			return SIZE_MAX;
		}
		span += (align - rem);
	}

	return span;
}

static const struct fss_tlv_entry_header *
fss_tlv_next_entry(const struct fss_tlv_header *tlv_header,
		   const struct fss_tlv_entry_header *eh)
{
	const uint8_t *tlv_header_base = (const uint8_t *)tlv_header;
	const uint8_t *entries_base = tlv_header_base + tlv_header->header_size;
	const uint8_t *entries_end  = entries_base + tlv_header->used_size;
	const size_t span = fss_tlv_entry_span_aligned(tlv_header, eh);
	const uint8_t *curr = (const uint8_t *)eh;
	const uint8_t *next;

	if (span == SIZE_MAX) {
		return NULL;
	}

	if (check_uptr_overflow((uintptr_t)curr, span)) {
		ERROR("%s overflow detected\n", __func__);
		return NULL;
	}

	next = curr + fss_tlv_entry_span_aligned(tlv_header, eh);

	return (next < entries_end) ?
		(const struct fss_tlv_entry_header *)next : NULL;
}

static const void *
fss_tlv_entry_data(const struct fss_tlv_entry_header *eh)
{
	return (const void *)((const uint8_t *)eh + eh->header_len);
}

static int fss_tlv_validate_entry(const struct fss_tlv_entry_header *eh, const uint8_t *end)
{
	const void *data;

	if (!eh) {
		return -EINVAL;
	}

	if (eh->header_len != sizeof(*eh)) {
		ERROR("%s: TLV entry header_len should be %x\n",
		      __func__, eh->header_len);
		return -EINVAL;
	}

	if (check_uptr_overflow((uintptr_t)eh, eh->header_len)) {
		ERROR("%s: TLV entry header overflow\n", __func__);
		return -ERANGE;
	}

	if ((const uint8_t *)eh + eh->header_len > end) {
		ERROR("%s: TLV entry header too big\n", __func__);
		return -ERANGE;
	}

	data = fss_tlv_entry_data(eh);

	if (check_uptr_overflow((uintptr_t)data, (size_t)eh->data_len)) {
		ERROR("%s: TLV entry overflow\n", __func__);
		return -ERANGE;
	}

	if ((const uint8_t *)data + eh->data_len > end) {
		ERROR("%s: TLV entry data overflow\n", __func__);
		return -ERANGE;
	}

	return 0;
}

static int fss_tlv_validate_entries(const struct fss_tlv_header *tlv_header)
{
	const struct fss_tlv_entry_header *eh;
	const uint8_t *end;
	int ret;

	eh = fss_tlv_first_entry(tlv_header);
	end = (const uint8_t *)tlv_header + tlv_header->header_size +
		tlv_header->used_size;

	while (eh) {
		ret = fss_tlv_validate_entry(eh, end);
		if (ret != 0) {
			return ret;
		}

		eh = fss_tlv_next_entry(tlv_header, eh);
	}

	return 0;
}

static int map_preboot_tlv_region(uintptr_t tlv_base, size_t tlv_size)
{
	static bool mmap_added;
	int ret;

	if (!is_mmu_el3_enabled() || mmap_added) {
		return 0;
	}

	ret = mmap_add_dynamic_region(tlv_base, tlv_base,
				      MMU_ROUND_UP_TO_PAGE(tlv_size),
				      MT_RO_DATA | MT_SECURE);
	if (ret != 0) {
		ERROR("Failed to map preboot TLV region: %d\n", ret);
		return ret;
	}

	mmap_added = true;

	return 0;
}

static int fss_tlv_bind(void)
{
	const unsigned int nr_bits = (unsigned int)(sizeof(size_t) * CHAR_BIT);
	const struct fss_tlv_header *tlv_header =
		(const struct fss_tlv_header *)fss_get_tlv_base();
	const size_t tlv_size = fss_get_tlv_size();
	static bool tlv_binded;
	int ret;

	if (!tlv_header || !tlv_size) {
		ERROR("No TLV region found\n");
		return -EINVAL;
	}

	ret = map_preboot_tlv_region((uintptr_t)tlv_header, tlv_size);
	if (ret != 0) {
		return ret;
	}

	if (tlv_binded) {
		return 0;
	}

	if (tlv_header->magic != FSS_TLV_MAGIC) {
		ERROR("FSS TLV: bad magic: 0x%08x\n", tlv_header->magic);
		return -EINVAL;
	}

	if (tlv_header->version != FSS_TLV_VERSION) {
		ERROR("FSS TLV: unsupported version: %u\n", tlv_header->version);
		return -EINVAL;
	}

	if (tlv_header->header_size != sizeof(struct fss_tlv_header)) {
		ERROR("FSS TLV: bad header size: %u\n", tlv_header->header_size);
		return -EINVAL;
	}

	if ((size_t)tlv_header->total_size > tlv_size) {
		ERROR("FSS TLV: total_size %u > region len %zu\n",
		      tlv_header->total_size, tlv_size);
		return -ERANGE;
	}

	if (check_u32_overflow((uint32_t)tlv_header->header_size,
			       tlv_header->used_size)) {
		ERROR("FSS TLV: header+used overflow\n");
		return -ERANGE;
	}

	if ((size_t)tlv_header->header_size + (size_t)tlv_header->used_size >
			(size_t)tlv_header->total_size) {
		ERROR("FSS TLV: header+used exceeds total size\n");
		return -ERANGE;
	}

	if (tlv_header->alignment >= nr_bits) {
		ERROR("FSS TLV: alignment could not exceed size_t number of bits\n");
		return -EINVAL;
	}

	ret = fss_tlv_validate_entries(tlv_header);
	if (ret != 0)
		return ret;

	tlv_binded = true;

	return 0;
}

static inline uint32_t fss_get_tlv_type(const struct fss_tlv_entry_header *eh)
{
	return (uint32_t)eh->type[0] |
		((uint32_t)eh->type[1] << 8) |
		((uint32_t)eh->type[2] << 16);
}

static const struct fss_tlv_entry_header *fss_find_first_by_type(uint32_t type)
{
	const struct fss_tlv_header *tlv_header =
		(const struct fss_tlv_header *)fss_get_tlv_base();
	const struct fss_tlv_entry_header *eh;
	const uint8_t *end;

	if (!tlv_header) {
		ERROR("No TLV region found\n");
		return NULL;
	}

	eh = fss_tlv_first_entry(tlv_header);
	end = (const uint8_t *)tlv_header + tlv_header->header_size +
		tlv_header->used_size;

	while (eh && ((const uint8_t *)eh + sizeof(*eh) <= end)) {
		if (fss_get_tlv_type(eh) == type) {
			return eh;
		}

		eh = fss_tlv_next_entry(tlv_header, eh);
	}

	return NULL;
}

int fss_tlv_get_arm_timer_freq(uint32_t *out_hz)
{
	const struct fss_tlv_entry_header *eh;
	const uint32_t *data;
	int ret;

	ret = fss_tlv_bind();
	if (ret != 0) {
		return ret;
	}

	if (!out_hz) {
		return -EINVAL;
	}

	eh = fss_find_first_by_type(FSS_TLV_FF_F00B_ARM_GENERIC_TIMER_HZ);
	if (!eh) {
		return -ENOENT;
	}

	if (eh->data_len < sizeof(uint32_t)) {
		return -ERANGE;
	}

	data = (const uint32_t *)fss_tlv_entry_data(eh);
	*out_hz = *data;

	return 0;
}

int fss_tlv_get_boot_info(const struct fss_boot_info **boot_info)
{
	const struct fss_tlv_entry_header *eh;
	const void *data;
	int ret;

	ret = fss_tlv_bind();
	if (ret != 0) {
		return ret;
	}

	if (!boot_info) {
		return -EINVAL;
	}

	eh = fss_find_first_by_type(FSS_TLV_FF_F003_BOOT_INFO);
	if (!eh) {
		return -ENOENT;
	}

	if (eh->data_len < sizeof(struct fss_boot_info)) {
		return -ERANGE;
	}

	data = fss_tlv_entry_data(eh);
	*boot_info = (const struct fss_boot_info *)data;

	return 0;
}

int fss_tlv_get_soc_info(struct fss_soc_product *prod,
			 struct fss_soc_family *family)
{
	const struct fss_tlv_entry_header *eh;
	uint8_t prod_len, fam_len;
	const uint8_t *data, *end;
	int ret;

	ret = fss_tlv_bind();
	if (ret != 0) {
		return ret;
	}

	if (!prod || !family) {
		return -EINVAL;
	}

	eh = fss_find_first_by_type(FSS_TLV_FF_F001_SOC_INFO);
	if (!eh) {
		return -ENOENT;
	}

	data = (const uint8_t *)fss_tlv_entry_data(eh);
	end = data + eh->data_len;

	/* Obtain product length. */
	if ((size_t)(end - data) < 1U) {
		return -ERANGE;
	}

	prod_len = *data++;
	if ((size_t)(end - data) < (size_t)prod_len + 1U) {
		return -ERANGE;
	}

	prod->name_len = prod_len;
	prod->name = (const char *)data;
	data += (size_t)prod_len + 1U; /* skip name + terminator */

	/* Obtain family length. */
	if ((size_t)(end - data) < 1U) {
		return -ERANGE;
	}

	fam_len = *data++;
	if ((size_t)(end - data) < (size_t)fam_len + 1U) {
		return -ERANGE;
	}

	family->name_len = fam_len;
	family->name = (const char *)data;

	return 0;
}

int fss_tlv_get_xspi_dump(const uint8_t **out_ptr, size_t *out_len)
{
	const struct fss_tlv_entry_header *eh;
	int ret;

	ret = fss_tlv_bind();
	if (ret != 0) {
		return ret;
	}

	if (!out_ptr || !out_len) {
		return -EINVAL;
	}

	eh = fss_find_first_by_type(FSS_TLV_FF_F00C_XSPI_CTRL_SETTINGS);
	if (!eh) {
		return -ENOENT;
	}

	*out_ptr = (const uint8_t *)fss_tlv_entry_data(eh);
	*out_len = (size_t)eh->data_len;

	return 0;
}

int fss_tlv_get_aps_local_addr(const struct fss_aps_local_addr **aps_local_addr)
{
	const struct fss_aps_local_addr *local_addr;
	const struct fss_tlv_entry_header *eh;
	const uint8_t *data, *end;
	size_t need;
	int ret;

	ret = fss_tlv_bind();
	if (ret != 0) {
		return ret;
	}

	if (!aps_local_addr) {
		return -EINVAL;
	}

	eh = fss_find_first_by_type(FSS_TLV_FF_F00E_APS_PERIPH_ADDRESS);
	if (!eh) {
		return -ENOENT;
	}

	local_addr = (const struct fss_aps_local_addr *)fss_tlv_entry_data(eh);

	/* Validate the ranges buffer is fully present according to aps_count. */
	data = (const uint8_t *)local_addr;
	end  = data + eh->data_len;
	need = sizeof(local_addr->aps_count);
	need += local_addr->aps_count * sizeof(struct fss_aps_range);

	if ((size_t)(end - data) < need) {
		return -ERANGE;
	}

	*aps_local_addr = local_addr;
	return 0;
}
