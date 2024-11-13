/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: afalgo.h
 * Description:
 *
 */

#ifndef _AF_ALG_H_
#define _AF_ALG_H_

#include "cvi_comm_3a.h"
#include "cvi_comm_isp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#ifndef UNUSED
#define UNUSED(x)	(void)(x)
#endif

#define AF_SENSOR_NUM (VI_MAX_PIPE_NUM)
#define AAA_LIMIT(var, min, max) ((var) = ((var) < (min)) ? (min) : (((var) > (max)) ? (max) : (var)))
#define AAA_ABS(a) ((a) > 0 ? (a) : -(a))
#define AAA_MIN(a, b) ((a) < (b) ? (a) : (b))
#define AAA_MAX(a, b) ((a) > (b) ? (a) : (b))
#define AAA_DIV_0_TO_1(a) ((0 == (a)) ? 1 : (a))
#define TYPE_BIT (9)
#define DIRECTION_BIT (8)

typedef CVI_U16 QDataType;

typedef struct QListNode {
	struct QListNode *_next;
	QDataType _data;
} QNode;

typedef struct Queue {
	QNode *_front;
	QNode *_rear;
	int size;
} Queue;

typedef enum _AF_CTL_TYPE {
	AF_CTL_FOCUS,
	AF_CTL_ZOOM,
} AF_CTL_TYPE;

typedef enum _AF_POS_FLAG {
	AF_POS_NOT_INIT,
	AF_POS_RESET,
	AF_POS_INIT
} AF_POS_FLAG;

typedef enum _AF_CHASING_FOCUS_FLAG {
	AF_CHASING_FOCUS_DETECT_RISING,
	AF_CHASING_FOCUS_FIND_PEAK,
} AF_CHASING_FOCUS_FLAG;

typedef enum _AF_DBG_MODE {
	AF_DBG_DISABLE,
	AF_DBG_CODE_FLOW,
} AF_DBG_MODE;

typedef struct _AF_CTX_S {
	//common param
	CVI_BOOL bAeByPassSts;
	AF_POS_FLAG   ePosFlag; //0:not init 1:reset 2:init pos
	AF_CHASING_FOCUS_FLAG eChasingFocusFlag;
	CVI_U16 u16ZoomOffsetCnt;
	CVI_U16 u16FocusOffsetCnt;
	Queue q;
	CVI_U16 u16MaxZoomStep; //auto cal by frameRate
	CVI_U16 u16MaxFocusStep; //auto cal by frameRate
	//auto focus param
	ISP_FOCUS_Q_INFO_S *pstFocusQInfo;
	CVI_BOOL bInitFocusPos;
	CVI_BOOL bInitZoomPos;
	CVI_U16 u16FocusRevStep;
	CVI_U16 u16ZoomRevStep;
	CVI_BOOL bChasingFocus;
	//hw param
	ISP_AF_LEN_INFO_S stLenInfo;
	//calib param
	CVI_BOOL bCailb;
} AF_CTX_S;

typedef enum _AF_PARAMETER_UPDATE {
	AF_UPDATE_ATTR,
	AF_UPDATE_STATISTICS_CONFIG,
	AF_UPDATE_ALL,
	AF_UPDATE_TOTAL,
} AF_PARAMETER_UPDATE;

/* AF_MOTOR_Register:
 *    register motor control callback func
 * [in]
 *    pstAfMotorCb: motor control callback，customer can design it
 *    or use sophgo Gtype solution
 * [out]
 *    void
 * return: Function run success or not
 */
CVI_S32 AF_MOTOR_Register(VI_PIPE ViPipe, ISP_AF_MOTOR_FUNC_S *pstAfMotorCb);
/* AF_MOTOR_UnRegister:
 *    unregister motor control callback func
 * [in]
 *    pstAfMotorCb: motor control callback，customer can design it
 *    or use sophgo Gtype solution
 * [out]
 *    void
 * return: Function run success or not
 */
CVI_S32 AF_MOTOR_UnRegister(VI_PIPE ViPipe, ISP_AF_MOTOR_FUNC_S *pstAfMotorCb);
/* AF_GET_MOTOR_CB:
 *    Get global af motor cb func by ViPipe
 * [in]
 *    ViPipe: Correspond to sensor
 * [out]
 *    void
 * return: Cur Vipipe ISP_AF_MOTOR_FUNC_S obj
 */
ISP_AF_MOTOR_FUNC_S *AF_GET_MOTOR_CB(VI_PIPE ViPipe);
/* AF_GET_CTX:
 *    Get global af ctx by ViPipe
 * [in]
 *    ViPipe: Correspond to sensor
 * [out]
 *    void
 * return: Cur Vipipe ctx
 */
AF_CTX_S *AF_GET_CTX(VI_PIPE ViPipe);
/* AF_ViPipe2sID:
 *    Check ViPipe
 * [in]
 *    ViPipe: Correspond to sensor
 * [out]
 *    void
 * return: sID
 */
CVI_U8 AF_ViPipe2sID(VI_PIPE ViPipe);
/* afInit:
 *    init af algo
 * [in]
 *    ViPipe: Correspond to sensor
 *    pstAfParam: sensor attribute
 * [out]
 *    void
 * return: Function run success or not
 */
CVI_S32 afInit(VI_PIPE ViPipe, const ISP_AF_PARAM_S *pstAfParam);
/* afRun:
 *    run af algo, trigger it by vi event
 * [in]
 *    ViPipe: Correspond to sensor
 *    pstAfInfo: af sts and frame rate info
 * [out]
 *    pstAfResult: not use now
 * return: Function run success or not
 */
