/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <assert.h>
#include <plat/common/platform.h>
#include <platform_def.h>

static unsigned char s32n_power_domain_tree_desc[PLATFORM_CLUSTER_COUNT + 2];

int plat_core_pos_by_mpidr(u_register_t mpidr)
{
	return 0;
}

const unsigned char *plat_get_power_domain_tree_desc(void)
{
	unsigned int i;

	assert(PLATFORM_CLUSTER_COUNT > 0U);
	assert(PLATFORM_MAX_CPUS_PER_CLUSTER == 2U);

	/* The root is the system level, followed by the clusters,
	 * and then adding the cores in each cluster.
	 */
	s32n_power_domain_tree_desc[0] = (unsigned char)PLATFORM_SYSTEM_COUNT;
	s32n_power_domain_tree_desc[1] = (unsigned char)PLATFORM_CLUSTER_COUNT;

	for (i = 0; i < PLATFORM_CLUSTER_COUNT; i++)
		s32n_power_domain_tree_desc[i + 2] = (unsigned char)PLATFORM_MAX_CPUS_PER_CLUSTER;

	return s32n_power_domain_tree_desc;
}
