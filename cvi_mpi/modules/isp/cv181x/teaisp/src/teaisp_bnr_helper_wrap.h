/*
 * Copyright (C) Cvitek Co., Ltd. 2023-2024. All rights reserved.
 *
 * File Name: teaisp_bnr_helper_wrap.h
 * Description:
 *
 */

#ifndef _TEAISP_BNR_HELPER_WRAP_H_
#define _TEAISP_BNR_HELPER_WRAP_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

CVI_S32 teaisp_bnr_load_model_wrap(VI_PIPE ViPipe, const char *path, void **model);
CVI_S32 teaisp_bnr_unload_model_wrap(VI_PIPE ViPipe, void *model);

CVI_S32 teaisp_bnr_set_driver_init_wrap(VI_PIPE ViPipe);
CVI_S32 teaisp_bnr_set_driver_deinit_wrap(VI_PIPE ViPipe);
CVI_S32 teaisp_bnr_set_driver_start_wrap(VI_PIPE ViPipe);
CVI_S32 teaisp_bnr_set_driver_stop_wrap(VI_PIPE ViPipe);
CVI_S32 teaisp_bnr_set_api_info_wrap(VI_PIPE ViPipe, void *model, void *param, int is_new);
CVI_S32 teaisp_bnr_get_model_type_wrap(VI_PIPE ViPipe, void *model_type);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _TEAISP_BNR_HELPER_WRAP_H_

