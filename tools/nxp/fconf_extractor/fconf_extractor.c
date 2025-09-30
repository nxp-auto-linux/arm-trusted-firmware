/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <libfdt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <common/debug.h>
#include <fconf_extractor.h>

/* Interpreted by GDB as an exit status code */
int global_ret;

static void breakpoint(void)
{
	INFO("%s function\n", __func__);
}

int main(int argc, char *argv[])
{
	size_t bytes_read;
	void *dtb = NULL;
	long size;
	FILE *fp;

	global_ret = -EINVAL;
	if (argc != 2) {
		printf("Usage: %s <device_tree_file.dtb>\n", argv[0]);
		printf("\nThis tool parses FCONF configuration from a device tree blob.\n");
		goto exit;
	}

	fp = fopen(argv[1], "rb");
	if (!fp) {
		ERROR("Failed to open device tree file: %s\n", argv[1]);
		goto exit;
	}

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	dtb = malloc(size);
	if (!dtb) {
		ERROR("Failed to allocate memory for DTB\n");
		fclose(fp);

		global_ret = -ENOMEM;
		goto exit;
	}

	bytes_read = fread(dtb, 1, size, fp);
	fclose(fp);

	if (bytes_read != (size_t)size) {
		ERROR("Failed to read complete DTB file\n");

		global_ret = -EIO;
		goto exit;
	}

	/* Validate DTB */
	if (fdt_check_header(dtb) != 0) {
		ERROR("Invalid device tree blob\n");

		global_ret = -EIO;
		goto exit;
	}

	INFO("Processing device tree blob: %s\n", argv[1]);
	INFO("DTB size: %ld bytes\n", size);

	global_ret = plat_populate_fconf_extractor((uintptr_t)dtb);

exit:
	printf("Exit code: %d\n", global_ret);
	/*
	 * GDB is expected to set a breakpoint on this function. Therefore, the
	 * call should be placed as close as possible to the end of execution
	 * to ensure that global_ret is set and all FCONF structures are fully
	 * populated.
	 */
	breakpoint();

	free(dtb);
	return global_ret;
}

