/*
 * Copyright (C) Cvitek Co., Ltd. 2023-2024. All rights reserved.
 *
 * File Name: teaisp_helper.c
 * Description:
 *
 */

#include "isp_debug.h"
#include "isp_defines.h"
#include "teaisp_helper.h"

static struct teaisp_helper_instance *__instance;

void teaisp_reg_helper_instance(struct teaisp_helper_instance *instance)
{
	__instance = instance;
}

CVI_S32 teaisp_bnr_load_model(VI_PIPE ViPipe, const char *path, void **model)
{
	if (__instance) {
		return __instance->teaisp_bnr_load_model(ViPipe, path, model);
	} else {
		return CVI_FAILURE;
	}
}

CVI_S32 teaisp_bnr_unload_model(VI_PIPE ViPipe, void *model)
{
	if (__instance) {
		return __instance->teaisp_bnr_unload_model(ViPipe, model);
	} else {
		return CVI_FAILURE;
	}
}

CVI_S32 teaisp_bnr_set_driver_init(VI_PIPE ViPipe)
{
	if (__instance) {
		return __instance->teaisp_bnr_set_driver_init(ViPipe);
	} else {
		return CVI_FAILURE;
	}
}

CVI_S32 teaisp_bnr_set_driver_deinit(VI_PIPE ViPipe)
{
	if (__instance) {
		return __instance->teaisp_bnr_set_driver_deinit(ViPipe);
	} else {
		return CVI_FAILURE;
	}
}

CVI_S32 teaisp_bnr_set_driver_start(VI_PIPE ViPipe)
{
	if (__instance) {
		return __instance->teaisp_bnr_set_driver_start(ViPipe);
	} else {
		return CVI_FAILURE;
	}
}

CVI_S32 teaisp_bnr_set_driver_stop(VI_PIPE ViPipe)
{
	if (__instance) {
		return __instance->teaisp_bnr_set_driver_stop(ViPipe);
	} else {
		return CVI_FAILURE;
	}
}

CVI_S32 teaisp_bnr_set_api_info(VI_PIPE ViPipe, void *model, void *param, int is_new)
{
	if (__instance) {
		return __instance->teaisp_bnr_set_api_info(ViPipe, model, param, is_new);
	} else {
		return CVI_FAILURE;
	}
}

CVI_S32 teaisp_bnr_get_model_type(VI_PIPE ViPipe, void *model_type)
{
	if (__instance) {
		return __instance->teaisp_bnr_get_model_type(ViPipe, model_type);
	} else {
		return CVI_FAILURE;
	}
}
