/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cvi_isp.c
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifndef ARCH_RTOS_CV181X
#include <sys/ioctl.h>
#endif

#ifndef ARCH_RTOS_CV181X
#include "isp_version.h"
//#include "cvi_base.h"
#endif

#include "clog.h"
#include "cvi_isp.h"
#include "cvi_ae.h"
#include "cvi_awb.h"
#include "cvi_comm_isp.h"
#include "isp_main_local.h"
#include "isp_defines.h"
#include "isp_3a.h"
#include "isp_mgr.h"
#include "isp_tun_buf_ctrl.h"

#include "isp_ioctl.h"

#include "isp_ipc_local.h"
#ifndef ARCH_RTOS_CV181X
#include "peri.h"
#else
#include "rtos_isp_mgr.h"
#endif
#include "isp_proc_local.h"
#include "isp_sts_ctrl.h"
#include "isp_freeze.h"

#include "rtos_isp_cmd.h"
#include "isp_mailbox.h"

#include "isp_feature.h"
#include "isp_mgr_buf.h"

CVI_BOOL g_isp_debug_print_mpi = 1;
CVI_BOOL g_isp_debug_diff_only = 1;
CVI_BOOL g_isp_thread_run = CVI_FALSE;

// AE/AWB Debug callback function
ISP_AE_DEBUG_FUNC_S aeAlgoInternalLibReg[VI_MAX_PIPE_NUM] = {
	[0 ... VI_MAX_PIPE_NUM - 1].pfn_ae_dumplog = NULL,
	[0 ... VI_MAX_PIPE_NUM - 1].pfn_ae_setdumplogpath = NULL
};

ISP_AWB_DEBUG_FUNC_S awbAlgoInternalLibReg[VI_MAX_PIPE_NUM] = {
	[0 ... VI_MAX_PIPE_NUM - 1].pfn_awb_dumplog = NULL,
	[0 ... VI_MAX_PIPE_NUM - 1].pfn_awb_setdumplogpath = NULL
};

CVI_S32 CVI_ISP_Init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

#ifdef ARCH_RTOS_CV181X
	ret = rtos_isp_mgr_init(ViPipe);
#else
	ret = isp_mgr_init(ViPipe);
#endif

	return ret;
}

CVI_S32 CVI_ISP_MemInit(VI_PIPE ViPipe)
{
	UNUSED(ViPipe);

	isp_mgr_buf_init(ViPipe);

	return CVI_SUCCESS;
}

#ifdef ISP_RECEIVE_IPC_CMD
static CVI_S32 parse_isp_fifo_path_from_environment(char *fifo_path, CVI_U32 path_length)
{
	char *env = getenv(ENV_ISP_FIFO_IPC_NAME);

	if (env == NULL) {
		snprintf(fifo_path, path_length, DEFAULT_ISP_FIFO_IPC_FILENAME);
		return CVI_FAILURE;
	}

	snprintf(fifo_path, path_length, env);
	return CVI_SUCCESS;
}

