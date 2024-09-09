/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_ipc_local.h
 * Description:
 *
 */

#ifndef _ISP_IPC_LOCAL_H_
#define _ISP_IPC_LOCAL_H_

#include "isp_ipc.h"

#define ISP_RECEIVE_IPC_CMD

#define DEFAULT_ISP_FIFO_IPC_FILENAME		"/tmp/isp_fifo"
#define ENV_ISP_FIFO_IPC_NAME				"ISP_FIFO_PATH"

#define TRIGGER_AE_LOG_CMD					"trigger=aelog"
#define TRIGGER_AWB_LOG_CMD					"trigger=awblog"
#define SET_AE_LOG_LOCATION_CMD				"setaelogpath="
#define SET_AWB_LOG_LOCATION_CMD			"setawblogpath="

#endif // _ISP_IPC_LOCAL_H_
