/*
 * Copyright (C) Cvitek Co., Ltd. 2013-2024. All rights reserved.
 *
 * File Name: teaisp.c
 * Description:
 *
 */

#include "clog.h"
#include "isp_defines.h"
#include "isp_ioctl.h"
#include "cvi_isp.h"
#include "teaisp_helper.h"
#include "teaisp_bnr_helper_wrap.h"
#include "teaisp.h"

static int g_max_dev;

static void reg_helper_instance(void)
{
	static struct teaisp_helper_instance instance;

	instance.teaisp_bnr_load_model = teaisp_bnr_load_model_wrap;
	instance.teaisp_bnr_unload_model = teaisp_bnr_unload_model_wrap;

	instance.teaisp_bnr_set_driver_init = teaisp_bnr_set_driver_init_wrap;
	instance.teaisp_bnr_set_driver_deinit = teaisp_bnr_set_driver_deinit_wrap;
	instance.teaisp_bnr_set_driver_start = teaisp_bnr_set_driver_start_wrap;
	instance.teaisp_bnr_set_driver_stop = teaisp_bnr_set_driver_stop_wrap;
	instance.teaisp_bnr_set_api_info = teaisp_bnr_set_api_info_wrap;
	instance.teaisp_bnr_get_model_type = teaisp_bnr_get_model_type_wrap;

	teaisp_reg_helper_instance(&instance);
}

//-----------------------------------------------------------------------------
//  TEAISP
//-----------------------------------------------------------------------------
CVI_S32 CVI_TEAISP_Init(VI_PIPE ViPipe, CVI_S32 maxDev)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	reg_helper_instance();

	g_max_dev = maxDev;

	if (g_max_dev > TEAISP_MAX_TPU_DEV) {
		g_max_dev = TEAISP_MAX_TPU_DEV;
	}

	if (g_max_dev <= 0) {
		g_max_dev = 1;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_TEAISP_GetMaxDev(VI_PIPE ViPipe, CVI_S32 *maxDev)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	*maxDev = g_max_dev;

	return CVI_SUCCESS;
}