static CVI_S32 initial_ipc_fifo(int *fifoFd, const char *fifo_path)
{
	if (access(fifo_path, F_OK) != 0)
		if (mkfifo(fifo_path, 0777) != 0) {
			ISP_LOG_ERR("Create ISP FIFO file fail\n");
			return CVI_FAILURE;
		}

	*fifoFd = open(fifo_path, O_RDWR);
	if (*fifoFd == -1) {
		ISP_LOG_ERR("Open ISP FIFO file fail\n");
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

static CVI_S32 release_ipc_fifo(const char *fifo_path)
{
	if (access(fifo_path, F_OK) == 0)
		if (unlink(fifo_path) != 0) {
			ISP_LOG_ERR("Remove ISP FIFO file fail\n");
			return CVI_FAILURE;
		}

	return CVI_SUCCESS;
}

static CVI_S32 handle_fifo_command(const char *command)
{
	size_t ae_log_location_len = strlen(SET_AE_LOG_LOCATION_CMD);
	size_t awb_log_location_len = strlen(SET_AWB_LOG_LOCATION_CMD);

	if (strcmp(TRIGGER_AE_LOG_CMD, command) == 0) {
		for (int pipeId = 0; pipeId < VI_MAX_PIPE_NUM; ++pipeId) {
			if (aeAlgoInternalLibReg[pipeId].pfn_ae_dumplog != NULL) {
				aeAlgoInternalLibReg[pipeId].pfn_ae_dumplog();
			}
		}
	} else if (strcmp(TRIGGER_AWB_LOG_CMD, command) == 0) {
		for (int pipeId = 0; pipeId < VI_MAX_PIPE_NUM; ++pipeId) {
			if (awbAlgoInternalLibReg[pipeId].pfn_awb_dumplog != NULL) {
				awbAlgoInternalLibReg[pipeId].pfn_awb_dumplog(pipeId);
			}
		}
	} else if (strncmp(SET_AE_LOG_LOCATION_CMD, command, ae_log_location_len) == 0) {
		for (CVI_U32 u32PipeID = 0; u32PipeID < VI_MAX_PIPE_NUM; ++u32PipeID) {
			if (aeAlgoInternalLibReg[u32PipeID].pfn_ae_dumplog != NULL) {
				aeAlgoInternalLibReg[u32PipeID].pfn_ae_setdumplogpath(command + ae_log_location_len);
			}
		}
	} else if (strncmp(SET_AWB_LOG_LOCATION_CMD, command, awb_log_location_len) == 0) {
		for (CVI_U32 u32PipeID = 0; u32PipeID < VI_MAX_PIPE_NUM; ++u32PipeID) {
			if (awbAlgoInternalLibReg[u32PipeID].pfn_awb_dumplog != NULL) {
				awbAlgoInternalLibReg[u32PipeID].pfn_awb_setdumplogpath(command + awb_log_location_len);
			}
		}
	}

	return CVI_SUCCESS;
}
#endif // ISP_RECEIVE_IPC_CMD

CVI_S32 CVI_ISP_AELibRegInternalCallBack(VI_PIPE ViPipe, ISP_AE_DEBUG_FUNC_S *pstRegister)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstRegister == CVI_NULL) {
		return CVI_FAILURE;
	}

	aeAlgoInternalLibReg[ViPipe].pfn_ae_dumplog = pstRegister->pfn_ae_dumplog;
	aeAlgoInternalLibReg[ViPipe].pfn_ae_setdumplogpath = pstRegister->pfn_ae_setdumplogpath;

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_AELibUnRegInternalCallBack(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	aeAlgoInternalLibReg[ViPipe].pfn_ae_dumplog = NULL;
	aeAlgoInternalLibReg[ViPipe].pfn_ae_setdumplogpath = NULL;

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_AWBLibRegInternalCallBack(VI_PIPE ViPipe, ISP_AWB_DEBUG_FUNC_S *pstRegister)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstRegister == CVI_NULL) {
		return CVI_FAILURE;
	}

	awbAlgoInternalLibReg[ViPipe].pfn_awb_dumplog = pstRegister->pfn_awb_dumplog;
	awbAlgoInternalLibReg[ViPipe].pfn_awb_setdumplogpath = pstRegister->pfn_awb_setdumplogpath;

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_AWBLibUnRegInternalCallBack(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	awbAlgoInternalLibReg[ViPipe].pfn_awb_dumplog = NULL;
	awbAlgoInternalLibReg[ViPipe].pfn_awb_setdumplogpath = NULL;

	return CVI_SUCCESS;
}

#ifndef ARCH_RTOS_CV181X
CVI_S32 CVI_ISP_Run(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CHECK_DEV_VALUE(ViPipe);

	ISP_CTX_S *pstIspCtx = NULL;

	ISP_GET_CTX(ViPipe, pstIspCtx);
	CVI_S32 fd = pstIspCtx->ispDevFd;

	if (ViPipe > 0) {
		ISP_LOG_DEBUG("ISP Dev %d return\n", ViPipe);
		ISP_LOG_DEBUG("No matter how many sns there are, only one thread runs\n");
		return CVI_SUCCESS;
	}

	if (fd == -1) {
		ISP_LOG_ASSERT("ISP%d fd state incorrect\n", fd);
		return -EBADF;
	}

	pstIspCtx->bIspRun = CVI_TRUE;
	//isp_snsSync_cfg_set(ViPipe, 0);
	fd_set efds, readfds;
	struct timeval tv;
	CVI_S32 r;

#ifdef ISP_RECEIVE_IPC_CMD
	int ipc_fifo_fd = 0;
	char fifo_path[128] = {'\0'};
	FILE *fFifo = NULL;

	parse_isp_fifo_path_from_environment(fifo_path, 128);

	if (initial_ipc_fifo(&ipc_fifo_fd, fifo_path) != CVI_SUCCESS)
		ISP_LOG_ERR("Enable ISP ipc fail\n");

	if (ipc_fifo_fd)
		fFifo = fdopen(ipc_fifo_fd, "r");
#endif // ISP_RECEIVE_IPC_CMD
#ifdef ENABLE_ISP_PROC_DEBUG
	isp_proc_run();
#endif

	g_isp_thread_run = CVI_TRUE;
	/* Timeout. */
	while (g_isp_thread_run) {
		int max_fd = fd;

		FD_ZERO(&efds);
		FD_ZERO(&readfds);
		FD_SET(fd, &efds);

#ifdef ISP_RECEIVE_IPC_CMD
		if (ipc_fifo_fd > 0) {
			FD_SET(ipc_fifo_fd, &readfds);
			max_fd = MAX(max_fd, ipc_fifo_fd);
		}
#endif // ISP_RECEIVE_IPC_CMD

		tv.tv_sec = 0;
		tv.tv_usec = 500 * 1000;
		r = select(max_fd + 1, &readfds, NULL, &efds, &tv);
		if (r == -1) {
			if (errno == EINTR)
				continue;
			//errno_exit("select");
			continue;
		}
		if (r == 0) {
			//fprintf(stderr, "%s: select timeout\n", __func__);
			continue;
		}

		if (FD_ISSET(fd, &efds)) {
			struct vi_ext_control ec1;
			struct vi_event ev;

			memset(&ec1, 0, sizeof(ec1));
			memset(&ev, 0, sizeof(ev));

			ec1.id = VI_IOCTL_DQEVENT;
			ec1.ptr = (void *)&ev;
			if (ioctl(fd, VI_IOC_G_CTRL, &ec1) < 0) {
				fprintf(stderr, "VI_IOC_G_CTRL - %s NG\n", __func__);
				break;
			}

			switch (ev.type) {
			case VI_EVENT_ISP_PROC_READ:
			{
#ifdef ENABLE_ISP_PROC_DEBUG
				ISP_LOG_DEBUG("VI_EVENT_ISP_PROC_READ\n");
				if (isp_proc_getProcStatus() == PROC_STATUS_READY) {
					isp_proc_setProcStatus(PROC_STATUS_RECEVENT);
				}
#endif
				break;
			}
			case VI_EVENT_PRE0_SOF:
				ISP_LOG_INFO("pipe 0 sof+++,%d\n", ev.frame_sequence);
				isp_mgr_pre_sof(0, ev.frame_sequence);
				ISP_LOG_INFO("pipe 0 sof---\n");
				break;
			case VI_EVENT_PRE1_SOF:
				ISP_LOG_INFO("pipe 1 sof+++,%d\n", ev.frame_sequence);
				isp_mgr_pre_sof(1, ev.frame_sequence);
				ISP_LOG_INFO("pipe 1 sof---\n");
				break;
			case VI_EVENT_PRE2_SOF:
				ISP_LOG_INFO("pipe 2 sof+++,%d\n", ev.frame_sequence);
				isp_mgr_pre_sof(2, ev.frame_sequence);
				ISP_LOG_INFO("pipe 2 sof---\n");
				break;
			case VI_EVENT_VIRT_PRE3_SOF:
				ISP_LOG_INFO("virt pipe 3 sof+++,%d\n", ev.frame_sequence);
				isp_mgr_pre_sof(3, ev.frame_sequence);
				ISP_LOG_INFO("virt pipe 3 sof---\n");
				break;
			case VI_EVENT_PRE0_EOF:
				ISP_LOG_INFO("pipe 0 be eof+++,%d\n", ev.frame_sequence);
				isp_mgr_pre_eof(0, ev.frame_sequence);
				ISP_LOG_INFO("pipe 0 be eof---\n");
				break;
			case VI_EVENT_PRE1_EOF:
				ISP_LOG_INFO("pipe 1 be eof+++,%d\n", ev.frame_sequence);
				isp_mgr_pre_eof(1, ev.frame_sequence);
				ISP_LOG_INFO("pipe 1 be eof---\n");
				break;
			case VI_EVENT_PRE2_EOF:
				ISP_LOG_INFO("pipe 2 be eof+++,%d\n", ev.frame_sequence);
				isp_mgr_pre_eof(2, ev.frame_sequence);
				ISP_LOG_INFO("pipe 2 be eof---\n");
				break;
			case VI_EVENT_VIRT_PRE3_EOF:
				ISP_LOG_INFO("virt pipe 3 be eof+++,%d\n", ev.frame_sequence);
				isp_mgr_pre_eof(3, ev.frame_sequence);
				ISP_LOG_INFO("virt pipe 3 be eof---\n");
				break;
			case VI_EVENT_POST_EOF:
				ISP_LOG_INFO("pipe 0 post eof+++,%d\n", ev.frame_sequence);
				isp_mgr_post_eof(0, ev.frame_sequence);
				ISP_LOG_INFO("pipe 0 post eof---\n");
				break;
			case VI_EVENT_POST1_EOF:
				ISP_LOG_INFO("pipe 1 post eof+++,%d\n", ev.frame_sequence);
				isp_mgr_post_eof(1, ev.frame_sequence);
				ISP_LOG_INFO("pipe 1 post eof---\n");
				break;
			case VI_EVENT_POST2_EOF:
				ISP_LOG_INFO("pipe 2 post eof+++,%d\n", ev.frame_sequence);
				isp_mgr_post_eof(2, ev.frame_sequence);
				ISP_LOG_INFO("pipe 2 post eof---\n");
				break;
			case VI_EVENT_VIRT_POST3_EOF:
				ISP_LOG_INFO("virt pipe 3 post eof+++,%d\n", ev.frame_sequence);
				isp_mgr_post_eof(3, ev.frame_sequence);
				ISP_LOG_INFO("virt pipe 3 post eof---\n");
				break;
			}
		}

#ifdef ISP_RECEIVE_IPC_CMD
		if (ipc_fifo_fd > 0)
			if (FD_ISSET(ipc_fifo_fd, &readfds)) {
				char szCmdCommand[128] = {'\0'};

				fscanf(fFifo, "%s", szCmdCommand);
				// printf("fifo ipc command : %s\n", szCmdCommand);

				handle_fifo_command(szCmdCommand);
			}
#endif // ISP_RECEIVE_IPC_CMD
	}

	ISP_LOG_DEBUG("End\n");

#ifdef ISP_RECEIVE_IPC_CMD
	if (fFifo)
		fclose(fFifo);

	release_ipc_fifo(fifo_path);
#endif // ISP_RECEIVE_IPC_CMD

	pstIspCtx->bIspRun = CVI_FALSE;

	return CVI_SUCCESS;
}
#else
CVI_S32 CVI_ISP_Run(VI_PIPE ViPipe)
{
	ISP_LOG_WARNING("+\n");

	UNUSED(ViPipe);

	return CVI_SUCCESS;
}
#endif

CVI_S32 CVI_ISP_RunOnce(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CHECK_DEV_VALUE(ViPipe);

	// TODO: implementation
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_Exit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

#ifndef ARCH_RTOS_CV181X
	g_isp_thread_run = CVI_FALSE;
	ret = isp_mgr_uninit(ViPipe);
	isp_mgr_buf_uninit(ViPipe);
#else
	ret = rtos_isp_mgr_uninit(ViPipe);
#endif

	return ret;
}

CVI_S32 CVI_ISP_SetPubAttr(VI_PIPE ViPipe, const ISP_PUB_ATTR_S *pstPubAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstPubAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	ISP_CTX_S *pstIspCtx = NULL;

	ISP_GET_CTX(ViPipe, pstIspCtx);

	ISP_LOG_INFO("fps %f -> %f\n",
		pstIspCtx->stSnsImageMode.f32Fps,
		pstPubAttr->f32FrameRate);

	pstIspCtx->enBayer = pstPubAttr->enBayer;
	pstIspCtx->u8SnsWDRMode = pstPubAttr->enWDRMode;
	pstIspCtx->stSysRect.s32X = pstPubAttr->stWndRect.s32X;
	pstIspCtx->stSysRect.s32Y = pstPubAttr->stWndRect.s32Y;
	pstIspCtx->stSysRect.u32Width = pstPubAttr->stWndRect.u32Width;
	pstIspCtx->stSysRect.u32Height = pstPubAttr->stWndRect.u32Height;
	pstIspCtx->stSnsImageMode.f32Fps = pstPubAttr->f32FrameRate;
	pstIspCtx->stSnsImageMode.u16Width = pstPubAttr->stSnsSize.u32Width;
	pstIspCtx->stSnsImageMode.u16Height = pstPubAttr->stSnsSize.u32Height;
	pstIspCtx->stSnsImageMode.u8SnsMode = pstPubAttr->u8SnsMode;

	ISP_LOG_INFO("set pub attr, b:%d,wm:%d,(%d,%d,%d,%d),fps:%f,(%d,%d),sm:%d\n",
		pstPubAttr->enBayer,
		pstPubAttr->enWDRMode,
		pstPubAttr->stWndRect.s32X,
		pstPubAttr->stWndRect.s32Y,
		pstPubAttr->stWndRect.u32Width,
		pstPubAttr->stWndRect.u32Height,
		pstPubAttr->f32FrameRate,
		pstPubAttr->stSnsSize.u32Width,
		pstPubAttr->stSnsSize.u32Height,
		pstPubAttr->u8SnsMode);

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetPubAttr(VI_PIPE ViPipe, ISP_PUB_ATTR_S *pstPubAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstPubAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	ISP_CTX_S *pstIspCtx = NULL;

	ISP_GET_CTX(ViPipe, pstIspCtx);

	pstPubAttr->enBayer = pstIspCtx->enBayer;
	pstPubAttr->enWDRMode = pstIspCtx->u8SnsWDRMode;
	pstPubAttr->stSnsSize.u32Width = pstIspCtx->stSnsImageMode.u16Width;
	pstPubAttr->stSnsSize.u32Height = pstIspCtx->stSnsImageMode.u16Height;
	pstPubAttr->stWndRect.s32X = pstIspCtx->stSysRect.s32X;
	pstPubAttr->stWndRect.s32Y = pstIspCtx->stSysRect.s32Y;
	pstPubAttr->stWndRect.u32Width = pstIspCtx->stSysRect.u32Width;
	pstPubAttr->stWndRect.u32Height = pstIspCtx->stSysRect.u32Height;
	pstPubAttr->f32FrameRate = pstIspCtx->stSnsImageMode.f32Fps;
	pstPubAttr->u8SnsMode = pstIspCtx->stSnsImageMode.u8SnsMode;

	return CVI_SUCCESS;
}

/*TODO. Ctrl Param still need to be implement*/
CVI_S32 CVI_ISP_SetCtrlParam(VI_PIPE ViPipe, const ISP_CTRL_PARAM_S *pstIspCtrlParam)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstIspCtrlParam == CVI_NULL) {
		return CVI_FAILURE;
	}

	ISP_CTX_S *pstIspCtx = NULL;

	ISP_GET_CTX(ViPipe, pstIspCtx);

	pstIspCtx->ispProcCtrl.u32ProcParam = pstIspCtrlParam->u32ProcParam;
	pstIspCtx->ispProcCtrl.u32ProcLevel = pstIspCtrlParam->u32ProcLevel;
	pstIspCtx->ispProcCtrl.u32AEStatIntvl = pstIspCtrlParam->u32AEStatIntvl;
	pstIspCtx->ispProcCtrl.u32AWBStatIntvl = pstIspCtrlParam->u32AWBStatIntvl;
	pstIspCtx->ispProcCtrl.u32AFStatIntvl = pstIspCtrlParam->u32AFStatIntvl;
	pstIspCtx->ispProcCtrl.u32UpdatePos = pstIspCtrlParam->u32UpdatePos;
	pstIspCtx->ispProcCtrl.u32IntTimeOut = pstIspCtrlParam->u32IntTimeOut;
	pstIspCtx->ispProcCtrl.u32PwmNumber = pstIspCtrlParam->u32PwmNumber;
	pstIspCtx->ispProcCtrl.u32PortIntDelay = pstIspCtrlParam->u32PortIntDelay;

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetCtrlParam(VI_PIPE ViPipe, ISP_CTRL_PARAM_S *pstIspCtrlParam)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstIspCtrlParam == CVI_NULL) {
		return CVI_FAILURE;
	}

	ISP_CTX_S *pstIspCtx = NULL;

	ISP_GET_CTX(ViPipe, pstIspCtx);

	pstIspCtrlParam->u32ProcParam = pstIspCtx->ispProcCtrl.u32ProcParam;
	pstIspCtrlParam->u32ProcLevel = pstIspCtx->ispProcCtrl.u32ProcLevel;
	pstIspCtrlParam->u32AEStatIntvl = pstIspCtx->ispProcCtrl.u32AEStatIntvl;
	pstIspCtrlParam->u32AWBStatIntvl = pstIspCtx->ispProcCtrl.u32AWBStatIntvl;
	pstIspCtrlParam->u32AFStatIntvl = pstIspCtx->ispProcCtrl.u32AFStatIntvl;
	pstIspCtrlParam->u32UpdatePos = pstIspCtx->ispProcCtrl.u32UpdatePos;
	pstIspCtrlParam->u32IntTimeOut = pstIspCtx->ispProcCtrl.u32IntTimeOut;
	pstIspCtrlParam->u32PwmNumber = pstIspCtx->ispProcCtrl.u32PwmNumber;
	pstIspCtrlParam->u32PortIntDelay = pstIspCtx->ispProcCtrl.u32PortIntDelay;

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_SetFMWState(VI_PIPE ViPipe, const ISP_FMW_STATE_E enState)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	isp_freeze_set_fw_state(ViPipe, enState);

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetFMWState(VI_PIPE ViPipe, ISP_FMW_STATE_E *penState)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (penState == CVI_NULL) {
		return CVI_FAILURE;
	}

	isp_freeze_get_fw_state(ViPipe, penState);

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_SetModuleControl(VI_PIPE ViPipe, const ISP_MODULE_CTRL_U *punModCtrl)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (punModCtrl == CVI_NULL) {
		return CVI_FAILURE;
	}

	ISP_MODULE_CTRL_U tmpModCtrl = *punModCtrl;

	isp_feature_ctrl_set_module_ctrl(ViPipe, &tmpModCtrl);

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetModuleControl(VI_PIPE ViPipe, ISP_MODULE_CTRL_U *punModCtrl)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (punModCtrl == CVI_NULL) {
		return CVI_FAILURE;
	}

	isp_feature_ctrl_get_module_ctrl(ViPipe, punModCtrl);

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_SetBindAttr(VI_PIPE ViPipe, const ISP_BIND_ATTR_S *pstBindAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstBindAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret;
	CVI_U32 i;
	ISP_CTX_S *pstIspCtx = NULL;
	ALG_LIB_S bindInfo[AAA_TYPE_MAX] = {
		pstBindAttr->stAeLib,
		pstBindAttr->stAwbLib,
		pstBindAttr->stAfLib};

	ISP_GET_CTX(ViPipe, pstIspCtx);
	for (i = 0; i < AAA_TYPE_MAX; i++) {

		if ((bindInfo[i].s32Id != -1) && (strlen(bindInfo[i].acLibName) != 0)) {
			ret = isp_3aLib_find(ViPipe, &(bindInfo[i]), i);
			// When can't find 3a register lib. Only show message for remind.
			if (ret != ISP_3ALIB_FIND_FAIL) {
				pstIspCtx->activeLibIdx[i] = ret;
			} else {
				ISP_LOG_DEBUG("Find type %u Lib fail. Bind Lib NG %d\n", i, ret);
			}
		}
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetBindAttr(VI_PIPE ViPipe, ISP_BIND_ATTR_S *pstBindAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstBindAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	isp_3aLib_get(ViPipe, &(pstBindAttr->stAeLib), AAA_TYPE_AE);
	isp_3aLib_get(ViPipe, &(pstBindAttr->stAwbLib), AAA_TYPE_AWB);
	isp_3aLib_get(ViPipe, &(pstBindAttr->stAfLib), AAA_TYPE_AF);

	ISP_LOG_DEBUG("pstBindAttr->stAeLib %s %d\n"
		, pstBindAttr->stAeLib.acLibName,  pstBindAttr->stAeLib.s32Id);
	ISP_LOG_DEBUG("pstBindAttr->stAwbLib %s %d\n"
		, pstBindAttr->stAwbLib.acLibName,  pstBindAttr->stAwbLib.s32Id);
	ISP_LOG_DEBUG("pstBindAttr->stAfLib %s %d\n"
		, pstBindAttr->stAfLib.acLibName,  pstBindAttr->stAfLib.s32Id);

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetVDTimeOut(VI_PIPE ViPipe, ISP_VD_TYPE_E enIspVDType, CVI_U32 u32MilliSec)
{
#ifndef ARCH_RTOS_CV181X
#define SEC_TO_USEC 1000000
#define MSEC_TO_USEC 1000
#define USEC_TO_NSEC 1000
	ISP_CTX_S *pstIspCtx = NULL;
	CVI_S32 s32Ret;
	CVI_U32 timeoutSec;
	CVI_U32 timeoutUSec;
	struct timeval currentTime;
	struct timespec outtime;

	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	ISP_GET_CTX(ViPipe, pstIspCtx);

	pthread_mutex_lock(&pstIspCtx->ispEventLock);
	// wait event timeout.
	// When time out set to 0. Wait forever.
	if (u32MilliSec == 0) {
		s32Ret = pthread_cond_wait(&pstIspCtx->ispEventCond[enIspVDType], &pstIspCtx->ispEventLock);
	} else {
		gettimeofday(&currentTime, NULL);
		// calculate timeout system time.
		timeoutSec = (currentTime.tv_usec + u32MilliSec * MSEC_TO_USEC) / SEC_TO_USEC;
		timeoutUSec = (currentTime.tv_usec + u32MilliSec * MSEC_TO_USEC) - (timeoutSec * SEC_TO_USEC);

		outtime.tv_sec = currentTime.tv_sec + timeoutSec;
		outtime.tv_nsec = timeoutUSec * USEC_TO_NSEC;

		s32Ret = pthread_cond_timedwait(&pstIspCtx->ispEventCond[enIspVDType],
			&pstIspCtx->ispEventLock, &outtime);
	}
	if (s32Ret != 0) {
		ISP_LOG_ERR("ViPipe %d Type %d wait time out with %#x\n", ViPipe, enIspVDType, s32Ret);
		pthread_mutex_unlock(&pstIspCtx->ispEventLock);
		return s32Ret;
	}
	pthread_mutex_unlock(&pstIspCtx->ispEventLock);
#else
	UNUSED(ViPipe);
	UNUSED(enIspVDType);
	UNUSED(u32MilliSec);
#endif

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_SensorRegCallBack(VI_PIPE ViPipe, ISP_SNS_ATTR_INFO_S *pstSnsAttrInfo,
						ISP_SENSOR_REGISTER_S *pstRegister)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstSnsAttrInfo == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (pstRegister == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 s32Ret;
	ISP_CTX_S *pstIspCtx = CVI_NULL;

#if 0
	if (pstRegister->stSnsExp.pfn_cmos_sensor_init == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (pstRegister->stSnsExp.pfn_cmos_sensor_exit == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (pstRegister->stSnsExp.pfn_cmos_sensor_global_init == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (pstRegister->stSnsExp.pfn_cmos_set_image_mode == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (pstRegister->stSnsExp.pfn_cmos_set_wdr_mode == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (pstRegister->stSnsExp.pfn_cmos_get_isp_default == CVI_NULL) {
		return CVI_FAILURE;
	}

	//if (pstRegister->stSnsExp.pfn_cmos_get_isp_black_level == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (pstRegister->stSnsExp.pfn_cmos_get_sns_reg_info == CVI_NULL) {
		return CVI_FAILURE;
	}

	//if (pstRegister->stSnsExp.pfn_cmos_set_pixel_detect == CVI_NULL) {
		return CVI_FAILURE;
	}

#endif
	ISP_GET_CTX(ViPipe, pstIspCtx);
	if (pstIspCtx == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (pstIspCtx->bSnsReg == CVI_TRUE)
		return CVI_FAILURE;

	s32Ret = isp_sensor_register(ViPipe, pstSnsAttrInfo, pstRegister);
	if (s32Ret != CVI_SUCCESS)
		return s32Ret;

	pstIspCtx->stBindAttr.sensorId = pstSnsAttrInfo->eSensorId;
	pstIspCtx->bSnsReg = CVI_TRUE;

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_SensorUnRegCallBack(VI_PIPE ViPipe, SENSOR_ID SensorId)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	SENSOR_ID sensorId = 0;
	CVI_S32 s32Ret;
	ISP_CTX_S *pstIspCtx = CVI_NULL;

	if (g_astIspCtx[ViPipe] != CVI_NULL) {
		ISP_GET_CTX(ViPipe, pstIspCtx);
	}

	isp_sensor_getId(ViPipe, &sensorId);

	if (pstIspCtx != CVI_NULL) {
		if (pstIspCtx->stBindAttr.sensorId != sensorId) {
			ISP_LOG_ERR("ViPipe %d current sensor is %d, unregister sensor is %d\n",
				ViPipe, pstIspCtx->stBindAttr.sensorId, sensorId);
			return -EINVAL;
		}
	}

	s32Ret = isp_sensor_unRegister(ViPipe);

	if (pstIspCtx != CVI_NULL) {
		pstIspCtx->bSnsReg = CVI_FALSE;
	}

	UNUSED(SensorId);

	return s32Ret;
}

CVI_S32 CVI_ISP_AELibRegCallBack(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib, ISP_AE_REGISTER_S *pstRegister)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstAeLib == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (pstRegister == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 aelib_index;
	CVI_S32 ret = CVI_SUCCESS;

	aelib_index = isp_3aLib_find(ViPipe, pstAeLib, AAA_TYPE_AE);
	if (aelib_index != ISP_3ALIB_FIND_FAIL) {
		ISP_LOG_ERR("AE Lib already be registered\n");
		return -EINVAL;
	}

	ret = isp_3aLib_reg(ViPipe, pstAeLib, &(pstRegister->stAeExpFunc), AAA_TYPE_AE);
	if (ret != CVI_SUCCESS) {
		ISP_LOG_ERR("AE Lib register fail\n");
		isp_reg_lib_dump();
	}

	return ret;
}

CVI_S32 CVI_ISP_AWBLibRegCallBack(VI_PIPE ViPipe, ALG_LIB_S *pstAwbLib, ISP_AWB_REGISTER_S *pstRegister)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstAwbLib == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (pstRegister == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 awblib_index;
	CVI_S32 ret = CVI_SUCCESS;

	awblib_index = isp_3aLib_find(ViPipe, pstAwbLib, AAA_TYPE_AWB);
	if (awblib_index != ISP_3ALIB_FIND_FAIL) {
		ISP_LOG_ERR("AWB Lib already be registered\n");
		return -EINVAL;
	}

	ret = isp_3aLib_reg(ViPipe, pstAwbLib, &(pstRegister->stAwbExpFunc), AAA_TYPE_AWB);
	if (ret != CVI_SUCCESS) {
		ISP_LOG_ERR("AWB Lib register fail\n");
		isp_reg_lib_dump();
	}

	return ret;
}

CVI_S32 CVI_ISP_AFLibRegCallBack(VI_PIPE ViPipe, ALG_LIB_S *pstAfLib, ISP_AF_REGISTER_S *pstRegister)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstAfLib == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (pstRegister == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 aflib_index;
	CVI_S32 ret = CVI_SUCCESS;

	aflib_index = isp_3aLib_find(ViPipe, pstAfLib, AAA_TYPE_AF);
	if (aflib_index != ISP_3ALIB_FIND_FAIL) {
		ISP_LOG_ERR("AF Lib already be registered\n");
		return -EINVAL;
	}

	ret = isp_3aLib_reg(ViPipe, pstAfLib, &(pstRegister->stAfExpFunc), AAA_TYPE_AF);
	if (ret != CVI_SUCCESS) {
		ISP_LOG_ERR("AF Lib register fail\n");
		isp_reg_lib_dump();
	}

	return ret;
}

CVI_S32 CVI_ISP_AELibUnRegCallBack(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstAeLib == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (isp_3aLib_unreg(ViPipe, pstAeLib, AAA_TYPE_AE) != CVI_SUCCESS) {
		ISP_LOG_ERR("AE Lib unregister fail\n");
		isp_reg_lib_dump();
		return -EINVAL;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_AWBLibUnRegCallBack(VI_PIPE ViPipe, ALG_LIB_S *pstAwbLib)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstAwbLib == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (isp_3aLib_unreg(ViPipe, pstAwbLib, AAA_TYPE_AWB) != CVI_SUCCESS) {
		ISP_LOG_ERR("AWB Lib unregister fail\n");
		isp_reg_lib_dump();
		return -EINVAL;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_AFLibUnRegCallBack(VI_PIPE ViPipe, ALG_LIB_S *pstAfLib)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstAfLib == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (isp_3aLib_unreg(ViPipe, pstAfLib, AAA_TYPE_AF) != CVI_SUCCESS) {
		ISP_LOG_ERR("AF Lib unregister fail\n");
		isp_reg_lib_dump();
		return -EINVAL;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetAEStatistics(VI_PIPE ViPipe, ISP_AE_STATISTICS_S *pstAeStat)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstAeStat == CVI_NULL) {
		return CVI_FAILURE;
	}

	ISP_CTX_S *pstIspCtx = CVI_NULL;

	ISP_GET_CTX(ViPipe, pstIspCtx);
	ISP_STATISTICS_CTRL_U unStatKey = pstIspCtx->stsCfgInfo.unKey;

	ISP_AE_STATISTICS_COMPAT_S *ae_sts;

	/*TODO@CF. Need lock when get statistic data value.*/
	isp_sts_ctrl_get_ae_sts(ViPipe, &ae_sts);

	unStatKey = pstIspCtx->stsCfgInfo.unKey;
	/*TODO@CF. Need add sts enable check here.*/
	for (CVI_U32 i = 0; i < ISP_CHANNEL_MAX_NUM; i++) {
		for (CVI_U32 j = 0; j < AE_MAX_NUM; j++) {
			for (CVI_U32 m = 0; m < MAX_HIST_BINS; m++)
				pstAeStat->au32FEHist1024Value[i][j][m] =
					ae_sts->aeStat1[j].au32HistogramMemArray[i][m];
			// Give global data only when Stat enable.
			if (unStatKey.bit1FEAeGloStat) {
				pstAeStat->au16FEGlobalAvg[i][j][0] = ae_sts->aeStat2[j].u16GlobalAvgB[i];
				pstAeStat->au16FEGlobalAvg[i][j][1] = ae_sts->aeStat2[j].u16GlobalAvgGr[i];
				pstAeStat->au16FEGlobalAvg[i][j][2] = ae_sts->aeStat2[j].u16GlobalAvgGb[i];
				pstAeStat->au16FEGlobalAvg[i][j][3] = ae_sts->aeStat2[j].u16GlobalAvgR[i];
			}
			// Give local data only when Stat enable.
			if (unStatKey.bit1FEAeLocStat) {
				for (CVI_U32 col = 0; col < AE_ZONE_COLUMN; col++) {
					for (CVI_U32 row = 0; row < AE_ZONE_ROW; row++) {
						pstAeStat->au16FEZoneAvg[i][j][row][col][0] =
							ae_sts->aeStat3[j].au16ZoneAvg[i][row][col][0];
						pstAeStat->au16FEZoneAvg[i][j][row][col][1] =
							ae_sts->aeStat3[j].au16ZoneAvg[i][row][col][1];
						pstAeStat->au16FEZoneAvg[i][j][row][col][2] =
							ae_sts->aeStat3[j].au16ZoneAvg[i][row][col][2];
						pstAeStat->au16FEZoneAvg[i][j][row][col][3] =
							ae_sts->aeStat3[j].au16ZoneAvg[i][row][col][3];
					}
				}
			}
		}
	}
#if 0
	for (i = 0; i < ISP_CHANNEL_MAX_NUM; i++) {
		for (j = 0; j < MAX_HIST_BINS; j++) {
			printf("pstAeStat->au32FEHist1024Value[%d][%d] %d\n", i, j,
				pstAeStat->au32FEHist1024Value[i][j]);
		}
		for (j = 0; j < AE_MAX_NUM; j++) {
			printf("pstAeStat->au16FEGlobalAvg[%d][%d][0] %d %d %d %d\n", i, j,
				pstAeStat->au16FEGlobalAvg[i][j][0], pstAeStat->au16FEGlobalAvg[i][j][1],
				pstAeStat->au16FEGlobalAvg[i][j][2], pstAeStat->au16FEGlobalAvg[i][j][3]);
			for (l = 0; l < AE_ZONE_COL; l++) {
				for (k = 0; k < AE_ZONE_ROW; k++) {
					printf("pstAeStat->au16FEZoneAvg[%d][%d][%d][0] %d %d %d %d\n", i, k, l,
						pstAeStat->au16FEZoneAvg[i][j][k][l][0],
						pstAeStat->au16FEZoneAvg[i][j][k][l][1],
						pstAeStat->au16FEZoneAvg[i][j][k][l][2],
						pstAeStat->au16FEZoneAvg[i][j][k][l][3]);
				}
			}
		}
	}
#endif
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetWBStatistics(VI_PIPE ViPipe, ISP_WB_STATISTICS_S *pstAwbStat)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstAwbStat == CVI_NULL) {
		return CVI_FAILURE;
	}

	ISP_CTX_S *pstIspCtx = CVI_NULL;

	ISP_GET_CTX(ViPipe, pstIspCtx);
	ISP_STATISTICS_CTRL_U unStatKey = pstIspCtx->stsCfgInfo.unKey;

	/*TODO@CF. Need lock when get statistic data value.*/
	ISP_WB_STATISTICS_S *awb_sts;

	isp_sts_ctrl_get_awb_sts(ViPipe, ISP_CHANNEL_LE, &awb_sts);

	// HISI only has long exposure data. So only return LE now.
	// Global AWB info enable.
	if (unStatKey.bit1AwbStat1) {
		pstAwbStat->u16GlobalB = awb_sts->u16GlobalB;
		pstAwbStat->u16GlobalG = awb_sts->u16GlobalG;
		pstAwbStat->u16GlobalR = awb_sts->u16GlobalR;
		pstAwbStat->u16CountAll = 0xFFFF;
	}

	// Local AWB info enable.
	if (unStatKey.bit1AwbStat2) {
		for (CVI_U32 i = 0; i < AWB_ZONE_NUM; i++) {
			pstAwbStat->au16ZoneAvgB[i] = awb_sts->au16ZoneAvgB[i];
			pstAwbStat->au16ZoneAvgG[i] = awb_sts->au16ZoneAvgG[i];
			pstAwbStat->au16ZoneAvgR[i] = awb_sts->au16ZoneAvgR[i];
			pstAwbStat->au16ZoneCountAll[i] = 0xFFFF;
		}
	}
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetFocusStatistics(VI_PIPE ViPipe, ISP_AF_STATISTICS_S *pstAfStat)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstAfStat == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_U32 row, col;
	CVI_U32 afWinXNum = AF_ZONE_COLUMN, afWinYNum = AF_ZONE_ROW;
	ISP_CTX_S *pstIspCtx = CVI_NULL;
	ISP_STATISTICS_CTRL_U unStatKey;

	/*TODO@CF. Need lock when get statistic data value.*/
	ISP_GET_CTX(ViPipe, pstIspCtx);

	unStatKey = pstIspCtx->stsCfgInfo.unKey;

	ISP_AF_STATISTICS_S *af_sts;

	isp_sts_ctrl_get_af_sts(ViPipe, &af_sts);

	if (unStatKey.bit1FEAfStat) {
		for (row = 0; row < afWinYNum; row++) {
			for (col = 0; col < afWinXNum; col++) {
				pstAfStat->stFEAFStat.stZoneMetrics[row][col].u16HlCnt =
					af_sts->stFEAFStat.stZoneMetrics[row][col].u16HlCnt;
				pstAfStat->stFEAFStat.stZoneMetrics[row][col].u32v0 =
					af_sts->stFEAFStat.stZoneMetrics[row][col].u32v0;
				pstAfStat->stFEAFStat.stZoneMetrics[row][col].u64h0 =
					af_sts->stFEAFStat.stZoneMetrics[row][col].u64h0;
				pstAfStat->stFEAFStat.stZoneMetrics[row][col].u64h1 =
					af_sts->stFEAFStat.stZoneMetrics[row][col].u64h1;
			}
		}
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetLightboxGain(VI_PIPE ViPipe, ISP_AWB_LightBox_Gain_S *pstAWBLightBoxGain)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstAWBLightBoxGain == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_U32 rSum = 0, bSum = 0, gSum = 0;
	CVI_U32 col, row, index = 0;
	CVI_U32 colStart, colEnd;
	CVI_U32 rowStart, rowEnd;
	CVI_S32 s32Ret;
	ISP_WB_STATISTICS_S *pstWBStat = NULL;

	pstWBStat = ISP_MALLOC(sizeof(ISP_WB_STATISTICS_S));
	if (pstWBStat == NULL) {
		ISP_LOG_ERR("%s\n", "malloc fail");
		return CVI_FAILURE;
	}
	ISP_STATISTICS_CFG_S *pstStatCfg = NULL;

	pstStatCfg = ISP_MALLOC(sizeof(ISP_STATISTICS_CFG_S));
	if (pstStatCfg == NULL) {
		ISP_LOG_ERR("%s\n", "malloc fail");
		free(pstWBStat);
		return CVI_FAILURE;
	}
	CVI_U32 centerRowNum = 0, centerColNum = 0;
	CVI_U32 rowStartOffset = 0, rowEndOffset = 0;
	CVI_U32 colStartOffset = 0, colEndOffset = 0;

	s32Ret = CVI_ISP_GetStatisticsConfig(ViPipe, pstStatCfg);
	if (s32Ret != CVI_SUCCESS) {
		ISP_LOG_DEBUG("CVI_ISP_GetStatisticsConfig fail, 0x%x\n", s32Ret);
		free(pstWBStat);
		free(pstStatCfg);
		return CVI_FAILURE;
	}
	s32Ret = CVI_ISP_GetWBStatistics(ViPipe, pstWBStat);
	if (s32Ret != CVI_SUCCESS) {
		ISP_LOG_DEBUG("CVI_ISP_GetWBStatistics fail, 0x%x\n", s32Ret);
		free(pstWBStat);
		free(pstStatCfg);
		return CVI_FAILURE;
	}

	// calculate how many block is 1/8 image.
	centerRowNum = (pstStatCfg->stWBCfg.u16ZoneRow + 7) >> 3;
	centerColNum = (pstStatCfg->stWBCfg.u16ZoneCol + 7) >> 3;

	// when 1/8 image size block num is even. End offset more.
	rowStartOffset = centerRowNum >> 1;
	rowEndOffset = centerRowNum - rowStartOffset;
	colStartOffset = centerColNum >> 1;
	colEndOffset = centerColNum - colStartOffset;

	colStart = (pstStatCfg->stWBCfg.u16ZoneCol >> 1) - colStartOffset;
	colEnd   = (pstStatCfg->stWBCfg.u16ZoneCol >> 1) + colEndOffset;
	rowStart = (pstStatCfg->stWBCfg.u16ZoneRow >> 1) - rowStartOffset;
	rowEnd   = (pstStatCfg->stWBCfg.u16ZoneRow >> 1) + rowEndOffset;
	/*Get_statistics*/
	for (row = rowStart; row < rowEnd; row++) {
		for (col = colStart; col < colEnd; col++) {
			index = row * pstStatCfg->stWBCfg.u16ZoneCol + col;
			rSum += pstWBStat->au16ZoneAvgR[index];
			bSum += pstWBStat->au16ZoneAvgB[index];
			gSum += pstWBStat->au16ZoneAvgG[index];
		}
	}

	rSum = (rSum == 0) ? 1 : rSum;
	bSum = (bSum == 0) ? 1 : bSum;
	// center block R/B gain for calibration. 1024 = 1x.
	pstAWBLightBoxGain->u16AvgRgain = (gSum << 10) / rSum;
	pstAWBLightBoxGain->u16AvgBgain = (gSum << 10) / bSum;

	free(pstWBStat);
	free(pstStatCfg);
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetMGStatistics(VI_PIPE ViPipe, ISP_MG_STATISTICS_S *pstMgStat)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstMgStat == CVI_NULL) {
		return CVI_FAILURE;
	}

	/*TODO@CF. Need check what is MGstatistics*/
	return CVI_SUCCESS;
}

static CVI_S32 isp_check_statistics_ae_attr_valid(const ISP_AE_STATISTICS_CFG_S *pstAEStatisticsCfg)
{
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(pstAEStatisticsCfg);

	return ret;
}

static CVI_S32 isp_check_statistics_wb_attr_valid(const ISP_WB_STATISTICS_CFG_S *pstWBStatisticsCfg)
{
	CVI_S32 ret = CVI_SUCCESS;

	CHECK_VALID_CONST(pstWBStatisticsCfg, u16ZoneRow, 0x0, AWB_ZONE_ORIG_ROW);
	CHECK_VALID_CONST(pstWBStatisticsCfg, u16ZoneCol, 0x0, AWB_ZONE_ORIG_COLUMN);
	CHECK_VALID_CONST(pstWBStatisticsCfg, u16BlackLevel, 0x0, 0xfff);
	CHECK_VALID_CONST(pstWBStatisticsCfg, u16WhiteLevel, (pstWBStatisticsCfg->u16BlackLevel)+1, 0xfff);

	return ret;
}

static CVI_S32 isp_check_statistics_focus_attr_valid(const ISP_FOCUS_STATISTICS_CFG_S *pstFocusStatisticsCfg)
{
	CVI_S32 ret = CVI_SUCCESS;

	//CHECK_VALID_CONST(pstFocusStatisticsCfg, stConfig.u8ThHigh, 0x0, 0xff);
	CHECK_VALID_CONST(pstFocusStatisticsCfg, stConfig.u8ThLow, 0x0,
		pstFocusStatisticsCfg->stConfig.u8ThHigh);
	CHECK_VALID_CONST(pstFocusStatisticsCfg, stConfig.u8GainLow, 0x0, 0xfe);
	CHECK_VALID_CONST(pstFocusStatisticsCfg, stConfig.u8GainHigh, 0x0, 0xfe);
	CHECK_VALID_CONST(pstFocusStatisticsCfg, stConfig.u8SlopLow, 0x0, 0xf);
	CHECK_VALID_CONST(pstFocusStatisticsCfg, stConfig.u8SlopHigh, 0x0, 0xf);

	return ret;
}

CVI_S32 CVI_ISP_SetStatisticsConfig(VI_PIPE ViPipe, const ISP_STATISTICS_CFG_S *pstStatCfg)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstStatCfg == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	ISP_CTX_S *pstIspCtx = NULL;

	ISP_GET_CTX(ViPipe, pstIspCtx);

	ret = isp_check_statistics_ae_attr_valid(&(pstStatCfg->stAECfg));
	if (ret != CVI_SUCCESS)
		return ret;
	ret = isp_check_statistics_wb_attr_valid(&(pstStatCfg->stWBCfg));
	if (ret != CVI_SUCCESS)
		return ret;
	ret = isp_check_statistics_focus_attr_valid(&(pstStatCfg->stFocusCfg));
	if (ret != CVI_SUCCESS)
		return ret;

	memcpy(&pstIspCtx->stsCfgInfo, pstStatCfg, sizeof(ISP_STATISTICS_CFG_S));

	CVI_ISP_SetAEStatisticsConfig(ViPipe, &pstIspCtx->stsCfgInfo.stAECfg);

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetStatisticsConfig(VI_PIPE ViPipe, ISP_STATISTICS_CFG_S *pstStatCfg)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstStatCfg == CVI_NULL) {
		return CVI_FAILURE;
	}

	ISP_CTX_S *pstIspCtx = NULL;

	ISP_GET_CTX(ViPipe, pstIspCtx);

	CVI_ISP_GetAEStatisticsConfig(ViPipe, &pstIspCtx->stsCfgInfo.stAECfg);

	memcpy(pstStatCfg, &pstIspCtx->stsCfgInfo, sizeof(ISP_STATISTICS_CFG_S));
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_SetModParam(const ISP_MOD_PARAM_S *pstModParam)
{
	ISP_LOG_DEBUG("+\n");
	if (pstModParam == CVI_NULL) {
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetModParam(ISP_MOD_PARAM_S *pstModParam)
{
	if (pstModParam == CVI_NULL) {
		return CVI_FAILURE;
	}

	// BotHalf = 1 means botoom half.
	pstModParam->u32IntBotHalf = 1;

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_SetRegister(VI_PIPE ViPipe, CVI_U32 u32Addr, CVI_U32 u32Value)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

#ifndef ARCH_RTOS_CV181X
	CVI_S32 s32Ret = 0;

	if ((u32Addr < ISP_BASE_ADDR) || (u32Addr > (ISP_BASE_ADDR + ISP_REG_RANGE))) {
		ISP_LOG_DEBUG("addr %#x is over ISP register range\n", u32Addr);
		return CVI_FAILURE;
	}
	s32Ret = regInit();
	if (s32Ret != CVI_SUCCESS) {
		ISP_LOG_DEBUG("Init register address fail\n");
		return CVI_FAILURE;
	}
	regWrite(u32Addr, u32Value);
	regExit();
#else
	UNUSED(u32Addr);
	UNUSED(u32Value);
#endif
	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetRegister(VI_PIPE ViPipe, CVI_U32 u32Addr, CVI_U32 *pu32Value)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pu32Value == CVI_NULL) {
		return CVI_FAILURE;
	}

#ifndef ARCH_RTOS_CV181X
	CVI_S32 s32Ret = 0;

	if ((u32Addr < ISP_BASE_ADDR) || (u32Addr > (ISP_BASE_ADDR + ISP_REG_RANGE))) {
		ISP_LOG_DEBUG("addr %#x is over ISP register range\n", u32Addr);
		return CVI_FAILURE;
	}
	s32Ret = regInit();
	if (s32Ret != CVI_SUCCESS) {
		ISP_LOG_DEBUG("Init register address fail\n");
		return CVI_FAILURE;
	}
	*pu32Value = regRead(u32Addr);
	regExit();
#else
	UNUSED(u32Addr);
#endif
	return CVI_SUCCESS;
}

// For Tool get some parameter use at calculate IQ.
CVI_S32 CVI_ISP_QueryInnerStateInfo(VI_PIPE ViPipe, ISP_INNER_STATE_INFO_S *pstInnerStateInfo)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstInnerStateInfo == CVI_NULL) {
		return CVI_FAILURE;
	}

	ISP_CTX_S *pstIspCtx = NULL;

	ISP_GET_CTX(ViPipe, pstIspCtx);
	isp_feature_ctrl_get_module_info(ViPipe, pstInnerStateInfo);

	pstInnerStateInfo->bWDRSwitchFinish = pstIspCtx->bWDRSwitchFinish;
	// exposure ratio default return 64.
	for (CVI_U32 i = 0; i < ISP_WDR_FRAME_IDX_SIZE; i++)
		pstInnerStateInfo->u32WDRExpRatioActual[i] = 64;
	if (pstIspCtx->u8SnsWDRMode == WDR_MODE_2To1_LINE) {
		pstInnerStateInfo->u32WDRExpRatioActual[ISP_WDR_FRAME_IDX_1]
			= pstIspCtx->stAeResult.u32ExpRatio;
	} else {
		// When not linear or 2TO1 mode. Print not support Log.
		if (pstIspCtx->u8SnsWDRMode != WDR_MODE_NONE) {
			ISP_LOG_DEBUG("This Wdr mode not support get exposure ratio now\n");
		}
	}
	return CVI_SUCCESS;
}

//-----------------------------------------------------------------------------
//  Black Level Correction(BLC)
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_GetBlackLevelAttr(VI_PIPE ViPipe, ISP_BLACK_LEVEL_ATTR_S *pstBlackLevelAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstBlackLevelAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_BLACK_LEVEL_ATTR_S *pTemp = NULL;

	ret = isp_blc_ctrl_get_blc_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstBlackLevelAttr, pTemp, sizeof(ISP_BLACK_LEVEL_ATTR_S));
	}

	return ret;
}

CVI_S32 CVI_ISP_SetBlackLevelAttr(VI_PIPE ViPipe, const ISP_BLACK_LEVEL_ATTR_S *pstBlackLevelAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstBlackLevelAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_blc_ctrl_set_blc_attr(ViPipe, pstBlackLevelAttr);

	return ret;
}

//-----------------------------------------------------------------------------
//  Dead pixel correction (DPC)
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_SetDPDynamicAttr(VI_PIPE ViPipe, const ISP_DP_DYNAMIC_ATTR_S *pstDPCDynamicAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstDPCDynamicAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_dpc_ctrl_set_dpc_dynamic_attr(ViPipe, pstDPCDynamicAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetDPDynamicAttr(VI_PIPE ViPipe, ISP_DP_DYNAMIC_ATTR_S *pstDPCDynamicAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstDPCDynamicAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_DP_DYNAMIC_ATTR_S *pTemp = NULL;

	ret = isp_dpc_ctrl_get_dpc_dynamic_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstDPCDynamicAttr, pTemp, sizeof(ISP_DP_DYNAMIC_ATTR_S));
	}

	return ret;
}

CVI_S32 CVI_ISP_SetDPStaticAttr(VI_PIPE ViPipe, const ISP_DP_STATIC_ATTR_S *pstDPStaticAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstDPStaticAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_dpc_ctrl_set_dpc_static_attr(ViPipe, pstDPStaticAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetDPStaticAttr(VI_PIPE ViPipe, ISP_DP_STATIC_ATTR_S *pstDPStaticAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstDPStaticAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
#ifndef ARCH_RTOS_CV181X
	const ISP_DP_STATIC_ATTR_S *pTemp = NULL;

	ret = isp_dpc_ctrl_get_dpc_static_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstDPStaticAttr, pTemp, sizeof(ISP_DP_STATIC_ATTR_S));
	}
#else
	UNUSED(ViPipe);
	UNUSED(pstDPStaticAttr);
#endif
	return ret;
}

CVI_S32 CVI_ISP_SetDPCalibrate(VI_PIPE ViPipe, const ISP_DP_CALIB_ATTR_S *pstDPCalibAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstDPCalibAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
#ifndef ARCH_RTOS_CV181X
	ret = isp_dpc_ctrl_set_dpc_calibrate(ViPipe, pstDPCalibAttr);
#else
	UNUSED(ViPipe);
	UNUSED(pstDPCalibAttr);
#endif
	return ret;
}

CVI_S32 CVI_ISP_GetDPCalibrate(VI_PIPE ViPipe, ISP_DP_CALIB_ATTR_S *pstDPCalibAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstDPCalibAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
#ifndef ARCH_RTOS_CV181X
	const ISP_DP_CALIB_ATTR_S *pTemp = NULL;

	ret = isp_dpc_ctrl_get_dpc_calibrate(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstDPCalibAttr, pTemp, sizeof(ISP_DP_CALIB_ATTR_S));
	}
#else
	UNUSED(ViPipe);
	UNUSED(pstDPCalibAttr);
#endif
	return ret;
}

//-----------------------------------------------------------------------------
//  Crosstalk
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_SetCrosstalkAttr(VI_PIPE ViPipe, const ISP_CROSSTALK_ATTR_S *pstCrosstalkAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstCrosstalkAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_crosstalk_ctrl_set_crosstalk_attr(ViPipe, pstCrosstalkAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetCrosstalkAttr(VI_PIPE ViPipe, ISP_CROSSTALK_ATTR_S *pstCrosstalkAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstCrosstalkAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_CROSSTALK_ATTR_S *pTemp = NULL;

	ret = isp_crosstalk_ctrl_get_crosstalk_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstCrosstalkAttr, pTemp, sizeof(ISP_CROSSTALK_ATTR_S));
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  Bayer domain noise reduction (BNR)
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_SetNRAttr(VI_PIPE ViPipe, const ISP_NR_ATTR_S *pstNRAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstNRAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_bnr_ctrl_set_bnr_attr(ViPipe, pstNRAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetNRAttr(VI_PIPE ViPipe, ISP_NR_ATTR_S *pstNRAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstNRAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_NR_ATTR_S *pTemp = NULL;

	ret = isp_bnr_ctrl_get_bnr_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstNRAttr, pTemp, sizeof(ISP_NR_ATTR_S));
	}

	return ret;
}

CVI_S32 CVI_ISP_SetNRFilterAttr(VI_PIPE ViPipe, const ISP_NR_FILTER_ATTR_S *pstNRFilterAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstNRFilterAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_bnr_ctrl_set_bnr_filter_attr(ViPipe, pstNRFilterAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetNRFilterAttr(VI_PIPE ViPipe, ISP_NR_FILTER_ATTR_S *pstNRFilterAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstNRFilterAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_NR_FILTER_ATTR_S *pTemp = NULL;

	ret = isp_bnr_ctrl_get_bnr_filter_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstNRFilterAttr, pTemp, sizeof(ISP_NR_FILTER_ATTR_S));
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  Demosaic
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_SetDemosaicAttr(VI_PIPE ViPipe, const ISP_DEMOSAIC_ATTR_S *pstDemosaicAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstDemosaicAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	ISP_RGBCAC_ATTR_S stRGBCACAttr;

	CVI_ISP_GetRGBCACAttr(ViPipe, &stRGBCACAttr);
	if (stRGBCACAttr.Enable && !pstDemosaicAttr->Enable) {
		ISP_LOG_ERR("Don't support demosaic disable && rgbcac enable\n");
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_demosaic_ctrl_set_demosaic_attr(ViPipe, pstDemosaicAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetDemosaicAttr(VI_PIPE ViPipe, ISP_DEMOSAIC_ATTR_S *pstDemosaicAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstDemosaicAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_DEMOSAIC_ATTR_S *pTemp = NULL;

	ret = isp_demosaic_ctrl_get_demosaic_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstDemosaicAttr, pTemp, sizeof(ISP_DEMOSAIC_ATTR_S));
	}

	return ret;
}

CVI_S32 CVI_ISP_SetDemosaicDemoireAttr(VI_PIPE ViPipe,
	const ISP_DEMOSAIC_DEMOIRE_ATTR_S *pstDemosaicDemoireAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstDemosaicDemoireAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_demosaic_ctrl_set_demosaic_demoire_attr(ViPipe, pstDemosaicDemoireAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetDemosaicDemoireAttr(VI_PIPE ViPipe,
	ISP_DEMOSAIC_DEMOIRE_ATTR_S *pstDemosaicDemoireAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstDemosaicDemoireAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_DEMOSAIC_DEMOIRE_ATTR_S *pTemp = NULL;

	ret = isp_demosaic_ctrl_get_demosaic_demoire_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstDemosaicDemoireAttr, pTemp, sizeof(ISP_DEMOSAIC_DEMOIRE_ATTR_S));
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  RGBCAC
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_SetRGBCACAttr(VI_PIPE ViPipe, const ISP_RGBCAC_ATTR_S *pstRGBCACAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstRGBCACAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	ISP_DEMOSAIC_ATTR_S stDemosaicAttr;

	CVI_ISP_GetDemosaicAttr(ViPipe, &stDemosaicAttr);
	if (!stDemosaicAttr.Enable && pstRGBCACAttr->Enable) {
		ISP_LOG_ERR("Don't support demosaic disable && rgbcac enable\n");
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_rgbcac_ctrl_set_rgbcac_attr(ViPipe, pstRGBCACAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetRGBCACAttr(VI_PIPE ViPipe, ISP_RGBCAC_ATTR_S *pstRGBCACAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstRGBCACAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_RGBCAC_ATTR_S *pTemp = NULL;

	ret = isp_rgbcac_ctrl_get_rgbcac_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstRGBCACAttr, pTemp, sizeof(ISP_RGBCAC_ATTR_S));
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  LCAC
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_SetLCACAttr(VI_PIPE ViPipe, const ISP_LCAC_ATTR_S *pstLCACAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstLCACAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_lcac_ctrl_set_lcac_attr(ViPipe, pstLCACAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetLCACAttr(VI_PIPE ViPipe, ISP_LCAC_ATTR_S *pstLCACAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstLCACAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_LCAC_ATTR_S *pTemp = NULL;

	ret = isp_lcac_ctrl_get_lcac_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstLCACAttr, pTemp, sizeof(ISP_LCAC_ATTR_S));
	}

	return ret;
}

// TODO@Kidd should add ISP API to calculate settings of radio shadding
// and remove CVI_ISP_SetRadioShadingAttr() / CVI_ISP_GetRadioShadingAttr()
//-----------------------------------------------------------------------------
//  Mesh lens shading correction (MLSC)
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_SetMeshShadingAttr(VI_PIPE ViPipe, const ISP_MESH_SHADING_ATTR_S *pstMeshShadingAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstMeshShadingAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_mlsc_ctrl_set_mlsc_attr(ViPipe, pstMeshShadingAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetMeshShadingAttr(VI_PIPE ViPipe, ISP_MESH_SHADING_ATTR_S *pstMeshShadingAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstMeshShadingAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_MESH_SHADING_ATTR_S *pTemp = NULL;

	ret = isp_mlsc_ctrl_get_mlsc_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstMeshShadingAttr, pTemp, sizeof(ISP_MESH_SHADING_ATTR_S));
	}

	return ret;
}

CVI_S32 CVI_ISP_SetMeshShadingGainLutAttr(VI_PIPE ViPipe
	, const ISP_MESH_SHADING_GAIN_LUT_ATTR_S *pstMeshShadingGainLutAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstMeshShadingGainLutAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_mlsc_ctrl_set_mlsc_lut_attr(ViPipe, pstMeshShadingGainLutAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetMeshShadingGainLutAttr(VI_PIPE ViPipe, ISP_MESH_SHADING_GAIN_LUT_ATTR_S *pstMeshShadingGainLutAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstMeshShadingGainLutAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_MESH_SHADING_GAIN_LUT_ATTR_S *pTemp = NULL;

	ret = isp_mlsc_ctrl_get_mlsc_lut_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstMeshShadingGainLutAttr, pTemp,
			sizeof(ISP_MESH_SHADING_GAIN_LUT_ATTR_S));
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  CCM
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_SetSaturationAttr(VI_PIPE ViPipe, const ISP_SATURATION_ATTR_S *pstSaturationAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstSaturationAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_ccm_ctrl_set_saturation_attr(ViPipe, pstSaturationAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetSaturationAttr(VI_PIPE ViPipe, ISP_SATURATION_ATTR_S *pstSaturationAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstSaturationAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_SATURATION_ATTR_S *pTemp = NULL;

	ret = isp_ccm_ctrl_get_saturation_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstSaturationAttr, pTemp, sizeof(ISP_SATURATION_ATTR_S));
	}

	return ret;
}

CVI_S32 CVI_ISP_SetCCMSaturationAttr(VI_PIPE ViPipe, const ISP_CCM_SATURATION_ATTR_S *pstCCMSaturationAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstCCMSaturationAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_ccm_ctrl_set_ccm_saturation_attr(ViPipe, pstCCMSaturationAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetCCMSaturationAttr(VI_PIPE ViPipe, ISP_CCM_SATURATION_ATTR_S *pstCCMSaturationAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstCCMSaturationAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_CCM_SATURATION_ATTR_S *pTemp = NULL;

	ret = isp_ccm_ctrl_get_ccm_saturation_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstCCMSaturationAttr, pTemp, sizeof(ISP_CCM_SATURATION_ATTR_S));
	}

	return ret;
}

CVI_S32 CVI_ISP_SetCCMAttr(VI_PIPE ViPipe, const ISP_CCM_ATTR_S *pstCCMAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstCCMAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_ccm_ctrl_set_ccm_attr(ViPipe, pstCCMAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetCCMAttr(VI_PIPE ViPipe, ISP_CCM_ATTR_S *pstCCMAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstCCMAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_CCM_ATTR_S *pTemp = NULL;

	ret = isp_ccm_ctrl_get_ccm_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstCCMAttr, pTemp, sizeof(ISP_CCM_ATTR_S));
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  CSC
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_SetCSCAttr(VI_PIPE ViPipe, const ISP_CSC_ATTR_S *pstCscAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstCscAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_csc_ctrl_set_csc_attr(ViPipe, pstCscAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetCSCAttr(VI_PIPE ViPipe, ISP_CSC_ATTR_S *pstCscAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstCscAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_CSC_ATTR_S *pTemp = NULL;

	ret = isp_csc_ctrl_get_csc_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstCscAttr, pTemp, sizeof(ISP_CSC_ATTR_S));
	}

	return ret;
}
//-----------------------------------------------------------------------------
//  Color tone
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_SetColorToneAttr(VI_PIPE ViPipe, const ISP_COLOR_TONE_ATTR_S *pstColorToneAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstColorToneAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_wb_ctrl_set_wb_colortone_attr(ViPipe, pstColorToneAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetColorToneAttr(VI_PIPE ViPipe, ISP_COLOR_TONE_ATTR_S *pstColorToneAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstColorToneAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_COLOR_TONE_ATTR_S *pTemp = NULL;

	ret = isp_wb_ctrl_get_wb_colortone_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstColorToneAttr, pTemp, sizeof(ISP_COLOR_TONE_ATTR_S));
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  FSWDR
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_SetFSWDRAttr(VI_PIPE ViPipe, const ISP_FSWDR_ATTR_S *pstFSWDRAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstFSWDRAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (pstFSWDRAttr->stManual.WDRCombineMaxWeight <
		pstFSWDRAttr->stManual.WDRCombineMinWeight) {
		return CVI_FAILURE_ILLEGAL_PARAM;
	}
	if (pstFSWDRAttr->stManual.WDRMotionCombineMaxWeight <
			pstFSWDRAttr->stManual.WDRMotionCombineMinWeight) {
		return CVI_FAILURE_ILLEGAL_PARAM;
	}

	for (CVI_U32 i = 0; i < ISP_AUTO_LV_NUM; i++) {
		if (pstFSWDRAttr->stAuto.WDRCombineMaxWeight[i] <
			pstFSWDRAttr->stAuto.WDRCombineMinWeight[i]) {
			return CVI_FAILURE_ILLEGAL_PARAM;
		}
		if (pstFSWDRAttr->stAuto.WDRMotionCombineMaxWeight[i] <
				pstFSWDRAttr->stAuto.WDRMotionCombineMinWeight[i]) {
			return CVI_FAILURE_ILLEGAL_PARAM;
		}
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_fswdr_ctrl_set_fswdr_attr(ViPipe, pstFSWDRAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetFSWDRAttr(VI_PIPE ViPipe, ISP_FSWDR_ATTR_S *pstFSWDRAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstFSWDRAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_FSWDR_ATTR_S *pTemp = NULL;

	ret = isp_fswdr_ctrl_get_fswdr_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstFSWDRAttr, pTemp, sizeof(ISP_FSWDR_ATTR_S));
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  DRC
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_SetDRCAttr(VI_PIPE ViPipe, const ISP_DRC_ATTR_S *pstDRCAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstDRCAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_drc_ctrl_set_drc_attr(ViPipe, pstDRCAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetDRCAttr(VI_PIPE ViPipe, ISP_DRC_ATTR_S *pstDRCAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstDRCAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_DRC_ATTR_S *pTemp = NULL;

	ret = isp_drc_ctrl_get_drc_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstDRCAttr, pTemp, sizeof(ISP_DRC_ATTR_S));
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  Gamma
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_SetGammaAttr(VI_PIPE ViPipe, const ISP_GAMMA_ATTR_S *pstGammaAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstGammaAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_gamma_ctrl_set_gamma_attr(ViPipe, pstGammaAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetGammaAttr(VI_PIPE ViPipe, ISP_GAMMA_ATTR_S *pstGammaAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstGammaAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_GAMMA_ATTR_S *pTemp = NULL;

	ret = isp_gamma_ctrl_get_gamma_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstGammaAttr, pTemp, sizeof(ISP_GAMMA_ATTR_S));
	}

	return ret;
}

CVI_S32 CVI_ISP_GetGammaCurveByType(VI_PIPE ViPipe,
	ISP_GAMMA_ATTR_S *pstGammaAttr, const ISP_GAMMA_CURVE_TYPE_E curveType)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstGammaAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_gamma_ctrl_get_gamma_curve_by_type(ViPipe, pstGammaAttr, curveType);

	return ret;
}

CVI_S32 CVI_ISP_SetAutoGammaAttr(VI_PIPE ViPipe, const ISP_AUTO_GAMMA_ATTR_S *pstGammaAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstGammaAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_gamma_ctrl_set_auto_gamma_attr(ViPipe, pstGammaAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetAutoGammaAttr(VI_PIPE ViPipe, ISP_AUTO_GAMMA_ATTR_S *pstGammaAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstGammaAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_AUTO_GAMMA_ATTR_S *pTemp = NULL;

	ret = isp_gamma_ctrl_get_auto_gamma_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstGammaAttr, pTemp, sizeof(ISP_AUTO_GAMMA_ATTR_S));
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  Dehaze
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_SetDehazeAttr(VI_PIPE ViPipe, const ISP_DEHAZE_ATTR_S *pstDehazeAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstDehazeAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_dehaze_ctrl_set_dehaze_attr(ViPipe, pstDehazeAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetDehazeAttr(VI_PIPE ViPipe, ISP_DEHAZE_ATTR_S *pstDehazeAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstDehazeAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_DEHAZE_ATTR_S *pTemp = NULL;

	ret = isp_dehaze_ctrl_get_dehaze_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstDehazeAttr, pTemp, sizeof(ISP_DEHAZE_ATTR_S));
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  CLUT
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_SetClutAttr(VI_PIPE ViPipe, const ISP_CLUT_ATTR_S *pstClutAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstClutAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_clut_ctrl_set_clut_attr(ViPipe, pstClutAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetClutAttr(VI_PIPE ViPipe, ISP_CLUT_ATTR_S *pstClutAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstClutAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	const ISP_CLUT_ATTR_S *pTemp = NULL;

	ret = isp_clut_ctrl_get_clut_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstClutAttr, pTemp, sizeof(ISP_CLUT_ATTR_S));
	}
	return ret;
}

CVI_S32 CVI_ISP_SetClutSaturationAttr(VI_PIPE ViPipe, const ISP_CLUT_SATURATION_ATTR_S *pstClutSaturationAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstClutSaturationAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_clut_ctrl_set_clut_saturation_attr(ViPipe, pstClutSaturationAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetClutSaturationAttr(VI_PIPE ViPipe, ISP_CLUT_SATURATION_ATTR_S *pstClutSaturationAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstClutSaturationAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	const ISP_CLUT_SATURATION_ATTR_S *pTemp = NULL;

	ret = isp_clut_ctrl_get_clut_saturation_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstClutSaturationAttr, pTemp, sizeof(ISP_CLUT_SATURATION_ATTR_S));
	}
	return ret;
}

//-----------------------------------------------------------------------------
//  DCI
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_SetDCIAttr(VI_PIPE ViPipe, const ISP_DCI_ATTR_S *pstDCIAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstDCIAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_dci_ctrl_set_dci_attr(ViPipe, pstDCIAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetDCIAttr(VI_PIPE ViPipe, ISP_DCI_ATTR_S *pstDCIAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstDCIAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_DCI_ATTR_S *pTemp = NULL;

	ret = isp_dci_ctrl_get_dci_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstDCIAttr, pTemp, sizeof(ISP_DCI_ATTR_S));
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  LDCI
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_SetLDCIAttr(VI_PIPE ViPipe, const ISP_LDCI_ATTR_S *pstLDCIAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstLDCIAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_ldci_ctrl_set_ldci_attr(ViPipe, pstLDCIAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetLDCIAttr(VI_PIPE ViPipe, ISP_LDCI_ATTR_S *pstLDCIAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstLDCIAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_LDCI_ATTR_S *pstTemp = NULL;

	ret = isp_ldci_ctrl_get_ldci_attr(ViPipe, &pstTemp);
	if (pstTemp != NULL) {
		memcpy(pstLDCIAttr, pstTemp, sizeof(ISP_LDCI_ATTR_S));
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  CA (CA/CP)
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_SetCAAttr(VI_PIPE ViPipe, const ISP_CA_ATTR_S *pstCAAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstCAAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_ca_ctrl_set_ca_attr(ViPipe, pstCAAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetCAAttr(VI_PIPE ViPipe, ISP_CA_ATTR_S *pstCAAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstCAAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_CA_ATTR_S *pTemp = NULL;

	ret = isp_ca_ctrl_get_ca_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstCAAttr, pTemp, sizeof(ISP_CA_ATTR_S));
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  CA2
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_SetCA2Attr(VI_PIPE ViPipe, const ISP_CA2_ATTR_S *pstCA2Attr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstCA2Attr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_ca2_ctrl_set_ca2_attr(ViPipe, pstCA2Attr);

	return ret;
}

CVI_S32 CVI_ISP_GetCA2Attr(VI_PIPE ViPipe, ISP_CA2_ATTR_S *pstCA2Attr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstCA2Attr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_CA2_ATTR_S *pTemp = NULL;

	ret = isp_ca2_ctrl_get_ca2_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstCA2Attr, pTemp, sizeof(ISP_CA2_ATTR_S));
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  PreSharpen
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_SetPreSharpenAttr(VI_PIPE ViPipe, const ISP_PRESHARPEN_ATTR_S *pstPreSharpenAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstPreSharpenAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_presharpen_ctrl_set_presharpen_attr(ViPipe, pstPreSharpenAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetPreSharpenAttr(VI_PIPE ViPipe, ISP_PRESHARPEN_ATTR_S *pstPreSharpenAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstPreSharpenAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_PRESHARPEN_ATTR_S *pTemp = NULL;

	ret = isp_presharpen_ctrl_get_presharpen_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstPreSharpenAttr, pTemp, sizeof(ISP_PRESHARPEN_ATTR_S));
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  Time-domain noise reduction (TNR)
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_SetTNRAttr(VI_PIPE ViPipe, const ISP_TNR_ATTR_S *pstTNRAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstTNRAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_tnr_ctrl_set_tnr_attr(ViPipe, pstTNRAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetTNRAttr(VI_PIPE ViPipe, ISP_TNR_ATTR_S *pstTNRAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstTNRAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_TNR_ATTR_S *pTemp = NULL;

	ret = isp_tnr_ctrl_get_tnr_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstTNRAttr, pTemp, sizeof(ISP_TNR_ATTR_S));
	}

	return ret;
}

CVI_S32 CVI_ISP_SetTNRNoiseModelAttr(VI_PIPE ViPipe, const ISP_TNR_NOISE_MODEL_ATTR_S *pstTNRNoiseModelAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstTNRNoiseModelAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_tnr_ctrl_set_tnr_noise_model_attr(ViPipe, pstTNRNoiseModelAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetTNRNoiseModelAttr(VI_PIPE ViPipe, ISP_TNR_NOISE_MODEL_ATTR_S *pstTNRNoiseModelAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstTNRNoiseModelAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_TNR_NOISE_MODEL_ATTR_S *pTemp = NULL;

	ret = isp_tnr_ctrl_get_tnr_noise_model_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstTNRNoiseModelAttr, pTemp, sizeof(ISP_TNR_NOISE_MODEL_ATTR_S));
	}

	return ret;
}

CVI_S32 CVI_ISP_SetTNRLumaMotionAttr(VI_PIPE ViPipe, const ISP_TNR_LUMA_MOTION_ATTR_S *pstTNRLumaMotionAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstTNRLumaMotionAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_tnr_ctrl_set_tnr_luma_motion_attr(ViPipe, pstTNRLumaMotionAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetTNRLumaMotionAttr(VI_PIPE ViPipe, ISP_TNR_LUMA_MOTION_ATTR_S *pstTNRLumaMotionAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstTNRLumaMotionAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_TNR_LUMA_MOTION_ATTR_S *pTemp = NULL;

	ret = isp_tnr_ctrl_get_tnr_luma_motion_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstTNRLumaMotionAttr, pTemp, sizeof(ISP_TNR_LUMA_MOTION_ATTR_S));
	}

	return ret;
}

CVI_S32 CVI_ISP_SetTNRGhostAttr(VI_PIPE ViPipe, const ISP_TNR_GHOST_ATTR_S *pstTNRGhostAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstTNRGhostAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_tnr_ctrl_set_tnr_ghost_attr(ViPipe, pstTNRGhostAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetTNRGhostAttr(VI_PIPE ViPipe, ISP_TNR_GHOST_ATTR_S *pstTNRGhostAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstTNRGhostAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_TNR_GHOST_ATTR_S *pTemp = NULL;

	ret = isp_tnr_ctrl_get_tnr_ghost_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstTNRGhostAttr, pTemp, sizeof(ISP_TNR_GHOST_ATTR_S));
	}

	return ret;
}

CVI_S32 CVI_ISP_SetTNRMtPrtAttr(VI_PIPE ViPipe, const ISP_TNR_MT_PRT_ATTR_S *pstTNRMtPrtAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstTNRMtPrtAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_tnr_ctrl_set_tnr_mt_prt_attr(ViPipe, pstTNRMtPrtAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetTNRMtPrtAttr(VI_PIPE ViPipe, ISP_TNR_MT_PRT_ATTR_S *pstTNRMtPrtAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstTNRMtPrtAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_TNR_MT_PRT_ATTR_S *pTemp = NULL;

	ret = isp_tnr_ctrl_get_tnr_mt_prt_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstTNRMtPrtAttr, pTemp, sizeof(ISP_TNR_MT_PRT_ATTR_S));
	}

	return ret;
}

CVI_S32 CVI_ISP_SetTNRMotionAdaptAttr(VI_PIPE ViPipe, const ISP_TNR_MOTION_ADAPT_ATTR_S *pstTNRMotionAdaptAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstTNRMotionAdaptAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_tnr_ctrl_set_tnr_motion_adapt_attr(ViPipe, pstTNRMotionAdaptAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetTNRMotionAdaptAttr(VI_PIPE ViPipe, ISP_TNR_MOTION_ADAPT_ATTR_S *pstTNRMotionAdaptAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstTNRMotionAdaptAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_TNR_MOTION_ADAPT_ATTR_S *pTemp = NULL;

	ret = isp_tnr_ctrl_get_tnr_motion_adapt_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstTNRMotionAdaptAttr, pTemp, sizeof(ISP_TNR_MOTION_ADAPT_ATTR_S));
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  Y domain noise reduction (YNR)
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_SetYNRAttr(VI_PIPE ViPipe, const ISP_YNR_ATTR_S *pstYNRAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstYNRAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_ynr_ctrl_set_ynr_attr(ViPipe, pstYNRAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetYNRAttr(VI_PIPE ViPipe, ISP_YNR_ATTR_S *pstYNRAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstYNRAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_YNR_ATTR_S *pTemp = NULL;

	ret = isp_ynr_ctrl_get_ynr_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstYNRAttr, pTemp, sizeof(ISP_YNR_ATTR_S));
	}

	return ret;
}

CVI_S32 CVI_ISP_SetYNRMotionNRAttr(VI_PIPE ViPipe, const ISP_YNR_MOTION_NR_ATTR_S *pstYNRMotionNRAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstYNRMotionNRAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_ynr_ctrl_set_ynr_motion_attr(ViPipe, pstYNRMotionNRAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetYNRMotionNRAttr(VI_PIPE ViPipe, ISP_YNR_MOTION_NR_ATTR_S *pstYNRMotionNRAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstYNRMotionNRAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_YNR_MOTION_NR_ATTR_S *pTemp = NULL;

	ret = isp_ynr_ctrl_get_ynr_motion_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstYNRMotionNRAttr, pTemp, sizeof(ISP_YNR_MOTION_NR_ATTR_S));
	}

	return ret;
}

CVI_S32 CVI_ISP_SetYNRFilterAttr(VI_PIPE ViPipe, const ISP_YNR_FILTER_ATTR_S *pstYNRFilterAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstYNRFilterAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_ynr_ctrl_set_ynr_filter_attr(ViPipe, pstYNRFilterAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetYNRFilterAttr(VI_PIPE ViPipe, ISP_YNR_FILTER_ATTR_S *pstYNRFilterAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstYNRFilterAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_YNR_FILTER_ATTR_S *pTemp = NULL;

	ret = isp_ynr_ctrl_get_ynr_filter_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstYNRFilterAttr, pTemp, sizeof(ISP_YNR_FILTER_ATTR_S));
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  UV domain noise reduction (CNR)
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_SetCNRAttr(VI_PIPE ViPipe, const ISP_CNR_ATTR_S *pstCNRAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstCNRAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_cnr_ctrl_set_cnr_attr(ViPipe, pstCNRAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetCNRAttr(VI_PIPE ViPipe, ISP_CNR_ATTR_S *pstCNRAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstCNRAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_CNR_ATTR_S *pTemp = NULL;

	ret = isp_cnr_ctrl_get_cnr_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstCNRAttr, pTemp, sizeof(ISP_CNR_ATTR_S));
	}

	return ret;
}

CVI_S32 CVI_ISP_SetCNRMotionNRAttr(VI_PIPE ViPipe, const ISP_CNR_MOTION_NR_ATTR_S *pstCNRMotionNRAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstCNRMotionNRAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_cnr_ctrl_set_cnr_motion_attr(ViPipe, pstCNRMotionNRAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetCNRMotionNRAttr(VI_PIPE ViPipe, ISP_CNR_MOTION_NR_ATTR_S *pstCNRMotionNRAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstCNRMotionNRAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_CNR_MOTION_NR_ATTR_S *pTemp = NULL;

	ret = isp_cnr_ctrl_get_cnr_motion_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstCNRMotionNRAttr, pTemp, sizeof(ISP_CNR_MOTION_NR_ATTR_S));
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  CAC
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_SetCACAttr(VI_PIPE ViPipe, const ISP_CAC_ATTR_S *pstCACAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstCACAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_cac_ctrl_set_cac_attr(ViPipe, pstCACAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetCACAttr(VI_PIPE ViPipe, ISP_CAC_ATTR_S *pstCACAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstCACAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_CAC_ATTR_S *pTemp = NULL;

	ret = isp_cac_ctrl_get_cac_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstCACAttr, pTemp, sizeof(ISP_CAC_ATTR_S));
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  CA2 (CA-lite)
//-----------------------------------------------------------------------------
#if 0
CVI_S32 CVI_ISP_SetCA2Attr(VI_PIPE ViPipe, const ISP_CA2_ATTR_S *pstCA2Attr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstCA2Attr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_ca2_ctrl_set_ca2_attr(ViPipe, pstCA2Attr);

	return ret;
}

CVI_S32 CVI_ISP_GetCA2Attr(VI_PIPE ViPipe, ISP_CA2_ATTR_S *pstCA2Attr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstCA2Attr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	ISP_CA2_ATTR_S *pTemp = NULL;

	ret = isp_ca2_ctrl_get_ca2_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstCA2Attr, pTemp, sizeof(ISP_CA2_ATTR_S));
	}

	return ret;
}
#endif

//-----------------------------------------------------------------------------
//  Sharpen
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_SetSharpenAttr(VI_PIPE ViPipe, const ISP_SHARPEN_ATTR_S *pstSharpenAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstSharpenAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_sharpen_ctrl_set_sharpen_attr(ViPipe, pstSharpenAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetSharpenAttr(VI_PIPE ViPipe, ISP_SHARPEN_ATTR_S *pstSharpenAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstSharpenAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_SHARPEN_ATTR_S *pTemp = NULL;

	ret = isp_sharpen_ctrl_get_sharpen_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstSharpenAttr, pTemp, sizeof(ISP_SHARPEN_ATTR_S));
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  Y Contrast
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_SetYContrastAttr(VI_PIPE ViPipe, const ISP_YCONTRAST_ATTR_S *pstYContrastAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstYContrastAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_ycontrast_ctrl_set_ycontrast_attr(ViPipe, pstYContrastAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetYContrastAttr(VI_PIPE ViPipe, ISP_YCONTRAST_ATTR_S *pstYContrastAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstYContrastAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_YCONTRAST_ATTR_S *pTemp = NULL;

	ret = isp_ycontrast_ctrl_get_ycontrast_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstYContrastAttr, pTemp, sizeof(ISP_YCONTRAST_ATTR_S));
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  Mono
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_SetMonoAttr(VI_PIPE ViPipe, const ISP_MONO_ATTR_S *pstMonoAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstMonoAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_mono_ctrl_set_mono_attr(ViPipe, pstMonoAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetMonoAttr(VI_PIPE ViPipe, ISP_MONO_ATTR_S *pstMonoAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstMonoAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_MONO_ATTR_S *pTemp = NULL;

	ret = isp_mono_ctrl_get_mono_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstMonoAttr, pTemp, sizeof(ISP_MONO_ATTR_S));
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  Noise Profile
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_SetNoiseProfileAttr(VI_PIPE ViPipe, const ISP_CMOS_NOISE_CALIBRATION_S *pstNoiseProfileAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstNoiseProfileAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	ISP_CMOS_NOISE_CALIBRATION_S * p = CVI_NULL;

	isp_mgr_buf_get_np_addr(ViPipe, (CVI_VOID **) &p);

	memcpy((CVI_VOID *) p, pstNoiseProfileAttr, sizeof(*pstNoiseProfileAttr));

	ISP_TNR_INTER_ATTR_S stTNRInterAttr;

	// Notice TNR to update motion unit
	isp_tnr_ctrl_get_tnr_internal_attr(ViPipe, &stTNRInterAttr);
	stTNRInterAttr.bUpdateMotionUnit = CVI_TRUE;
	isp_tnr_ctrl_set_tnr_internal_attr(ViPipe, &stTNRInterAttr);

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_GetNoiseProfileAttr(VI_PIPE ViPipe, ISP_CMOS_NOISE_CALIBRATION_S *pstNoiseProfileAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstNoiseProfileAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	ISP_CMOS_NOISE_CALIBRATION_S * p = CVI_NULL;

	isp_mgr_buf_get_np_addr(ViPipe, (CVI_VOID **) &p);

	memcpy(pstNoiseProfileAttr, p, sizeof(*pstNoiseProfileAttr));

	return CVI_SUCCESS;
}

//-----------------------------------------------------------------------------
//  DIS
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_SetDisAttr(VI_PIPE ViPipe, const ISP_DIS_ATTR_S *pstDisAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstDisAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_dis_ctrl_set_dis_attr(ViPipe, pstDisAttr);

	return ret;
}

CVI_S32 CVI_ISP_GetDisAttr(VI_PIPE ViPipe, ISP_DIS_ATTR_S *pstDisAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstDisAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_DIS_ATTR_S *pTemp = NULL;

	ret = isp_dis_ctrl_get_dis_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstDisAttr, pTemp, sizeof(ISP_DIS_ATTR_S));
	}

	return ret;
}

CVI_S32 CVI_ISP_SetDisConfig(VI_PIPE ViPipe, const ISP_DIS_CONFIG_S *pstDisConfig)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstDisConfig == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_dis_ctrl_set_dis_config(ViPipe, pstDisConfig);

	return ret;
}

CVI_S32 CVI_ISP_GetDisConfig(VI_PIPE ViPipe, ISP_DIS_CONFIG_S *pstDisConfig)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstDisConfig == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	const ISP_DIS_CONFIG_S *pTemp = NULL;

	ret = isp_dis_ctrl_get_dis_config(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstDisConfig, pTemp, sizeof(ISP_DIS_CONFIG_S));
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  VC
//-----------------------------------------------------------------------------
CVI_S32 CVI_ISP_SetVCAttr(VI_PIPE ViPipe, const ISP_VC_ATTR_S *pstVCAttr)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstVCAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
#ifndef ARCH_RTOS_CV181X
	ret = isp_motion_ctrl_set_motion_attr(ViPipe, pstVCAttr);
#else
	UNUSED(ViPipe);
	UNUSED(pstVCAttr);
#endif
	return ret;
}

CVI_S32 CVI_ISP_GetVCAttr(VI_PIPE ViPipe, ISP_VC_ATTR_S *pstVCAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstVCAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
#ifndef ARCH_RTOS_CV181X
	const ISP_VC_ATTR_S *pTemp = NULL;

	ret = isp_motion_ctrl_get_motion_attr(ViPipe, &pTemp);
	if (pTemp != NULL) {
		memcpy(pstVCAttr, pTemp, sizeof(ISP_VC_ATTR_S));
	}
#else
	UNUSED(ViPipe);
	UNUSED(pstVCAttr);
#endif
	return ret;
}

CVI_S32 CVI_ISP_IrAutoRunOnce(ISP_DEV IspDev, ISP_IR_AUTO_ATTR_S *pstIrAttr)
{
#define IR_WB_GAIN_FORMAT	256
#define	IR_WB_MAX_GAIN		4096
#define IR_DIV_0_TO_1(a)	((0 == (a)) ? 1 : (a))

	CHECK_DEV_VALUE(IspDev);
	if (pstIrAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (!pstIrAttr->bEnable) {
		pstIrAttr->enIrSwitch = ISP_IR_SWITCH_NONE;
		return CVI_SUCCESS;
	}

	ISP_EXP_INFO_S aeInfo;
	CVI_U16 grayWorldRgain, grayWorldBgain;
	CVI_U32 RGRatio, BGRatio, minRG, maxRG, minBG, maxBG;
	CVI_U32 normal2IrISO, ir2NormalISO;

	CVI_ISP_QueryExposureInfo(IspDev, &aeInfo);
	CVI_ISP_GetGrayWorldAwbInfo(IspDev, &grayWorldRgain, &grayWorldBgain);

	RGRatio = IR_WB_GAIN_FORMAT * 1024 / IR_DIV_0_TO_1(grayWorldRgain);
	BGRatio = IR_WB_GAIN_FORMAT * 1024 / IR_DIV_0_TO_1(grayWorldBgain);

	minRG = MIN(pstIrAttr->u32RGMin, IR_WB_MAX_GAIN);
	maxRG = MIN(pstIrAttr->u32RGMax, IR_WB_MAX_GAIN);
	minBG = MIN(pstIrAttr->u32BGMin, IR_WB_MAX_GAIN);
	maxBG = MIN(pstIrAttr->u32BGMax, IR_WB_MAX_GAIN);

	if (minRG > maxRG)
		minRG = maxRG;

	if (minBG > maxBG)
		minBG = maxBG;

	normal2IrISO = pstIrAttr->u32Normal2IrIsoThr;
	ir2NormalISO = pstIrAttr->u32Ir2NormalIsoThr;
	ir2NormalISO = (ir2NormalISO < 100) ? 100 : ir2NormalISO;
	if (normal2IrISO < ir2NormalISO)
		normal2IrISO = ir2NormalISO;

	if (pstIrAttr->enIrStatus == ISP_IR_STATUS_NORMAL) {
		pstIrAttr->enIrSwitch = (aeInfo.u32ISO >= normal2IrISO) ?
			ISP_IR_SWITCH_TO_IR : ISP_IR_SWITCH_NONE;
	} else {
		pstIrAttr->enIrSwitch = (aeInfo.u32ISO <= ir2NormalISO &&
			(RGRatio < minRG || RGRatio > maxRG ||
			BGRatio < minBG || BGRatio > maxBG)) ?
			ISP_IR_SWITCH_TO_NORMAL : ISP_IR_SWITCH_NONE;
	}

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISP_DumpHwRegisterToFile(VI_PIPE ViPipe, FILE *fp)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (fp == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	return ret;
}

CVI_S32 CVI_ISP_DumpFrameRawInfoToFile(VI_PIPE ViPipe, FILE *fp)
{
	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (fp == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_dbg_dumpFrameRawInfoToFile(ViPipe, fp);

	return ret;
}

CVI_S32 CVI_ISP_SyncSensorCfg(VI_PIPE ViPipe)
{
	return isp_snsSync_cfg_set(ViPipe);
}

CVI_S32 CVI_ISP_SetSmartInfo(VI_PIPE ViPipe, const ISP_SMART_INFO_S *pstSmartInfo, CVI_U8 TimeOut)
{
	return isp_3aLib_smart_info_set(ViPipe, pstSmartInfo, TimeOut);
}

CVI_S32 CVI_ISP_GetSmartInfo(VI_PIPE ViPipe, ISP_SMART_INFO_S *pstSmartInfo)
{
	return isp_3aLib_smart_info_get(ViPipe, pstSmartInfo);
}

CVI_S32 CVI_ISP_QueryWBInfo(VI_PIPE ViPipe, ISP_WB_INFO_S *pstWBInfo)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (pstWBInfo == CVI_NULL) {
		return CVI_FAILURE;
	}

	ISP_WB_Q_INFO_S			tWBQInfo;
	ISP_INNER_STATE_INFO_S	tInnerStateInfo;

	if (CVI_AWB_QueryInfo(ViPipe, &tWBQInfo) != CVI_SUCCESS) {
		return CVI_FAILURE;
	}

	isp_feature_ctrl_get_module_info(ViPipe, &tInnerStateInfo);

	for (uint32_t idx = 0; idx < CCM_MATRIX_SIZE; ++idx) {
		CVI_S16 s16CCM = (CVI_S16)tInnerStateInfo.ccm[idx];

		if (s16CCM >= 0) {
			pstWBInfo->au16CCM[idx] = s16CCM;
		} else {
			pstWBInfo->au16CCM[idx] = -s16CCM;
			pstWBInfo->au16CCM[idx] |= 0x8000;
		}
	}

	pstWBInfo->u16Rgain = tWBQInfo.u16Rgain;
	pstWBInfo->u16Grgain = tWBQInfo.u16Grgain;
	pstWBInfo->u16Gbgain = tWBQInfo.u16Gbgain;
	pstWBInfo->u16Bgain = tWBQInfo.u16Bgain;
	pstWBInfo->u16Saturation = tWBQInfo.u16Saturation;
	pstWBInfo->u16ColorTemp = tWBQInfo.u16ColorTemp;
	pstWBInfo->u16LS0CT = tWBQInfo.u16LS0CT;
	pstWBInfo->u16LS1CT = tWBQInfo.u16LS1CT;
	pstWBInfo->u16LS0Area = tWBQInfo.u16LS0Area;
	pstWBInfo->u16LS1Area = tWBQInfo.u16LS1Area;
	pstWBInfo->u8MultiDegree = tWBQInfo.u8MultiDegree;
	pstWBInfo->u16ActiveShift = tWBQInfo.u16ActiveShift;
	pstWBInfo->u32FirstStableTime = tWBQInfo.u32FirstStableTime;
	pstWBInfo->enInOutStatus = tWBQInfo.enInOutStatus;
	pstWBInfo->s16Bv = tWBQInfo.s16Bv;

	return CVI_SUCCESS;
}
