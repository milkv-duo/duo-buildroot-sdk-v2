/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_mailbox.h
 * Description:
 *
 */

#ifndef _ISP_MAILBOX_H_
#define _ISP_MAILBOX_H_

#include "cvi_comm_isp.h"
#include "rtos_isp_cmd.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

CVI_S32 isp_mailbox_init(VI_PIPE ViPipe);
CVI_S32 isp_mailbox_uninit(VI_PIPE ViPipe);

CVI_S32 isp_mailbox_send_cmd_init(VI_PIPE ViPipe);
CVI_S32 isp_mailbox_send_cmd_deinit(VI_PIPE ViPipe);
CVI_S32 isp_mailbox_send_cmd_event(VI_PIPE ViPipe, enum rtos_isp_cmd event, CVI_U32 frame_idx);
//CVI_S32 isp_mailbox_send_cmd_sync_post_done(VI_PIPE ViPipe, CVI_U32 frame_idx);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_MAILBOX_H_
