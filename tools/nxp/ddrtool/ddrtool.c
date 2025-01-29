/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "ddr_init.h"
#include "ddrtool.h"

#include "s32g2/s32g2_cfg.h"
#include "s32g3/s32g3_cfg.h"
#include "s32r45evb/s32r45evb_cfg.h"
#include "s32r45renan/s32r45renan_cfg.h"

const char *valid_platforms[] = {
	"s32g274abluebox3",
	"s32g274aemu",
	"s32g274ardb2",
	"s32g2xxaevb",
	"s32g2xxaevb3",
	"s32g399abase",
	"s32g399aemu",
	"s32g399ardb3",
	"s32g399asom",
	"s32g3xxaevb",
	"s32g3xxaevb3",
	"s32g3xxaevb3rmii",
	"s32r45evb",
	"s32r45renan"
};

static void usage(void);
static bool is_platform_valid(char *plat);
static int create_ddr_bin(char *plat, bool enable_ecc, bool enable_perf, char *output_file);

static bool is_platform_valid(char *plat)
{
	for (size_t i = 0; i < ARRAY_SIZE(valid_platforms); i++) {
		if (strstr(plat, valid_platforms[i]))
			return true;
	}

	return false;
}

static int create_ddr_bin(char *plat, bool enable_ecc, bool enable_perf, char *output_file)
{
	ddr_fw_t ddr_fw;
	ddr_params_t params = {
		.enable_ecc = enable_ecc,
		.enable_perf = enable_perf
	};
	FILE *output;
	int res;

	if (!is_platform_valid(plat)) {
		perror("Invalid platform specified.\n");
		return -EINVAL;
	}

	output = fopen(output_file, "w");
	if (!output) {
		perror("Error opening input file.\n");
		return -ENOENT;
	}

	if (strstr(plat, "s32g2")) {
		s32g2_populate_ddr_conf(&ddr_fw, &params);
	} else if (strstr(plat, "s32g3")) {
		s32g3_populate_ddr_conf(&ddr_fw, &params);
	} else if (strstr(plat, "s32r45renan")) {
		s32r45renan_populate_ddr_conf(&ddr_fw, &params);
	} else if (strstr(plat, "s32r")) {
		s32r45evb_populate_ddr_conf(&ddr_fw, &params);
	} else {
		perror("The specified platform is not supported.\n");
		fclose(output);
		return -EINVAL;
	}

	struct {
		uint8_t *data;
		size_t size;
		size_t nmemb;
		const char *name;
	} fentries[] = {
		{ .data = (uint8_t *)&ddr_fw.ddr_hdr,
		  .size = sizeof(ddr_fw.ddr_hdr),
		  .nmemb = 1,
		  .name = "DDR FW header" },
		{ .data = (uint8_t *)ddr_fw.ddrc_cfg,
		  .size = sizeof(*ddr_fw.ddrc_cfg),
		  .nmemb = ddr_fw.ddr_hdr.ddrc_size,
		  .name = "DDRC config" },
		{ .data = (uint8_t *)ddr_fw.dq_swap_cfg,
		  .size = sizeof(*ddr_fw.dq_swap_cfg),
		  .nmemb = ddr_fw.ddr_hdr.dq_swap_size,
		  .name = "DQ SWAP config" },
		{ .data = (uint8_t *)ddr_fw.phy_cfg,
		  .size = sizeof(*ddr_fw.phy_cfg),
		  .nmemb = ddr_fw.ddr_hdr.phy_size,
		  .name = "PHY config" },
		{ .data = (uint8_t *)ddr_fw.imem_1d_cfg,
		  .size = sizeof(*ddr_fw.imem_1d_cfg),
		  .nmemb = ddr_fw.ddr_hdr.imem_1d_size,
		  .name = "IMEM 1D config" },
		{ .data = (uint8_t *)ddr_fw.dmem_1d_cfg,
		  .size = sizeof(*ddr_fw.dmem_1d_cfg),
		  .nmemb = ddr_fw.ddr_hdr.dmem_1d_size,
		  .name = "DMEM 1D config" },
		{ .data = (uint8_t *)ddr_fw.imem_2d_cfg,
		  .size = sizeof(*ddr_fw.imem_2d_cfg),
		  .nmemb = ddr_fw.ddr_hdr.imem_2d_size,
		  .name = "IMEM 2D config" },
		{ .data = (uint8_t *)ddr_fw.dmem_2d_cfg,
		  .size = sizeof(*ddr_fw.dmem_2d_cfg),
		  .nmemb = ddr_fw.ddr_hdr.dmem_2d_size,
		  .name = "DMEM 2D config" },
		{ .data = (uint8_t *)ddr_fw.pie_cfg,
		  .size = sizeof(*ddr_fw.pie_cfg),
		  .nmemb = ddr_fw.ddr_hdr.pie_size,
		  .name = "PIE config" },
		{ .data = (uint8_t *)ddr_fw.csr_to_store,
		  .size = sizeof(*ddr_fw.csr_to_store),
		  .nmemb = ddr_fw.ddr_hdr.csr_to_store_size,
		  .name = "CSR config" },
		{ .data = (uint8_t *)ddr_fw.ddrc_to_store,
		  .size = sizeof(*ddr_fw.ddrc_to_store),
		  .nmemb = ddr_fw.ddr_hdr.ddrc_to_store_size,
		  .name = "DDRC STORE config" },
	};

	for (size_t i = 0; i < ARRAY_SIZE(fentries); i++) {
		res = fwrite(fentries[i].data, fentries[i].size, fentries[i].nmemb, output);
		if (res != fentries[i].nmemb) {
			printf("Error writing %s to output file.\n", fentries[i].name);
			fclose(output);
			return -EIO;
		}
	}
	fwrite(&ddr_fw.ddr_fw_end, sizeof(ddr_fw.ddr_fw_end), 1, output);

	fclose(output);

	return 0;
}

