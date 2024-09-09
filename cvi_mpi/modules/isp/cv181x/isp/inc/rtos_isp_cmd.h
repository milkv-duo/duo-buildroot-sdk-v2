/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: rtos_isp_cmd.h
 * Description:
 *
 */

#ifndef _RTOS_ISP_CMD_H_
#define _RTOS_ISP_CMD_H_

#include "isp_comm_inc.h"
#include "isp_defines.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define ISP_CMD_OFS 0x10

enum rtos_isp_cmd {
	RTOS_ISP_CMD_BEGIN = ISP_CMD_OFS | 0x0,
	RTOS_ISP_CMD_INIT = RTOS_ISP_CMD_BEGIN,
	RTOS_ISP_CMD_PRE_SOF,
	RTOS_ISP_CMD_PRE_EOF,
	RTOS_ISP_CMD_PRE_FE_SOF,
	RTOS_ISP_CMD_PRE_FE_EOF,
	RTOS_ISP_CMD_PRE_BE_SOF,
	RTOS_ISP_CMD_PRE_BE_EOF,
	RTOS_ISP_CMD_POST_SOF,
	RTOS_ISP_CMD_POST_EOF,
	RTOS_ISP_CMD_POST_DONE_SYNC,
	RTOS_ISP_CMD_DEINIT,
	RTOS_ISP_CMD_SWITCH_TO_MASTER,
	RTOS_ISP_CMD_END
};

struct rtos_isp_cmd_event {
	VI_PIPE ViPipe;
	CVI_U32 frame_idx;
	CVI_BOOL is_slave_done;
};

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif

