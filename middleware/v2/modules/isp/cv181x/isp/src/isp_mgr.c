#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"

#include "cvi_base.h"

#include "isp_sts_ctrl.h"
#include "isp_feature_ctrl.h"
#include "isp_tun_buf_ctrl.h"
#include "isp_mgr_buf.h"
#include "isp_control.h"
#include "isp_freeze.h"
#include "isp_mailbox.h"
#include "isp_proc_local.h"

#include "isp_debug.h"
#include "isp_defines.h"
#include "isp_mw_compat.h"
#include "cvi_comm_isp.h"
#include "rtos_isp_cmd.h"

ISP_CTX_S *g_astIspCtx[VI_MAX_PIPE_NUM];
//ISP_PARAMETER_BUFFER g_param[VI_MAX_PIPE_NUM];

struct isp_mgr_ctrl_runtime {
	CVI_S32 foo;
};
struct isp_mgr_ctrl_runtime mgr_ctrl_runtime[VI_MAX_PIPE_NUM];

static struct isp_mgr_ctrl_runtime *_get_mgr_ctrl_runtime(VI_PIPE ViPipe);

static CVI_S32 _global_init(VI_PIPE ViPipe);
static CVI_S32 _global_exit(VI_PIPE ViPipe);
//static CVI_S32 check_enable_debug_message_to_stdout(void);
static CVI_S32 apply_debug_level_from_environment(void);
static CVI_S32 set_sensorname_to_environment(VI_PIPE ViPipe);

CVI_S32 isp_mgr_init(VI_PIPE ViPipe)
{
	apply_debug_level_from_environment();
	//check_enable_debug_message_to_stdout();

	isp_dbg_init();

#ifndef ISP_VERSION
#define ISP_VERSION "-"
#endif
	ISP_LOG_WARNING("ISP VERSION(%s)\n", ISP_VERSION);

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_mgr_ctrl_runtime *runtime = _get_mgr_ctrl_runtime(ViPipe);

	ISP_LOG_DEBUG("+\n");
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	_global_init(ViPipe);

	isp_control_set_fd_info(ViPipe);

	isp_tun_buf_ctrl_init(ViPipe);
#ifdef ENABLE_ISP_PROC_DEBUG
	isp_proc_buf_init(ViPipe);
#endif
	isp_feature_ctrl_init(ViPipe);

	isp_sts_ctrl_init(ViPipe);

	for (CVI_U32 u32AlgoIdx = 0; u32AlgoIdx < AAA_TYPE_MAX; ++u32AlgoIdx)
		isp_3aLib_init(ViPipe, u32AlgoIdx);

	/*Move here for check wdr mode.*/
	// BIN file will overwrite AE/AWB settings
	isp_iqParam_addr_initDefault(ViPipe);
	isp_sns_info_init(ViPipe);
	isp_freeze_init(ViPipe);

	isp_mailbox_init(ViPipe);

	UNUSED(runtime);

	/*set env variable to make libbin be able to get sensor name.*/
	set_sensorname_to_environment(ViPipe);

#ifdef ENABLE_ISP_C906L
	isp_mailbox_send_cmd_init(ViPipe);
#endif
	return ret;
}

CVI_S32 isp_mgr_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	ISP_CTX_S *pstIspCtx = NULL;

	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	ISP_GET_CTX(ViPipe, pstIspCtx);
#ifdef ENABLE_ISP_PROC_DEBUG
	isp_proc_exit();
#endif
	int count = 40;

	while (pstIspCtx->bIspRun && (--count > 0))
		USLEEP_COMPAT(50 * 1000);

	if (pstIspCtx->bIspRun) {
		ISP_LOG_DEBUG("ISP%d incorrect\n", ViPipe);
	}

	isp_freeze_uninit(ViPipe);
	isp_sensor_exit(ViPipe);
	for (AAA_LIB_TYPE_E type = 0; type < AAA_TYPE_MAX; type++) {
		isp_3aLib_exit(ViPipe, type);
	}

	isp_sts_ctrl_exit(ViPipe);
	isp_feature_ctrl_uninit(ViPipe);
#ifdef ENABLE_ISP_PROC_DEBUG
	isp_proc_buf_deinit(ViPipe);
#endif
	isp_tun_buf_ctrl_uninit(ViPipe);
	_global_exit(ViPipe);

#ifdef ENABLE_ISP_C906L
	isp_mailbox_send_cmd_deinit(ViPipe);
#endif
	isp_mailbox_uninit(ViPipe);

	isp_dbg_deinit();

	return ret;
}

CVI_S32 isp_mgr_pre_sof(VI_PIPE ViPipe, CVI_U32 frame_idx)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	ISP_CTX_S *pstIspCtx = NULL;

	ISP_GET_CTX(ViPipe, pstIspCtx);

#ifdef ENABLE_ISP_C906L
	isp_mailbox_send_cmd_event(ViPipe, RTOS_ISP_CMD_PRE_SOF, frame_idx);
#endif
	isp_flow_event_siganl(ViPipe, ISP_VD_FE_START);
	isp_control_set_scene_info(ViPipe);

	UNUSED(frame_idx);

	pstIspCtx->enPreViEvent = ViPipe == 0 ? VI_EVENT_PRE0_SOF : VI_EVENT_PRE1_SOF;

	return ret;
}

CVI_S32 isp_mgr_pre_eof(VI_PIPE ViPipe, CVI_U32 frame_idx)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	ISP_CTX_S *pstIspCtx = NULL;

	ISP_GET_CTX(ViPipe, pstIspCtx);

#ifdef ENABLE_ISP_C906L
	isp_mailbox_send_cmd_event(ViPipe, RTOS_ISP_CMD_PRE_EOF, frame_idx);
#endif
	isp_flow_event_siganl(ViPipe, ISP_VD_FE_END);

	isp_sts_ctrl_pre_fe_eof(ViPipe);
	isp_sts_ctrl_pre_be_eof(ViPipe, frame_idx);
	isp_feature_ctrl_pre_fe_eof(ViPipe);
	isp_feature_ctrl_pre_be_eof(ViPipe);

	pstIspCtx->enPreViEvent = ViPipe == 0 ? VI_EVENT_PRE0_EOF : VI_EVENT_PRE1_EOF;

	return ret;
}

CVI_S32 isp_mgr_pre_fe_sof(VI_PIPE ViPipe, CVI_U32 frame_idx)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	// isp_sts_ctrl_pre_fe_eof(ViPipe);

	UNUSED(ViPipe);
	UNUSED(frame_idx);

	return ret;
}

CVI_S32 isp_mgr_pre_fe_eof(VI_PIPE ViPipe, CVI_U32 frame_idx)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);
	UNUSED(frame_idx);

	return ret;
}

CVI_S32 isp_mgr_pre_be_sof(VI_PIPE ViPipe, CVI_U32 frame_idx)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);
	UNUSED(frame_idx);

	return ret;
}

CVI_S32 isp_mgr_pre_be_eof(VI_PIPE ViPipe, CVI_U32 frame_idx)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);
	UNUSED(frame_idx);

	return ret;
}

CVI_S32 isp_mgr_post_sof(VI_PIPE ViPipe, CVI_U32 frame_idx)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);
	UNUSED(frame_idx);

	return ret;
}