static void usage(void)
{
	printf("%s: ddrtool --plat <PLAT> [--enable-ecc] [--enable-perf] --output <OUTPUT_FILE>\n",
	       __func__);
	printf("\n");
	printf("Generates DDR Firmware binary file based on the specific platform.\n");
	printf("\n");
	printf("Options:\n");
	printf("  --plat <PLAT>		Specify the platform for which DDR Firmware is needed.\n");
	printf("\n");
	printf("  --enable-ecc		Enable ECC. If not specified, ECC is disabled.\n");
	printf("\n");
	printf("  --enable-perf		Enable PERF. If not specified, PERF is disabled.\n");
	printf("\n");
	printf("  --output		Specify the path for the output binary location.\n");
	printf("\n");
	printf("  --list-platforms	List all available platforms.\n");
	printf("\n");
	printf("  --h, --help		Show this help message and exit.\n");

	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
	char *output_file = NULL;
	char *plat = NULL;
	int enable_perf = 0;
	int enable_ecc = 0;

	while (1) {
		int opt, opt_index = 0;
		static struct option opts[] = {
			{ "plat", required_argument, NULL, 'P'},
			{ "enable-ecc", no_argument, NULL, 'e'},
			{ "enable-perf", no_argument, NULL, 'p'},
			{ "output", required_argument, NULL, 'o'},
			{ "list-platforms", no_argument, NULL, 'l'},
			{ "help", no_argument, NULL, 'h'}
		};

		/*
		 * Set POSIX mode so getopt stops at the first non-option
		 * which is the subcommand.
		 */
		opt = getopt_long(argc, argv, "P:e:p:o:h:l", opts, &opt_index);
		if (opt == -1)
			break;

		switch (opt) {
		case 'P':
			plat = optarg;
			break;
		case 'e':
			enable_ecc = 1;
			break;
		case 'p':
			enable_perf = 1;
			break;
		case 'o':
			output_file = optarg;
			break;
		case 'l':
			printf("Available platforms: ");
			for (size_t i = 0; i < ARRAY_SIZE(valid_platforms); i++) {
				printf("%s| ", valid_platforms[i]);
			}
			printf("\n");
			break;
		case 'h':
			usage();
			break;
		default:
			printf("Unknown option '%s'\n", argv[optind - 1]);
			break;
		}
	}

	if (plat == NULL || output_file == NULL) {
		perror("Both platform and output file must be specified.\n");
		usage();
	}

	return create_ddr_bin(plat, enable_ecc, enable_perf, output_file);
}