CVI_S32 afRun(VI_PIPE ViPipe, const ISP_AF_INFO_S *pstAfInfo, ISP_AF_RESULT_S *pstAfResult, CVI_S32 s32Rsv);
/* afCtrl:
 *    ioctl af algo, not use now
 * [in]
 *    ViPipe: Correspond to sensor
 *    u32Cmd: ioctl cmd
 *    pValue: ioctl value
 * [out]
 *    void
 * return: Function run success or not
 */
CVI_S32 afCtrl(VI_PIPE ViPipe, CVI_U32 u32Cmd, CVI_VOID *pValue);
/* afExit:
 *    exit af algo
 * [in]
 *    ViPipe: Correspond to sensor
 * [out]
 *    void
 * return: Function run success or not
 */
CVI_S32 afExit(VI_PIPE ViPipe);
/* AF_GetAlgoVer:
 *    get af algo version
 * [in]
 *    pVer: main version
 *    pSubVer: sub version
 * [out]
 *    void
 * return: void
 */
void AF_GetAlgoVer(CVI_U16 *pVer, CVI_U16 *pSubVer);
/* AF_CalFv:
 *    cal af fv value
 * [in]
 *    sID: get it by ViPipe
 *    pstAfInfo: af sts
 * [out]
 *    void
 * return: Function run success or not
 */
CVI_S32 AF_CalFv(CVI_U8 sID, const ISP_AF_INFO_S *pstAfInfo);
/* AF_GetFv:
 *    get the newest frame af fv value
 * [in]
 *    sID: get it by ViPipe
 * [out]
 *    fv: Focus value
 * return: Function run success or not
 */
CVI_S32 AF_GetFv(CVI_U8 sID, CVI_U32 *fv);
/* AF_AutoFocus:
 *    start auto focus
 * [in]
 *    sID: get it by ViPipe
 * [out]
 *    void
 * return: Function run success or not
 */
CVI_S32 AF_AutoFocus(CVI_U8 sID);
/* AF_SetZoomSpeed:
 *    set Zoom motor rotational speed, generally not set
 * [in]
 *    sID: get it by ViPipe
 *    eSpeed: rotational speed
 * [out]
 *    void
 * return: Function run success or not
 */
CVI_S32 AF_SetZoomSpeed(CVI_U8 sID, ISP_AF_MOTOR_SPEED_E eSpeed);
/* AF_SetZoom:
 *    set zoom motor rotational speed, generally not set
 * [in]
 *    sID: get it by ViPipe
 *    eDir: rotational direction
 *    step: rotational step
 * [out]
 *    void
 * return: Function run success or not
 */
CVI_S32 AF_SetZoom(CVI_U8 sID, AF_DIRECTION eDir, CVI_U8 step);
/* AF_SetFocusSpeed:
 *    set focus motor rotational speed
 * [in]
 *    sID: get it by ViPipe
 *    eSpeed: rotational speed
 * [out]
 *    void
 * return: Function run success or not
 */
CVI_S32 AF_SetFocusSpeed(CVI_U8 sID, ISP_AF_MOTOR_SPEED_E eSpeed);
/* AF_SetFocus:
 *    set focus motor rotational speed, generally not set
 * [in]
 *    sID: get it by ViPipe
 *	  eDir: rotational direction
 *    step: rotational step
 * [out]
 *    void
 * return: Function run success or not
 */
CVI_S32 AF_SetFocus(CVI_U8 sID, AF_DIRECTION eDir, CVI_U8 step);
/* AF_SetZoomAndFocus:
 *    set both zoom motor and focus motor rotational speed
 * [in]
 *    sID: get it by ViPipe
 *	  eDir: rotational direction
 *    zoomStep: rotational step
 *    focusStep: rotational step
 * [out]
 *    void
 * return: Function run success or not
 */
CVI_S32 AF_SetZoomAndFocus(CVI_U8 sID, AF_DIRECTION eDir, CVI_U8 zoomStep, CVI_U8 focusStep);
/* AF_SetAttr:
 *    set af algo control param
 * [in]
 *    sID: get it by ViPipe
 *	  pstFocusAttr: control param
 * [out]
 *    void
 * return: Function run success or not
 */
CVI_S32 AF_SetAttr(CVI_U8 sID, const ISP_FOCUS_ATTR_S *pstFocusAttr);
/* AF_GetAttr:
 *    get af algo control param
 * [in]
 *    sID: get it by ViPipe
 * [out]
 *	  pstFocusAttr: control param
 * return: Function run success or not
 */
CVI_S32 AF_GetAttr(CVI_U8 sID, ISP_FOCUS_ATTR_S *pstFocusAttr);
/* AF_GetQueryInfo:
 *    get af algo status info
 * [in]
 *    sID: get it by ViPipe
 * [out]
 *	  pstFocusQInfo: af status info
 * return: Function run success or not
 */
CVI_S32 AF_GetQueryInfo(CVI_U8 sID, ISP_FOCUS_Q_INFO_S *pstFocusQInfo);
/* AF_SetStatisticsConfig:
 *    set af static config to af algo
 * [in]
 *    sID: get it by ViPipe
 *    pstAfStatCfg: af static config
 * [out]
 *	  void
 * return: Function run success or not
 */
CVI_S32 AF_SetStatisticsConfig(CVI_U8 sID, const ISP_FOCUS_STATISTICS_CFG_S *pstAfStatCfg);
/* AF_SetStatisticsConfig:
 *    get af static config from af algo
 * [in]
 *    sID: get it by ViPipe
 * [out]
 *    pstAfStatCfg: af static config
 * return: Function run success or not
 */
CVI_S32 AF_GetStatisticsConfig(CVI_U8 sID, ISP_FOCUS_STATISTICS_CFG_S *pstAfStatCfg);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _AF_ALG_H_
