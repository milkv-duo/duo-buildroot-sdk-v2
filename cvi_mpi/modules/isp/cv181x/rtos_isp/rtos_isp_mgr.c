#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"

#include "isp_main_local.h"

#include "isp_debug.h"
#include "isp_defines.h"
#include "cvi_comm_isp.h"
#include "isp_feature_ctrl.h"
#include "isp_sts_ctrl.h"
#include "rtos_isp_cmd.h"

ISP_CTX_S *g_astIspCtx[VI_MAX_PIPE_NUM];
//ISP_PARAMETER_BUFFER g_param[VI_MAX_PIPE_NUM];

//struct rtos_isp_mgr_ctrl_runtime {
//
//};
//struct rtos_isp_mgr_ctrl_runtime *mgr_ctrl_runtime[VI_MAX_PIPE_NUM];
//
//static struct rtos_isp_mgr_ctrl_runtime **_get_mgr_ctrl_runtime(VI_PIPE ViPipe);

//static CVI_S32 _global_init(VI_PIPE ViPipe);
//
CVI_S32 rtos_isp_mgr_init(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	//struct rtos_isp_mgr_ctrl_runtime *runtime = _get_mgr_ctrl_runtime(ViPipe);

	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	//_global_init(ViPipe);

	isp_feature_ctrl_init(ViPipe);

	for (CVI_U32 u32AlgoIdx = 0; u32AlgoIdx < AAA_TYPE_MAX; ++u32AlgoIdx)
		isp_3aLib_init(ViPipe, u32AlgoIdx);

	isp_iqParam_addr_initDefault(ViPipe);

	isp_sns_info_init(ViPipe);

	return ret;
}

CVI_S32 rtos_isp_mgr_uninit(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;

	isp_sensor_exit(ViPipe);
	for (AAA_LIB_TYPE_E type = 0; type < AAA_TYPE_MAX; type++) {
		isp_3aLib_exit(ViPipe, type);
	}
	isp_sts_ctrl_exit(ViPipe);
	isp_feature_ctrl_uninit(ViPipe);

	return ret;
}

CVI_S32 rtos_isp_mgr_pre_sof(VI_PIPE ViPipe, CVI_U32 frame_idx)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	//isp_snsSync_cfg_set(ViPipe);

	UNUSED(ViPipe);
	UNUSED(frame_idx);

	return ret;
}

CVI_S32 rtos_isp_mgr_pre_eof(VI_PIPE ViPipe, CVI_U32 frame_idx)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);
	UNUSED(frame_idx);

	return ret;
}

CVI_S32 rtos_isp_mgr_pre_fe_sof(VI_PIPE ViPipe, CVI_U32 frame_idx)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);
	UNUSED(frame_idx);

	return ret;
}

CVI_S32 rtos_isp_mgr_pre_fe_eof(VI_PIPE ViPipe, CVI_U32 frame_idx)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	// isp_sts_ctrl_pre_fe_eof(ViPipe);

	UNUSED(ViPipe);
	UNUSED(frame_idx);

	return ret;
}

CVI_S32 rtos_isp_mgr_pre_be_sof(VI_PIPE ViPipe, CVI_U32 frame_idx)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);
	UNUSED(frame_idx);

	return ret;
}

CVI_S32 rtos_isp_mgr_pre_be_eof(VI_PIPE ViPipe, CVI_U32 frame_idx)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	// isp_sts_ctrl_pre_be_eof(ViPipe, frame_idx);

	UNUSED(ViPipe);
	UNUSED(frame_idx);

	return ret;
}

CVI_S32 rtos_isp_mgr_post_sof(VI_PIPE ViPipe, CVI_U32 frame_idx)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);
	UNUSED(frame_idx);

	return ret;
}

CVI_S32 rtos_isp_mgr_post_eof(VI_PIPE ViPipe, CVI_U32 frame_idx)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	//isp_3a_frameCnt_set(ViPipe, frame_idx);

	//isp_sts_ctrl_post_eof(ViPipe, frame_idx);

	//for (CVI_U32 u32AlgoIdx = 0; u32AlgoIdx < AAA_TYPE_MAX; ++u32AlgoIdx)
	//	isp_3aLib_run(ViPipe, u32AlgoIdx);

	isp_tun_buf_invalid(ViPipe);

	isp_feature_ctrl_post_eof(ViPipe);

	isp_tun_buf_flush(ViPipe);

	UNUSED(ViPipe);
	UNUSED(frame_idx);

	return ret;
}

//static CVI_S32 _global_init(VI_PIPE ViPipe)
//{
//	//CVI_U32 i;
//	//CVI_S32 s32Ret = 0;
//	ISP_CTX_S *pstIspCtx;
//
//	ISP_GET_CTX(ViPipe, pstIspCtx);
//	pstIspCtx->frameCnt = 0;
//
//	pstIspCtx->frameInfo.u32Again = pstIspCtx->frameInfo.u32Dgain =
//		pstIspCtx->frameInfo.u32IspDgain = 0;
//
//	pstIspCtx->u8AERunInterval = 1;
//	pstIspCtx->ispProcCtrl.u32AEStatIntvl = 1;
//	pstIspCtx->ispProcCtrl.u32AWBStatIntvl = 1;
//	pstIspCtx->ispProcCtrl.u32AFStatIntvl = 0;
//	//we default not get AF sts, if u want, u should call CVI_ISP_SetCtrlParam
//	pstIspCtx->bIspRun = CVI_FALSE;
//
//	pstIspCtx->wdrLinearMode = !(IS_2to1_WDR_MODE(pstIspCtx->u8SnsWDRMode));
//
//	return CVI_SUCCESS;
//}

//static struct rtos_isp_mgr_ctrl_runtime  **_get_mgr_ctrl_runtime(VI_PIPE ViPipe)
//{
//	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));
//
//	if (!isVipipeValid) {
//		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
//		return NULL;
//	}
//
//	return &mgr_ctrl_runtime[ViPipe];
//}

