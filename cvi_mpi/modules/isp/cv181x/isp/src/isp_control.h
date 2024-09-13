/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_control.h
 * Description:
 *
 */

#ifndef _ISP_CONTROL_H_
#define _ISP_CONTROL_H_

#include "cvi_comm_isp.h"
#include "isp_main_local.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

CVI_S32 isp_control_set_fd_info(VI_PIPE ViPipe);
CVI_S32 isp_control_set_scene_info(VI_PIPE ViPipe);
CVI_S32 isp_control_get_scene_info(VI_PIPE ViPipe, enum ISP_SCENE_INFO *scene);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_CONTROL_H_