CVI_S32 isp_mgr_post_eof(VI_PIPE ViPipe, CVI_U32 frame_idx)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	ISP_CTX_S *pstIspCtx = NULL;
	ISP_FMW_STATE_E state;
	CVI_BOOL bFreezeState = CVI_FALSE;

	struct timeval tv1, tv2;

	ISP_GET_CTX(ViPipe, pstIspCtx);
	pstIspCtx->frameCnt++;
	isp_freeze_get_fw_state(ViPipe, &state);
	isp_3a_frameCnt_set(ViPipe, pstIspCtx->frameCnt);
	isp_flow_event_siganl(ViPipe, ISP_VD_BE_END);

	pstIspCtx->bWDRSwitchFinish = 1;

	CVI_BOOL FW_RUN = (
		(state == ISP_FMW_STATE_RUN) &&
		(pstIspCtx->frameCnt > pstIspCtx->u8AEWaitFrame));

	if (FW_RUN == CVI_TRUE) {

		isp_sts_ctrl_post_eof(ViPipe, frame_idx);

		gettimeofday(&tv1, NULL);

		for (CVI_U32 u32AlgoIdx = 0; u32AlgoIdx < AAA_TYPE_MAX; ++u32AlgoIdx)
			isp_3aLib_run(ViPipe, u32AlgoIdx);

		isp_snsSync_cfg_set(ViPipe);
		gettimeofday(&tv2, NULL);
		ISP_LOG_INFO("run 3a lib diffus: %d\n", isp_dbg_get_time_diff_us(&tv1, &tv2));

		isp_freeze_set_freeze(ViPipe, pstIspCtx->frameCnt,
			(pstIspCtx->stAeResult.bStable && pstIspCtx->stAwbResult.bStable));

		// isp_3a_update_proc_info(ViPipe);

		gettimeofday(&tv1, NULL);
#ifdef	ENABLE_ISP_C906L
		isp_mailbox_send_cmd_event(ViPipe, RTOS_ISP_CMD_POST_EOF, frame_idx);
		isp_tun_buf_invalid(ViPipe);
#endif
		isp_feature_ctrl_post_eof(ViPipe);
#ifdef ENABLE_ISP_C906L
		//isp_mailbox_send_cmd_sync_post_done(ViPipe, frame_idx);
#endif
		isp_tun_buf_ctrl_frame_done(ViPipe);
		gettimeofday(&tv2, NULL);
		ISP_LOG_INFO("run feature ctrl diffus: %d\n", isp_dbg_get_time_diff_us(&tv1, &tv2));

	} else {
		isp_freeze_get_state(ViPipe, &bFreezeState);

		if (bFreezeState) {
			isp_freeze_set_mute(ViPipe);
		}
	}

	//update information from temp struct to proc struct
#ifdef ENABLE_ISP_PROC_DEBUG
	if (isp_proc_getProcStatus() == PROC_STATUS_RECEVENT) {
		isp_proc_updateData2ProcST(ViPipe);
		isp_proc_setProcStatus(PROC_STATUS_UPDATEDATA);
	}
#endif
	pstIspCtx->enPreViEvent = ViPipe == 0 ? VI_EVENT_POST_EOF : VI_EVENT_POST1_EOF;

	return ret;
}

static CVI_S32 _global_init(VI_PIPE ViPipe)
{
	CVI_U32 i;
	CVI_S32 s32Ret = 0;
	ISP_CTX_S *pstIspCtx;

	ISP_GET_CTX(ViPipe, pstIspCtx);
	pstIspCtx->frameCnt = 0;

	// os param initial
	s32Ret = pthread_mutex_init(&pstIspCtx->ispEventLock, 0);
	if (s32Ret < 0) {
		ISP_LOG_ERR("ispEventLock initial fail with %#x.\n", s32Ret);
		return CVI_FAILURE;
	}
	for (i = 0 ; i < ISP_VD_MAX ; i++) {
		s32Ret = pthread_cond_init(&pstIspCtx->ispEventCond[i], 0);
		if (s32Ret < 0) {
			ISP_LOG_ERR("ispEventCond %d initial fail with %#x.\n", i, s32Ret);
			return CVI_FAILURE;
		}
	}

	pstIspCtx->frameInfo.u32Again = pstIspCtx->frameInfo.u32Dgain =
		pstIspCtx->frameInfo.u32IspDgain = 0;

	pstIspCtx->u8AERunInterval = 1;
	pstIspCtx->ispProcCtrl.u32AEStatIntvl = 1;
	pstIspCtx->ispProcCtrl.u32AWBStatIntvl = 6;
	pstIspCtx->ispProcCtrl.u32AFStatIntvl = 1;
	//we default not get AF sts, if u want, u should call CVI_ISP_SetCtrlParam
	pstIspCtx->bIspRun = CVI_FALSE;

	pstIspCtx->wdrLinearMode = !(IS_2to1_WDR_MODE(pstIspCtx->u8SnsWDRMode));

	return CVI_SUCCESS;
}

