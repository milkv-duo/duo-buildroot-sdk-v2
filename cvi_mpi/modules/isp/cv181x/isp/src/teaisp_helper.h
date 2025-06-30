/*
 * Copyright (C) Cvitek Co., Ltd. 2023-2024. All rights reserved.
 *
 * File Name: teaisp_helper.h
 * Description:
 *
 */

#ifndef _TEAISP_HELPER_H_
#define _TEAISP_HELPER_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct teaisp_helper_instance {
	CVI_S32 (*teaisp_bnr_load_model)(VI_PIPE ViPipe, const char *path, void **model);
	CVI_S32 (*teaisp_bnr_unload_model)(VI_PIPE ViPipe, void *model);

	CVI_S32 (*teaisp_bnr_set_driver_init)(VI_PIPE ViPipe);
	CVI_S32 (*teaisp_bnr_set_driver_deinit)(VI_PIPE ViPipe);
	CVI_S32 (*teaisp_bnr_set_driver_start)(VI_PIPE ViPipe);
	CVI_S32 (*teaisp_bnr_set_driver_stop)(VI_PIPE ViPipe);
	CVI_S32 (*teaisp_bnr_set_api_info)(VI_PIPE ViPipe, void *model, void *param, int is_new);
	CVI_S32 (*teaisp_bnr_get_model_type)(VI_PIPE ViPipe, void *model_type);
};

void teaisp_reg_helper_instance(struct teaisp_helper_instance *instance);

CVI_S32 teaisp_bnr_load_model(VI_PIPE ViPipe, const char *path, void **model);
CVI_S32 teaisp_bnr_unload_model(VI_PIPE ViPipe, void *model);

CVI_S32 teaisp_bnr_set_driver_init(VI_PIPE ViPipe);
CVI_S32 teaisp_bnr_set_driver_deinit(VI_PIPE ViPipe);
CVI_S32 teaisp_bnr_set_driver_start(VI_PIPE ViPipe);
CVI_S32 teaisp_bnr_set_driver_stop(VI_PIPE ViPipe);
CVI_S32 teaisp_bnr_set_api_info(VI_PIPE ViPipe, void *model, void *param, int is_new);
CVI_S32 teaisp_bnr_get_model_type(VI_PIPE ViPipe, void *model_type);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _TEAISP_HELPER_H_

