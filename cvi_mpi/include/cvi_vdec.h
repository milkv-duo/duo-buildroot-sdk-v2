/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: include/cvi_vdec.h
 * Description:
 *   Common video decode definitions.
 */

#ifndef __CVI_VDEC_H__
#define __CVI_VDEC_H__

#include <linux/cvi_common.h>
#include <linux/cvi_comm_video.h>
#include "cvi_comm_vb.h"
#include <linux/cvi_comm_vdec.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

/* Create a channel.
 *
 * @param VdChn(In): channel number
 * @param pstAttr(In): pointer to VDEC_CHN_ATTR_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VDEC_CreateChn(VDEC_CHN VdChn, const VDEC_CHN_ATTR_S *pstAttr);

/* Destroy a channel.
 *
 * @param VdChn(In): channel number
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VDEC_DestroyChn(VDEC_CHN VdChn);

/* Get channel attribute
 *
 * @param VdChn(In): channel number
 * @param pstAttr(Out): pointer to VDEC_CHN_ATTR_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VDEC_GetChnAttr(VDEC_CHN VdChn, VDEC_CHN_ATTR_S *pstAttr);

/* Set channel attribute
 *
 * @param VdChn(In): channel number
 * @param pstAttr(In): pointer to VDEC_CHN_ATTR_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VDEC_SetChnAttr(VDEC_CHN VdChn, const VDEC_CHN_ATTR_S *pstAttr);

/* Start Receive Stream
 *
 * @param VdChn(In): channel number
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VDEC_StartRecvStream(VDEC_CHN VdChn);

/* Stop Receive Stream
 *
 * @param VdChn(In): channel number
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VDEC_StopRecvStream(VDEC_CHN VdChn);

/* Query Status
 *
 * @param VdChn(In): channel number
 * @param pstStatus(Out): pointer to VDEC_CHN_STATUS_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VDEC_QueryStatus(VDEC_CHN VdChn, VDEC_CHN_STATUS_S *pstStatus);

/* Reset Channel
 *
 * @param VdChn(In): channel number
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VDEC_ResetChn(VDEC_CHN VdChn);

/* Set Channel Param
 *
 * @param VdChn(In): channel number
 * @param pstParam(In): pointer to VDEC_CHN_PARAM_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VDEC_SetChnParam(VDEC_CHN VdChn, const VDEC_CHN_PARAM_S *pstParam);

/* Get Channel Param
 *
 * @param VdChn(In): channel number
 * @param pstParam(Out): pointer to VDEC_CHN_PARAM_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VDEC_GetChnParam(VDEC_CHN VdChn, VDEC_CHN_PARAM_S *pstParam);

/* Set Protocol Param (Unused now)
 *
 * @param VdChn(In): channel number
 * @param pstParam(In): pointer to VDEC_PRTCL_PARAM_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VDEC_SetProtocolParam(VDEC_CHN VdChn, const VDEC_PRTCL_PARAM_S *pstParam);

/* Get Protocol Param (Unused now)
 *
 * @param VdChn(In): channel number
 * @param pstParam(Out): pointer to VDEC_PRTCL_PARAM_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VDEC_GetProtocolParam(VDEC_CHN VdChn, VDEC_PRTCL_PARAM_S *pstParam);

/* Send Stream
 *
 * @param VdChn(In): channel number
 * @param pstStream(In): pointer to VDEC_STREAM_S
 * @param s32MilliSec(In): -1 is block,0 is no block,other positive number is timeout
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VDEC_SendStream(VDEC_CHN VdChn, const VDEC_STREAM_S *pstStream, CVI_S32 s32MilliSec);

/* Get Frame
 *
 * @param VdChn(In): channel number
 * @param pstFrameInfo(Out): pointer to VIDEO_FRAME_INFO_S
 * @param s32MilliSec(In): timeout
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VDEC_GetFrame(VDEC_CHN VdChn, VIDEO_FRAME_INFO_S *pstFrameInfo, CVI_S32 s32MilliSec);

/* Release Frame
 *
 * @param VdChn(In): channel number
 * @param pstFrameInfo(In): pointer to VIDEO_FRAME_INFO_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VDEC_ReleaseFrame(VDEC_CHN VdChn, const VIDEO_FRAME_INFO_S *pstFrameInfo);

/* Get User Data (Unused now)
 *
 * @param VdChn(In): channel number
 * @param pstUserData(Out): pointer to VDEC_USERDATA_S
 * @param s32MilliSec(In): timeout
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VDEC_GetUserData(VDEC_CHN VdChn, VDEC_USERDATA_S *pstUserData, CVI_S32 s32MilliSec);

/* Release User Data (Unused now)
 *
 * @param VdChn(In): channel number
 * @param pstUserData(In): pointer to VDEC_USERDATA_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VDEC_ReleaseUserData(VDEC_CHN VdChn, const VDEC_USERDATA_S *pstUserData);

/* Set User Pic
 *
 * @param VdChn(In): channel number
 * @param pstUsrPic(In): pointer to VIDEO_FRAME_INFO_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VDEC_SetUserPic(VDEC_CHN VdChn, const VIDEO_FRAME_INFO_S *pstUsrPic);

/* Enable User Pic
 *
 * @param VdChn(In): channel number
 * @param bInstant(In): whether to enable User Pic
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VDEC_EnableUserPic(VDEC_CHN VdChn, CVI_BOOL bInstant);

/* Disable User Pic
 *
 * @param VdChn(In): channel number
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VDEC_DisableUserPic(VDEC_CHN VdChn);

/* Set Display Mode
 *
 * @param VdChn(In): channel number
 * @param enDisplayMode(In): video display mode
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VDEC_SetDisplayMode(VDEC_CHN VdChn, VIDEO_DISPLAY_MODE_E enDisplayMode);

/* Get Display Mode (Unused now)
 *
 * @param VdChn(In): channel number
 * @param penDisplayMode(Out): pointer to VIDEO_DISPLAY_MODE_E
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VDEC_GetDisplayMode(VDEC_CHN VdChn, VIDEO_DISPLAY_MODE_E *penDisplayMode);

/* Set Rotation (Unused now)
 *
 * @param VdChn(In): channel number
 * @param enRotation(In): rotation mode
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VDEC_SetRotation(VDEC_CHN VdChn, ROTATION_E enRotation);

/* Get Rotation (Unused now)
 *
 * @param VdChn(In): channel number
 * @param penRotation(Out): pointer to ROTATION_E
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VDEC_GetRotation(VDEC_CHN VdChn, ROTATION_E *penRotation);

/* Attach VbPool
 *
 * @param VdChn(In): channel number
 * @param pstPool(In): pointer to VDEC_CHN_POOL_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VDEC_AttachVbPool(VDEC_CHN VdChn, const VDEC_CHN_POOL_S *pstPool);

/* Detach VbPool
 *
 * @param VdChn(In): channel number
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VDEC_DetachVbPool(VDEC_CHN VdChn);

/* Set UserData Attribute (Unused now)
 *
 * @param VdChn(In): channel number
 * @param pstUserDataAttr(In): pointer to VDEC_USER_DATA_ATTR_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VDEC_SetUserDataAttr(VDEC_CHN VdChn, const VDEC_USER_DATA_ATTR_S *pstUserDataAttr);

/* Get UserData Attribute (Unused now)
 *
 * @param VdChn(In): channel number
 * @param pstUserDataAttr(Out): pointer to VDEC_USER_DATA_ATTR_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VDEC_GetUserDataAttr(VDEC_CHN VdChn, VDEC_USER_DATA_ATTR_S *pstUserDataAttr);

/* Set Module Param
 *
 * @param pstModParam(In): pointer to VDEC_MOD_PARAM_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VDEC_SetModParam(const VDEC_MOD_PARAM_S *pstModParam);

/* Get Module Param
 *
 * @param pstModParam(Out): pointer to VDEC_MOD_PARAM_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VDEC_GetModParam(VDEC_MOD_PARAM_S *pstModParam);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef  __CVI_VDEC_H__ */


