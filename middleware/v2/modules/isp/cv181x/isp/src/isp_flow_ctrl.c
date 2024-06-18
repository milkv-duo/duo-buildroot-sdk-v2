/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_flow_ctrl.c
 * Description:
 *
 */

#include <stdlib.h>
#include <string.h>
#include "isp_defines.h"
#include "isp_main_local.h"

CVI_S32 isp_flow_event_siganl(VI_PIPE ViPipe, ISP_VD_TYPE_E eventType)
{
#ifndef ARCH_RTOS_CV181X
	ISP_CTX_S *pstIspCtx = NULL;

	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	ISP_GET_CTX(ViPipe, pstIspCtx);

	pthread_mutex_lock(&pstIspCtx->ispEventLock);
	// signal sensor event.
	pthread_cond_signal(&pstIspCtx->ispEventCond[eventType]);
	pthread_mutex_unlock(&pstIspCtx->ispEventLock);
#else
	UNUSED(ViPipe);
	UNUSED(eventType);
#endif

	return CVI_SUCCESS;
}
