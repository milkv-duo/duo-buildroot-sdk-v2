/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: ae_project_param.h
 * Description:
 *
 */

#ifndef _AE_PROJECT_PARAM_H_
#define _AE_PROJECT_PARAM_H

#include "ae_common.h"


#define LINEAR_MODE_FPS	25	//30
#define WDR_MODE_FPS	25

#define MIN_WDR_RATIO	(2 * AE_WDR_RATIO_BASE)
#define MAX_WDR_RATIO	(256 * AE_WDR_RATIO_BASE)

#define BOOT_ISO_ENTRY	(ISO_100_Entry)
#define BOOT_TV_ENTRY	(EVTT_ENTRY_1_480SEC)


#define	ENABLE_SMOOTH_AE	1
#define	ISO_LIMIT_BY_BLC	1
#define ISPDGAIN_FIRST		0
#define DGAIN_TO_AGAIN		0
#define ENABLE_ISPDGAIN_COMPENSATION	1
#define ENABLE_METER_EVERY_FRAME	1

#define ENABLE_AE_DEBUG_LOG_TO_FILE 1

#endif // _AE_PROJECT_PARAM_H_