static CVI_S32 _global_exit(VI_PIPE ViPipe)
{
	CVI_U32 i;
	CVI_S32 s32Ret = 0;
	ISP_CTX_S *pstIspCtx;

	ISP_GET_CTX(ViPipe, pstIspCtx);
	// os param destroy.
	s32Ret = pthread_mutex_destroy(&pstIspCtx->ispEventLock);
	if (s32Ret < 0) {
		ISP_LOG_ERR("ispEventLock destroy fail with %#x.\n", s32Ret);
		return CVI_FAILURE;
	}
	for (i = 0 ; i < ISP_VD_MAX ; i++) {
		pthread_cond_destroy(&pstIspCtx->ispEventCond[i]);
		if (s32Ret < 0) {
			ISP_LOG_ERR("ispEventCond %d destroy fail with %#x.\n", i, s32Ret);
			return CVI_FAILURE;
		}
	}
	pstIspCtx->bWDRSwitchFinish = 0;
	return CVI_SUCCESS;
}

//static CVI_S32 check_enable_debug_message_to_stdout(void)
//{
//#define ENV_DEBUG_MESSAGE_TO_STDOUT		"ISP_ENABLE_STDOUT_DEBUG"
//	char *pszEnv = getenv(ENV_DEBUG_MESSAGE_TO_STDOUT);
//	char *pszEnd = NULL;
//
//	if (pszEnv == NULL)
//		return CVI_FAILURE;
//
//	CVI_U32 u32Level = (CVI_U32)strtol(pszEnv, &pszEnd, 10);
//
//	if (pszEnv == pszEnd) {
//		ISP_LOG_DEBUG("ISP_ENABLE_STDOUT_DEBUG invalid\n");
//		return CVI_FAILURE;
//	}
//
//	gExportToStdout = (u32Level > 0) ? CVI_TRUE : CVI_FALSE;
//	return CVI_SUCCESS;
//}

static CVI_S32 apply_debug_level_from_environment(void)
{
#define ENV_DEBUG_LEVEL		"ISP_DEBUG_LEVEL"
	char *pszEnv = getenv(ENV_DEBUG_LEVEL);
	char *pszEnd = NULL;

	if (pszEnv == NULL)
		return CVI_FAILURE;

	CVI_UL ulLevel = strtol(pszEnv, &pszEnd, 10);

	if ((ulLevel > LOG_DEBUG) || (pszEnv == pszEnd)) {
		ISP_LOG_DEBUG("ISP_DEBUG_LEVEL invalid\n");
		return CVI_FAILURE;
	}

	CVI_DEBUG_SetDebugLevel((int)ulLevel);
	return CVI_SUCCESS;
}

static CVI_S32 set_sensorname_to_environment(VI_PIPE ViPipe)
{
	#define NAME_SIZE 20
	#define ENV_SIZE 20
	CVI_CHAR sensorNameEnv[NAME_SIZE] = {0}, chEnvValue[ENV_SIZE] = {0};
	SENSOR_ID sensorId = 0;
	CVI_S32 ret = 0;

	isp_sensor_getId(ViPipe, &sensorId);
	snprintf(chEnvValue, ENV_SIZE, "%d", sensorId);
	snprintf(sensorNameEnv, NAME_SIZE, "SENSORNAME%d", ViPipe);
	ret = setenv(sensorNameEnv, chEnvValue, 1);
	if (ret != 0) {
		ISP_LOG_WARNING("set env value:%s fail.\n", chEnvValue);
	}

	return 0;
}

// static CVI_S32 _send_linux_info_to_rtos(VI_PIPE ViPipe)
// {

// }

static struct isp_mgr_ctrl_runtime  *_get_mgr_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	return &mgr_ctrl_runtime[ViPipe];
}
