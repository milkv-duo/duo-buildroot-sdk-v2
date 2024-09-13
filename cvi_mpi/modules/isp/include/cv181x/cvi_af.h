/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: include/cvi_af.h
 * Description:
 */

#ifndef __CVI_AF_H__
#define __CVI_AF_H__

#include "cvi_comm_isp.h"
#include "cvi_comm_3a.h"
#include "cvi_af_comm.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

typedef enum _CVI_AF_STATUS {
	CVI_AF_NOT_INIT,
	CVI_AF_INIT,
	CVI_AF_TRIGGER_FOCUS,
	CVI_AF_DETECT_DIRECTION,
	CVI_AF_FIND_BEST_POS,
	CVI_AF_FOCUSED,
} CVI_AF_STATUS;

/* CVI_AF_MOTOR_Register:
 *    register motor control callback func
 * [in]
 *    pstAfMotorCb: motor control callback，customer can design it
 *    or use sophgo Gtype solution
 * [out]
 *    void
 * return: Function run success or not
 */
CVI_S32 CVI_AF_MOTOR_Register(VI_PIPE ViPipe, ISP_AF_MOTOR_FUNC_S *pstAfMotorCb);
/* CVI_AF_MOTOR_UnRegister:
 *    unregister motor control callback func
 * [in]
 *    pstAfMotorCb: motor control callback，customer can design it
 *    or use sophgo Gtype solution
 * [out]
 *    void
 * return: Function run success or not
 */
CVI_S32 CVI_AF_MOTOR_UnRegister(VI_PIPE ViPipe, ISP_AF_MOTOR_FUNC_S *pstAfMotorCb);
/* CVI_AF_Register:
 *    register af algo lib
 * [in]
 *    ViPipe: Correspond to sensor
 *    pstAfLib: af algo lib
 * [out]
 *    void
 * return: Function run success or not
 */
CVI_S32 CVI_AF_Register(VI_PIPE ViPipe, ALG_LIB_S *pstAfLib);
/* CVI_AF_UnRegister:
 *    unregister af algo lib
 * [in]
 *    ViPipe: Correspond to sensor
 * [out]
 *    void
 * return: Function run success or not
 */
CVI_S32 CVI_AF_UnRegister(VI_PIPE ViPipe, ALG_LIB_S *pstAfLib);
/* CVI_AF_AutoFocus:
 *    start auto focus
 * [in]
 *    sID: get it by ViPipe
 * [out]
 *    void
 * return: Function run success or not
 */
CVI_S32 CVI_AF_AutoFocus(VI_PIPE ViPipe);
/* CVI_AF_GetFv:
 *    get the newest frame af fv value
 * [in]
 *    sID: get it by ViPipe
 * [out]
 *    pFv: Focus value
 * return: Function run success or not
 */
CVI_S32 CVI_AF_GetFv(VI_PIPE ViPipe, CVI_U32 *pFv);
/* CVI_AF_SetZoomSpeed:
 *    set Zoom motor rotational speed, generally not set
 * [in]
 *    sID: get it by ViPipe
 *	  eSpeed: rotational speed:4x/2x/1x/half
 * [out]
 *    void
 * return: Function run success or not
 */
CVI_S32 CVI_AF_SetZoomSpeed(VI_PIPE ViPipe, ISP_AF_MOTOR_SPEED_E eSpeed);
/* CVI_AF_SetZoom:
 *    set zoom motor rotational speed, generally not set
 * [in]
 *    sID: get it by ViPipe
 *	  eDir: rotational direction
 *    step: rotational step
 * [out]
 *    void
 * return: Function run success or not
 */
CVI_S32 CVI_AF_SetZoom(VI_PIPE ViPipe, CVI_BOOL direct, CVI_U8 step);
/* CVI_AF_SetFocusSpeed:
 *    set focus motor rotational speed
 * [in]
 *    sID: get it by ViPipe
 *    eSpeed: rotational speed:4x/2x/1x/half
 * [out]
 *    void
 * return: Function run success or not
 */
CVI_S32 CVI_AF_SetFocusSpeed(VI_PIPE ViPipe, ISP_AF_MOTOR_SPEED_E eSpeed);
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
CVI_S32 CVI_AF_SetFocus(VI_PIPE ViPipe, CVI_BOOL direct, CVI_U8 step);
/* CVI_AF_SetZoomAndFocus:
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
CVI_S32 CVI_AF_SetZoomAndFocus(VI_PIPE ViPipe, CVI_BOOL direct, CVI_U8 zoomStep, CVI_U8 focusStep);
/* CVI_AF_SetAttr:
 *    set af algo control param
 * [in]
 *    sID: get it by ViPipe
 *	  pstFocusAttr: control param
 * [out]
 *    void
 * return: Function run success or not
 */
CVI_S32 CVI_AF_SetAttr(VI_PIPE ViPipe, const ISP_FOCUS_ATTR_S *pstFocusAttr);
/* CVI_AF_GetAttr:
 *    get af algo control param
 * [in]
 *    sID: get it by ViPipe
 * [out]
 *	  pstFocusAttr: control param
 * return: Function run success or not
 */
CVI_S32 CVI_AF_GetAttr(VI_PIPE ViPipe, ISP_FOCUS_ATTR_S *pstFocusAttr);
/* CVI_ISP_QueryFocusInfo:
 *    get af algo status info
 * [in]
 *    sID: get it by ViPipe
 * [out]
 *	  pstFocusQInfo: af status info
 * return: Function run success or not
 */
CVI_S32 CVI_ISP_QueryFocusInfo(VI_PIPE ViPipe, ISP_FOCUS_Q_INFO_S *pstFocusQInfo);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* __CVI_AF_H__ */
