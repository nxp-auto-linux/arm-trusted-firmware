/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef S32CC_IMG_DEF_H
#define S32CC_IMG_DEF_H

#include <export/common/tbbr/tbbr_img_def_exp.h>

#define DDR_FW_IMAGE_ID         MAX_IMG_IDS_WITH_SPMDS
#define DDR_FW_KEY_CERT_ID      MAX_IMG_IDS_WITH_SPMDS + 1
#define DDR_FW_CONTENT_CERT_ID  MAX_IMG_IDS_WITH_SPMDS + 2

#define MAX_IMG_WITH_DDR_IDS		MAX_IMG_IDS_WITH_SPMDS + 3

#define MAX_NUMBER_IDS			MAX_IMG_WITH_DDR_IDS

#endif	/* S32CC_IMG_DEF_H */
