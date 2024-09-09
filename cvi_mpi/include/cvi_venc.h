/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: include/cvi_venc.h
 * Description:
 *   Common video encode definitions.
 */

#ifndef __CVI_VENC_H__
#define __CVI_VENC_H__

#include <linux/cvi_comm_video.h>
#include "cvi_comm_vb.h"
#include <linux/cvi_comm_venc.h>
#include <linux/cvi_common.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

/* set log level
 *
 * @param void
 * @return void
 */
CVI_VOID cviGetMask(void);

/* Create a channel.
 *
 * @param VeChn(In): channel number
 * @param pstAttr(In): pointer to VENC_CHN_ATTR_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_CreateChn(VENC_CHN VeChn, const VENC_CHN_ATTR_S *pstAttr);

/* Destroy a channel.
 *
 * @param VeChn(In): channel number
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_DestroyChn(VENC_CHN VeChn);

/* Reset a channel.
 *
 * @param VeChn(In): channel number
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_ResetChn(VENC_CHN VeChn);

/* Start Receive Frame
 *
 * @param VeChn(In): channel number
 * @param pstRecvParam(In): pointer to VENC_RECV_PIC_PARAM_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_StartRecvFrame(VENC_CHN VeChn, const VENC_RECV_PIC_PARAM_S *pstRecvParam);

/* Stop Receive Frame
 *
 * @param VeChn(In): channel number
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_StopRecvFrame(VENC_CHN VeChn);

/* Query Channel Status
 *
 * @param VeChn(In): channel number
 * @param pstStatus(Out): pointer to VENC_CHN_STATUS_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_QueryStatus(VENC_CHN VeChn, VENC_CHN_STATUS_S *pstStatus);

/* Set channel attribute
 *
 * @param VeChn(In): channel number
 * @param pstChnAttr(In): pointer to VENC_CHN_ATTR_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetChnAttr(VENC_CHN VeChn, const VENC_CHN_ATTR_S *pstChnAttr);

/* Get channel attribute
 *
 * @param VeChn(In): channel number
 * @param pstChnAttr(Out): pointer to VENC_CHN_ATTR_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetChnAttr(VENC_CHN VeChn, VENC_CHN_ATTR_S *pstChnAttr);

/* Get stream
 *
 * @param VeChn(In): channel number
 * @param pstStream(Out): pointer to VENC_STREAM_S
 * @param S32MilliSec(In): timeout
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetStream(VENC_CHN VeChn, VENC_STREAM_S *pstStream, CVI_S32 S32MilliSec);

/* Release stream
 *
 * @param VeChn(In): channel number
 * @param pstStream(In): pointer to VENC_STREAM_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_ReleaseStream(VENC_CHN VeChn, VENC_STREAM_S *pstStream);

/* Insert User Data
 *
 * @param VeChn(In): channel number
 * @param pu8Data(In): pointer to User Data
 * @param u32Len(In): User Data Length
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_InsertUserData(VENC_CHN VeChn, CVI_U8 *pu8Data, CVI_U32 u32Len);

/* Send Frame
 *
 * @param VeChn(In): channel number
 * @param pstFrame(In): pointer to VIDEO_FRAME_INFO_S
 * @param s32MilliSec(In): timeout
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SendFrame(VENC_CHN VeChn, const VIDEO_FRAME_INFO_S *pstFrame, CVI_S32 s32MilliSec);

/* Send Customized Frame
 *
 * @param VeChn(In): channel number
 * @param pstFrame(In): pointer to USER_FRAME_INFO_S
 * @param s32MilliSec(In): timeout
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SendFrameEx(VENC_CHN VeChn, const USER_FRAME_INFO_S *pstFrame, CVI_S32 s32MilliSec);

/* Request IDR
 *
 * @param VeChn(In): channel number
 * @param bInstant(In): Unused
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_RequestIDR(VENC_CHN VeChn, CVI_BOOL bInstant);

/* whether to enable SVC
 *
 * @param VeChn(In): channel number
 * @param enable(In): 1 or 0
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_EnableSvc(VENC_CHN VeChn, CVI_BOOL enable);

/* Get File Descriptor
 *
 * @param VeChn(In): channel number
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetFd(VENC_CHN VeChn);

/* Close File Descriptor
 *
 * @param VeChn(In): channel number
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_CloseFd(VENC_CHN VeChn);

/* Set ROI attribute
 *
 * @param VeChn(In): channel number
 * @param pstRoiAttr(In): pointer to VENC_ROI_ATTR_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetRoiAttr(VENC_CHN VeChn, const VENC_ROI_ATTR_S *pstRoiAttr);

/* Get ROI attribute
 *
 * @param VeChn(In): channel number
 * @param u32Index(In): ROI index
 * @param pstRoiAttr(Out): pointer to VENC_ROI_ATTR_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetRoiAttr(VENC_CHN VeChn, CVI_U32 u32Index, VENC_ROI_ATTR_S *pstRoiAttr);

/* Get ROI attribute (Unused now)
 *
 * @param VeChn(In): channel number
 * @param u32Index(In): ROI index
 * @param pstRoiAttrEx(Out): pointer to VENC_ROI_ATTR_EX_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetRoiAttrEx(VENC_CHN VeChn, CVI_U32 u32Index, VENC_ROI_ATTR_EX_S *pstRoiAttrEx);

/* Set ROI attribute (Unused now)
 *
 * @param VeChn(In): channel number
 * @param pstRoiAttrEx(In): pointer to VENC_ROI_ATTR_EX_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetRoiAttrEx(VENC_CHN VeChn, const VENC_ROI_ATTR_EX_S *pstRoiAttrEx);

/* Set ROI Background FrameRate (Unused now)
 *
 * @param VeChn(In): channel number
 * @param pstRoiBgFrmRate(In): pointer to VENC_ROIBG_FRAME_RATE_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetRoiBgFrameRate(VENC_CHN VeChn, const VENC_ROIBG_FRAME_RATE_S *pstRoiBgFrmRate);

/* Get ROI Background FrameRate (Unused now)
 *
 * @param VeChn(In): channel number
 * @param pstRoiBgFrmRate(Out): pointer to VENC_ROIBG_FRAME_RATE_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetRoiBgFrameRate(VENC_CHN VeChn, VENC_ROIBG_FRAME_RATE_S *pstRoiBgFrmRate);

/* Set H264 Slice Split
 *
 * @param VeChn(In): channel number
 * @param pstSliceSplit(In): pointer to VENC_H264_SLICE_SPLIT_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetH264SliceSplit(VENC_CHN VeChn, const VENC_H264_SLICE_SPLIT_S *pstSliceSplit);

/* Get H264 Slice Split
 *
 * @param VeChn(In): channel number
 * @param pstSliceSplit(Out): pointer to VENC_H264_SLICE_SPLIT_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetH264SliceSplit(VENC_CHN VeChn, VENC_H264_SLICE_SPLIT_S *pstSliceSplit);

/* Set H264 Intra Prediction
 *
 * @param VeChn(In): channel number
 * @param pstH264IntraPred(In): pointer to VENC_H264_INTRA_PRED_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetH264IntraPred(VENC_CHN VeChn, const VENC_H264_INTRA_PRED_S *pstH264IntraPred);

/* Get H264 Intra Prediction
 *
 * @param VeChn(In): channel number
 * @param pstH264IntraPred(Out): pointer to VENC_H264_INTRA_PRED_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetH264IntraPred(VENC_CHN VeChn, VENC_H264_INTRA_PRED_S *pstH264IntraPred);

/* Set H264 Transform
 *
 * @param VeChn(In): channel number
 * @param pstH264Trans(In): pointer to VENC_H264_TRANS_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetH264Trans(VENC_CHN VeChn, const VENC_H264_TRANS_S *pstH264Trans);

/* Get H264 Transform
 *
 * @param VeChn(In): channel number
 * @param pstH264Trans(Out): pointer to VENC_H264_TRANS_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetH264Trans(VENC_CHN VeChn, VENC_H264_TRANS_S *pstH264Trans);

/* Set H264 Entropy
 *
 * @param VeChn(In): channel number
 * @param pstH264EntropyEnc(In): pointer to VENC_H264_ENTROPY_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetH264Entropy(VENC_CHN VeChn, const VENC_H264_ENTROPY_S *pstH264EntropyEnc);

/* Get H264 Entropy
 *
 * @param VeChn(In): channel number
 * @param pstH264EntropyEnc(Out): pointer to VENC_H264_ENTROPY_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetH264Entropy(VENC_CHN VeChn, VENC_H264_ENTROPY_S *pstH264EntropyEnc);

/* Set H264 Deblock
 *
 * @param VeChn(In): channel number
 * @param pstH264Dblk(In): pointer to VENC_H264_DBLK_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetH264Dblk(VENC_CHN VeChn, const VENC_H264_DBLK_S *pstH264Dblk);

/* Get H264 Deblock
 *
 * @param VeChn(In): channel number
 * @param pstH264Dblk(Out): pointer to VENC_H264_DBLK_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetH264Dblk(VENC_CHN VeChn, VENC_H264_DBLK_S *pstH264Dblk);

/* Set H264 Vui Info
 *
 * @param VeChn(In): channel number
 * @param pstH264Vui(In): pointer to VENC_H264_VUI_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetH264Vui(VENC_CHN VeChn, const VENC_H264_VUI_S *pstH264Vui);

/* Get H264 Vui Info
 *
 * @param VeChn(In): channel number
 * @param pstH264Vui(Out): pointer to VENC_H264_VUI_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetH264Vui(VENC_CHN VeChn, VENC_H264_VUI_S *pstH264Vui);

/* Set H265 Vui Info
 *
 * @param VeChn(In): channel number
 * @param pstH265Vui(In): pointer to VENC_H265_VUI_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetH265Vui(VENC_CHN VeChn, const VENC_H265_VUI_S *pstH265Vui);

/* Get H265 Vui Info
 *
 * @param VeChn(In): channel number
 * @param pstH265Vui(Out): pointer to VENC_H265_VUI_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetH265Vui(VENC_CHN VeChn, VENC_H265_VUI_S *pstH265Vui);

/* Set Jpeg Param
 *
 * @param VeChn(In): channel number
 * @param pstJpegParam(In): pointer to VENC_JPEG_PARAM_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetJpegParam(VENC_CHN VeChn, const VENC_JPEG_PARAM_S *pstJpegParam);

/* Get Jpeg Param
 *
 * @param VeChn(In): channel number
 * @param pstJpegParam(Out): pointer to VENC_JPEG_PARAM_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetJpegParam(VENC_CHN VeChn, VENC_JPEG_PARAM_S *pstJpegParam);

/* Set Mjpeg Param
 *
 * @param VeChn(In): channel number
 * @param pstMjpegParam(In): pointer to VENC_MJPEG_PARAM_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetMjpegParam(VENC_CHN VeChn, const VENC_MJPEG_PARAM_S *pstMjpegParam);

/* Get Mjpeg Param
 *
 * @param VeChn(In): channel number
 * @param pstMjpegParam(Out): pointer to VENC_MJPEG_PARAM_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetMjpegParam(VENC_CHN VeChn, VENC_MJPEG_PARAM_S *pstMjpegParam);

/* Get RC Param
 *
 * @param VeChn(In): channel number
 * @param pstRcParam(Out): pointer to VENC_RC_PARAM_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetRcParam(VENC_CHN VeChn, VENC_RC_PARAM_S *pstRcParam);

/* Set RC Param
 *
 * @param VeChn(In): channel number
 * @param pstRcParam(In): pointer to VENC_RC_PARAM_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetRcParam(VENC_CHN VeChn, const VENC_RC_PARAM_S *pstRcParam);

/* Set Ref Param
 *
 * @param VeChn(In): channel number
 * @param pstRefParam(In): pointer to VENC_REF_PARAM_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetRefParam(VENC_CHN VeChn, const VENC_REF_PARAM_S *pstRefParam);

/* Get Ref Param
 *
 * @param VeChn(In): channel number
 * @param pstRefParam(Out): pointer to VENC_REF_PARAM_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetRefParam(VENC_CHN VeChn, VENC_REF_PARAM_S *pstRefParam);

/* Set Jpeg Encode Mode (Unused now)
 *
 * @param VeChn(In): channel number
 * @param enJpegEncodeMode(In): Jpeg Snap Mode
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetJpegEncodeMode(VENC_CHN VeChn, const VENC_JPEG_ENCODE_MODE_E enJpegEncodeMode);

/* Get Jpeg Encode Mode (Unused now)
 *
 * @param VeChn(In): channel number
 * @param penJpegEncodeMode(Out): pointer to VENC_JPEG_ENCODE_MODE_E
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetJpegEncodeMode(VENC_CHN VeChn, VENC_JPEG_ENCODE_MODE_E *penJpegEncodeMode);

/* VENC Enable IDR
 *
 * @param VeChn(In): channel number
 * @param bEnableIDR(In): whether to Enable IDR
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_EnableIDR(VENC_CHN VeChn, CVI_BOOL bEnableIDR);

/* Get StreamBuf Info (Unused now)
 *
 * @param VeChn(In): channel number
 * @param pstStreamBufInfo(Out): pointer to VENC_STREAM_BUF_INFO_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetStreamBufInfo(VENC_CHN VeChn, VENC_STREAM_BUF_INFO_S *pstStreamBufInfo);

/* Set H265 Slice Split
 *
 * @param VeChn(In): channel number
 * @param pstSliceSplit(In): pointer to VENC_H265_SLICE_SPLIT_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetH265SliceSplit(VENC_CHN VeChn, const VENC_H265_SLICE_SPLIT_S *pstSliceSplit);

/* Get H265 Slice Split
 *
 * @param VeChn(In): channel number
 * @param pstSliceSplit(Out): pointer to VENC_H265_SLICE_SPLIT_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetH265SliceSplit(VENC_CHN VeChn, VENC_H265_SLICE_SPLIT_S *pstSliceSplit);

/* Set H265 Prediction Unit
 *
 * @param VeChn(In): channel number
 * @param pstPredUnit(In): pointer to VENC_H265_PU_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetH265PredUnit(VENC_CHN VeChn, const VENC_H265_PU_S *pstPredUnit);

/* Get H265 Prediction Unit
 *
 * @param VeChn(In): channel number
 * @param pstPredUnit(Out): pointer to VENC_H265_PU_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetH265PredUnit(VENC_CHN VeChn, VENC_H265_PU_S *pstPredUnit);

/* Set H265 Transform
 *
 * @param VeChn(In): channel number
 * @param pstH265Trans(In): pointer to VENC_H265_TRANS_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetH265Trans(VENC_CHN VeChn, const VENC_H265_TRANS_S *pstH265Trans);

/* Get H265 Transform
 *
 * @param VeChn(In): channel number
 * @param pstH265Trans(Out): pointer to VENC_H265_TRANS_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetH265Trans(VENC_CHN VeChn, VENC_H265_TRANS_S *pstH265Trans);

/* Set H265 Entropy (Unused now)
 *
 * @param VeChn(In): channel number
 * @param pstH265Entropy(In): pointer to VENC_H265_ENTROPY_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetH265Entropy(VENC_CHN VeChn, const VENC_H265_ENTROPY_S *pstH265Entropy);

/* Get H265 Entropy (Unused now)
 *
 * @param VeChn(In): channel number
 * @param pstH265Entropy(Out): pointer to VENC_H265_ENTROPY_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetH265Entropy(VENC_CHN VeChn, VENC_H265_ENTROPY_S *pstH265Entropy);

/* Set H265 Deblock
 *
 * @param VeChn(In): channel number
 * @param pstH265Dblk(In): pointer to VENC_H265_DBLK_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetH265Dblk(VENC_CHN VeChn, const VENC_H265_DBLK_S *pstH265Dblk);

/* Get H265 Deblock
 *
 * @param VeChn(In): channel number
 * @param pstH265Dblk(Out): pointer to VENC_H265_DBLK_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetH265Dblk(VENC_CHN VeChn, VENC_H265_DBLK_S *pstH265Dblk);

/* Set H265 Sao
 *
 * @param VeChn(In): channel number
 * @param pstH265Sao(In): pointer to VENC_H265_SAO_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetH265Sao(VENC_CHN VeChn, const VENC_H265_SAO_S *pstH265Sao);

/* Get H265 Sao
 *
 * @param VeChn(In): channel number
 * @param pstH265Sao(Out): pointer to VENC_H265_SAO_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetH265Sao(VENC_CHN VeChn, VENC_H265_SAO_S *pstH265Sao);

/* Set FrameLost Strategy
 *
 * @param VeChn(In): channel number
 * @param pstFrmLostParam(In): pointer to VENC_FRAMELOST_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetFrameLostStrategy(VENC_CHN VeChn, const VENC_FRAMELOST_S *pstFrmLostParam);

/* Get FrameLost Strategy
 *
 * @param VeChn(In): channel number
 * @param pstFrmLostParam(Out): pointer to VENC_FRAMELOST_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetFrameLostStrategy(VENC_CHN VeChn, VENC_FRAMELOST_S *pstFrmLostParam);

/* Set SuperFrame Strategy
 *
 * @param VeChn(In): channel number
 * @param pstSuperFrmParam(In): pointer to VENC_SUPERFRAME_CFG_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetSuperFrameStrategy(VENC_CHN VeChn, const VENC_SUPERFRAME_CFG_S *pstSuperFrmParam);

/* Get SuperFrame Strategy
 *
 * @param VeChn(In): channel number
 * @param pstSuperFrmParam(Out): pointer to VENC_SUPERFRAME_CFG_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetSuperFrameStrategy(VENC_CHN VeChn, VENC_SUPERFRAME_CFG_S *pstSuperFrmParam);

/* Set Intra Refresh (Unused now)
 *
 * @param VeChn(In): channel number
 * @param pstIntraRefresh(In): pointer to VENC_INTRA_REFRESH_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetIntraRefresh(VENC_CHN VeChn, const VENC_INTRA_REFRESH_S *pstIntraRefresh);

/* Get Intra Refresh (Unused now)
 *
 * @param VeChn(In): channel number
 * @param pstIntraRefresh(Out): pointer to VENC_INTRA_REFRESH_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetIntraRefresh(VENC_CHN VeChn, VENC_INTRA_REFRESH_S *pstIntraRefresh);

/* Get SSE Region (Unused now)
 *
 * @param VeChn(In): channel number
 * @param u32Index(In): SSE Region Index
 * @param pstSSECfg(Out): pointer to VENC_SSE_CFG_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetSSERegion(VENC_CHN VeChn, CVI_U32 u32Index, VENC_SSE_CFG_S *pstSSECfg);

/* Set SSE Region (Unused now)
 *
 * @param VeChn(In): channel number
 * @param pstSSECfg(In): pointer to VENC_SSE_CFG_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetSSERegion(VENC_CHN VeChn, const VENC_SSE_CFG_S *pstSSECfg);

/* Set Channel Param
 *
 * @param VeChn(In): channel number
 * @param pstChnParam(In): pointer to VENC_CHN_PARAM_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetChnParam(VENC_CHN VeChn, const VENC_CHN_PARAM_S *pstChnParam);

/* Get Channel Param
 *
 * @param VeChn(In): channel number
 * @param pstChnParam(Out): pointer to VENC_CHN_PARAM_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetChnParam(VENC_CHN VeChn, VENC_CHN_PARAM_S *pstChnParam);

/* Set Module Param
 *
 * @param VeChn(In): channel number
 * @param pstModParam(In): pointer to VENC_PARAM_MOD_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetModParam(const VENC_PARAM_MOD_S *pstModParam);

/* Get Module Param
 *
 * @param pstModParam(Out): pointer to VENC_PARAM_MOD_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetModParam(VENC_PARAM_MOD_S *pstModParam);

/* Get Foreground Protect (Unused now)
 *
 * @param VeChn(In): channel number
 * @param pstForegroundProtect(Out): pointer to VENC_FOREGROUND_PROTECT_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetForegroundProtect(VENC_CHN VeChn, VENC_FOREGROUND_PROTECT_S *pstForegroundProtect);

/* Set Foreground Protect (Unused now)
 *
 * @param VeChn(In): channel number
 * @param pstForegroundProtect(In): pointer to VENC_FOREGROUND_PROTECT_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetForegroundProtect(VENC_CHN VeChn, const VENC_FOREGROUND_PROTECT_S *pstForegroundProtect);

/* Set Scene Mode (Unused now)
 *
 * @param VeChn(In): channel number
 * @param enSceneMode(In): encode scene mode
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetSceneMode(VENC_CHN VeChn, const VENC_SCENE_MODE_E enSceneMode);

/* Get Scene Mode (Unused now)
 *
 * @param VeChn(In): channel number
 * @param penSceneMode(Out): pointer to VENC_SCENE_MODE_E
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetSceneMode(VENC_CHN VeChn, VENC_SCENE_MODE_E *penSceneMode);

/* Attach VbPool
 *
 * @param VeChn(In): channel number
 * @param pstPool(In): pointer to VENC_CHN_POOL_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_AttachVbPool(VENC_CHN VeChn, const VENC_CHN_POOL_S *pstPool);

/* Detach VbPool
 *
 * @param VeChn(In): channel number
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_DetachVbPool(VENC_CHN VeChn);

/* Set Cu Prediction
 *
 * @param VeChn(In): channel number
 * @param pstCuPrediction(In): pointer to VENC_CU_PREDICTION_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetCuPrediction(VENC_CHN VeChn,
		const VENC_CU_PREDICTION_S *pstCuPrediction);

/* Get Cu Prediction
 *
 * @param VeChn(In): channel number
 * @param pstCuPrediction(Out): pointer to VENC_CU_PREDICTION_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetCuPrediction(VENC_CHN VeChn,
		VENC_CU_PREDICTION_S *pstCuPrediction);

/* Set Skip Bias (Unused now)
 *
 * @param VeChn(In): channel number
 * @param pstSkipBias(In): pointer to VENC_SKIP_BIAS_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetSkipBias(VENC_CHN VeChn, const VENC_SKIP_BIAS_S *pstSkipBias);

/* Get Skip Bias (Unused now)
 *
 * @param VeChn(In): channel number
 * @param pstSkipBias(Out): pointer to VENC_SKIP_BIAS_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetSkipBias(VENC_CHN VeChn, VENC_SKIP_BIAS_S *pstSkipBias);

/* Set DeBreath Effect (Unused now)
 *
 * @param VeChn(In): channel number
 * @param pstDeBreathEffect(In): pointer to VENC_DEBREATHEFFECT_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetDeBreathEffect(VENC_CHN VeChn, const VENC_DEBREATHEFFECT_S *pstDeBreathEffect);

/* Get DeBreath Effect (Unused now)
 *
 * @param VeChn(In): channel number
 * @param pstDeBreathEffect(Out): pointer to VENC_DEBREATHEFFECT_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetDeBreathEffect(VENC_CHN VeChn, VENC_DEBREATHEFFECT_S *pstDeBreathEffect);

/* Set Hierarchical Qp (Unused now)
 *
 * @param VeChn(In): channel number
 * @param pstHierarchicalQp(In): pointer to VENC_HIERARCHICAL_QP_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetHierarchicalQp(VENC_CHN VeChn, const VENC_HIERARCHICAL_QP_S *pstHierarchicalQp);

/* Get Hierarchical Qp (Unused now)
 *
 * @param VeChn(In): channel number
 * @param pstHierarchicalQp(Out): pointer to VENC_HIERARCHICAL_QP_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetHierarchicalQp(VENC_CHN VeChn, VENC_HIERARCHICAL_QP_S *pstHierarchicalQp);

/* Set Rc Advanced Param (Unused now)
 *
 * @param VeChn(In): channel number
 * @param pstRcAdvParam(In): pointer to VENC_RC_ADVPARAM_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetRcAdvParam(VENC_CHN VeChn, const VENC_RC_ADVPARAM_S *pstRcAdvParam);

/* Get Rc Advanced Param (Unused now)
 *
 * @param VeChn(In): channel number
 * @param pstRcAdvParam(Out): pointer to VENC_RC_ADVPARAM_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetRcAdvParam(VENC_CHN VeChn, VENC_RC_ADVPARAM_S *pstRcAdvParam);

/* Calculate Frame Param
 *
 * @param VeChn(In): channel number
 * @param pstFrameParam(Out): pointer to VENC_FRAME_PARAM_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_CalcFrameParam(VENC_CHN VeChn, VENC_FRAME_PARAM_S *pstFrameParam);

/* Set Frame Param
 *
 * @param VeChn(In): channel number
 * @param pstFrameParam(In): pointer to VENC_FRAME_PARAM_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetFrameParam(VENC_CHN VeChn, const VENC_FRAME_PARAM_S *pstFrameParam);

/* Get Frame Param
 *
 * @param VeChn(In): channel number
 * @param pstFrameParam(Out): pointer to VENC_FRAME_PARAM_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetFrameParam(VENC_CHN VeChn, VENC_FRAME_PARAM_S *pstFrameParam);

/* Set SVC Param
 *
 * @param VeChn(In): channel number
 * @param pstFrameParam(Out): pointer to VENC_SVC_PARAM_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_SetSvcParam(VENC_CHN VeChn, const VENC_SVC_PARAM_S *pstSvcParam);

/* Get SVC Param
 *
 * @param VeChn(In): channel number
 * @param pstFrameParam(Out): pointer to VENC_SVC_PARAM_S
 * @return Error code (0 if successful)
 */
CVI_S32 CVI_VENC_GetSvcParam(VENC_CHN VeChn, VENC_SVC_PARAM_S *pstSvcParam);

#define CVI_H264_PROFILE_DEFAULT	H264E_PROFILE_HIGH
#define CVI_H264_PROFILE_MIN		0
#define CVI_H264_PROFILE_MAX		(H264E_PROFILE_BUTT - 1)

#define CVI_H264_ENTROPY_DEFAULT	1
#define CVI_H264_ENTROPY_MIN		0
#define CVI_H264_ENTROPY_MAX		1

#define CVI_INITIAL_DELAY_DEFAULT	1000
#define CVI_INITIAL_DELAY_MIN		10
#define CVI_INITIAL_DELAY_MAX		3000

#define CVI_VARI_FPS_EN_DEFAULT	0
#define CVI_VARI_FPS_EN_MIN		0
#define CVI_VARI_FPS_EN_MAX		1

#define CVI_H26X_GOP_DEFAULT	60
#define CVI_H26X_GOP_MIN		1
#define CVI_H26X_GOP_MAX		3600

#define CVI_H26X_GOP_MODE_DEFAULT	VENC_GOPMODE_NORMALP
#define CVI_H26X_GOP_MODE_MIN		VENC_GOPMODE_NORMALP
#define CVI_H26X_GOP_MODE_MAX		(VENC_GOPMODE_BUTT - 1)

#define CVI_H26X_NORMALP_IP_QP_DELTA_DEFAULT	2
#define CVI_H26X_NORMALP_IP_QP_DELTA_MIN	-10
#define CVI_H26X_NORMALP_IP_QP_DELTA_MAX	30

#define CVI_H26X_SMARTP_BG_INTERVAL_DEFAULT	(CVI_H26X_GOP_DEFAULT * 2)
#define CVI_H26X_SMARTP_BG_INTERVAL_MIN		CVI_H26X_GOP_MIN
#define CVI_H26X_SMARTP_BG_INTERVAL_MAX		65536

#define CVI_H26X_SMARTP_BG_QP_DELTA_DEFAULT	2
#define CVI_H26X_SMARTP_BG_QP_DELTA_MIN		-10
#define CVI_H26X_SMARTP_BG_QP_DELTA_MAX		30

#define CVI_H26X_SMARTP_VI_QP_DELTA_DEFAULT	0
#define CVI_H26X_SMARTP_VI_QP_DELTA_MIN		-10
#define CVI_H26X_SMARTP_VI_QP_DELTA_MAX		30

#define CVI_H26X_MAXIQP_DEFAULT	51
#define CVI_H26X_MAXIQP_MIN		1
#define CVI_H26X_MAXIQP_MAX		51

#define CVI_H26X_MINIQP_DEFAULT	1
#define CVI_H26X_MINIQP_MIN		1
#define CVI_H26X_MINIQP_MAX		51

#define CVI_H26X_MAXQP_DEFAULT	51
#define CVI_H26X_MAXQP_MIN		0
#define CVI_H26X_MAXQP_MAX		51

#define CVI_H26X_MINQP_DEFAULT	1
#define CVI_H26X_MINQP_MIN		0
#define CVI_H26X_MINQP_MAX		51

#define CVI_H26X_MAX_I_PROP_DEFAULT	100
#define CVI_H26X_MAX_I_PROP_MIN		1
#define CVI_H26X_MAX_I_PROP_MAX		100

#define CVI_H26X_MIN_I_PROP_DEFAULT	1
#define CVI_H26X_MIN_I_PROP_MIN		1
#define CVI_H26X_MIN_I_PROP_MAX		100

#define CVI_H26X_MAXBITRATE_DEFAULT	5000
#define CVI_H26X_MAXBITRATE_MIN		100
#define CVI_H26X_MAXBITRATE_MAX		300000

#define CVI_H26X_CHANGE_POS_DEFAULT	90
#define CVI_H26X_CHANGE_POS_MIN		50
#define CVI_H26X_CHANGE_POS_MAX		100

#define CVI_H26X_MIN_STILL_PERCENT_DEFAULT	10
#define CVI_H26X_MIN_STILL_PERCENT_MIN		1
#define CVI_H26X_MIN_STILL_PERCENT_MAX		100

#define CVI_H26X_MAX_STILL_QP_DEFAULT	1
#define CVI_H26X_MAX_STILL_QP_MIN		0
#define CVI_H26X_MAX_STILL_QP_MAX		51

#define CVI_H26X_MOTION_SENSITIVITY_DEFAULT	100
#define CVI_H26X_MOTION_SENSITIVITY_MIN		1
#define CVI_H26X_MOTION_SENSITIVITY_MAX		1024

#define CVI_H26X_AVBR_FRM_LOST_OPEN_DEFAULT	1
#define CVI_H26X_AVBR_FRM_LOST_OPEN_MIN		0
#define CVI_H26X_AVBR_FRM_LOST_OPEN_MAX		1

#define CVI_H26X_AVBR_FRM_GAP_DEFAULT	1
#define CVI_H26X_AVBR_FRM_GAP_MIN		0
#define CVI_H26X_AVBR_FRM_GAP_MAX		100

#define CVI_H26X_AVBR_PURE_STILL_THR_DEFAULT	4
#define CVI_H26X_AVBR_PURE_STILL_THR_MIN		0
#define CVI_H26X_AVBR_PURE_STILL_THR_MAX		500

#define CVI_H26X_INTRACOST_DEFAULT	0
#define CVI_H26X_INTRACOST_MIN		0
#define CVI_H26X_INTRACOST_MAX		16383

#define CVI_H26X_THRDLV_DEFAULT	2
#define CVI_H26X_THRDLV_MIN		0
#define CVI_H26X_THRDLV_MAX		4

#define CVI_H26X_BG_ENHANCE_EN_DEFAULT	0
#define CVI_H26X_BG_ENHANCE_EN_MIN		0
#define CVI_H26X_BG_ENHANCE_EN_MAX		1

#define CVI_H26X_BG_DELTA_QP_DEFAULT	0
#define CVI_H26X_BG_DELTA_QP_MIN		-8
#define CVI_H26X_BG_DELTA_QP_MAX		8

#define CVI_H26X_ROW_QP_DELTA_DEFAULT	1
#define CVI_H26X_ROW_QP_DELTA_MIN		0
#define CVI_H26X_ROW_QP_DELTA_MAX		10

#define CVI_H26X_SUPER_FRM_MODE_DEFAULT	0
#define CVI_H26X_SUPER_FRM_MODE_MIN		0
#define CVI_H26X_SUPER_FRM_MODE_MAX		3

#define CVI_H26X_SUPER_I_BITS_THR_DEFAULT	(4 * 8 * 1024 * 1024)
#define CVI_H26X_SUPER_I_BITS_THR_MIN		1000
#define CVI_H26X_SUPER_I_BITS_THR_MAX		(10 * 8 * 1024 * 1024)

#define CVI_H26X_SUPER_P_BITS_THR_DEFAULT	(4 * 8 * 1024 * 1024)
#define CVI_H26X_SUPER_P_BITS_THR_MIN		1000
#define CVI_H26X_SUPER_P_BITS_THR_MAX		(10 * 8 * 1024 * 1024)

#define CVI_H26X_MAX_RE_ENCODE_DEFAULT	0
#define CVI_H26X_MAX_RE_ENCODE_MIN		0
#define CVI_H26X_MAX_RE_ENCODE_MAX		4

#define CVI_H26X_ASPECT_RATIO_INFO_PRESENT_FLAG_DEFAULT	0
#define CVI_H26X_ASPECT_RATIO_INFO_PRESENT_FLAG_MIN	0
#define CVI_H26X_ASPECT_RATIO_INFO_PRESENT_FLAG_MAX	1

#define CVI_H26X_ASPECT_RATIO_IDC_DEFAULT		1
#define CVI_H26X_ASPECT_RATIO_IDC_MIN			0
#define CVI_H26X_ASPECT_RATIO_IDC_MAX			255

#define CVI_H26X_OVERSCAN_INFO_PRESENT_FLAG_DEFAULT	0
#define CVI_H26X_OVERSCAN_INFO_PRESENT_FLAG_MIN		0
#define CVI_H26X_OVERSCAN_INFO_PRESENT_FLAG_MAX		1

#define CVI_H26X_OVERSCAN_APPROPRIATE_FLAG_DEFAULT	0
#define CVI_H26X_OVERSCAN_APPROPRIATE_FLAG_MIN		0
#define CVI_H26X_OVERSCAN_APPROPRIATE_FLAG_MAX		1

#define CVI_H26X_SAR_WIDTH_DEFAULT			1
#define CVI_H26X_SAR_WIDTH_MIN				1
#define CVI_H26X_SAR_WIDTH_MAX				65535

#define CVI_H26X_SAR_HEIGHT_DEFAULT			1
#define CVI_H26X_SAR_HEIGHT_MIN				1
#define CVI_H26X_SAR_HEIGHT_MAX				65535

#define CVI_H26X_TIMING_INFO_PRESENT_FLAG_DEFAULT	0
#define CVI_H26X_TIMING_INFO_PRESENT_FLAG_MIN		0
#define CVI_H26X_TIMING_INFO_PRESENT_FLAG_MAX		1

#define CVI_H264_FIXED_FRAME_RATE_FLAG_DEFAULT		0
#define CVI_H264_FIXED_FRAME_RATE_FLAG_MIN		0
#define CVI_H264_FIXED_FRAME_RATE_FLAG_MAX		1

#define CVI_H26X_NUM_UNITS_IN_TICK_DEFAULT		1
#define CVI_H26X_NUM_UNITS_IN_TICK_MIN			1
#define CVI_H26X_NUM_UNITS_IN_TICK_MAX			4294967295

#define CVI_H26X_TIME_SCALE_DEFAULT			60
#define CVI_H26X_TIME_SCALE_MIN				1
#define CVI_H26X_TIME_SCALE_MAX				4294967295

#define CVI_H265_NUM_TICKS_POC_DIFF_ONE_MINUS1_DEFAULT	1
#define CVI_H265_NUM_TICKS_POC_DIFF_ONE_MINUS1_MIN	0
#define CVI_H265_NUM_TICKS_POC_DIFF_ONE_MINUS1_MAX	4294967294

#define CVI_H26X_VIDEO_SIGNAL_TYPE_PRESENT_FLAG_DEFAULT	0
#define CVI_H26X_VIDEO_SIGNAL_TYPE_PRESENT_FLAG_MIN	0
#define CVI_H26X_VIDEO_SIGNAL_TYPE_PRESENT_FLAG_MAX	1

#define CVI_H26X_VIDEO_FORMAT_DEFAULT			5
#define CVI_H26X_VIDEO_FORMAT_MIN			0
#define CVI_H264_VIDEO_FORMAT_MAX			7
#define CVI_H265_VIDEO_FORMAT_MAX			5

#define CVI_H26X_VIDEO_FULL_RANGE_FLAG_DEFAULT		0
#define CVI_H26X_VIDEO_FULL_RANGE_FLAG_MIN		0
#define CVI_H26X_VIDEO_FULL_RANGE_FLAG_MAX		1

#define CVI_H26X_COLOUR_DESCRIPTION_PRESENT_FLAG_DEFAULT	0
#define CVI_H26X_COLOUR_DESCRIPTION_PRESENT_FLAG_MIN		0
#define CVI_H26X_COLOUR_DESCRIPTION_PRESENT_FLAG_MAX		1

#define CVI_H26X_COLOUR_PRIMARIES_DEFAULT		2
#define CVI_H26X_COLOUR_PRIMARIES_MIN			0
#define CVI_H26X_COLOUR_PRIMARIES_MAX			255

#define CVI_H26X_TRANSFER_CHARACTERISTICS_DEFAULT	2
#define CVI_H26X_TRANSFER_CHARACTERISTICS_MIN		0
#define CVI_H26X_TRANSFER_CHARACTERISTICS_MAX		255

#define CVI_H26X_MATRIX_COEFFICIENTS_DEFAULT		2
#define CVI_H26X_MATRIX_COEFFICIENTS_MIN		0
#define CVI_H26X_MATRIX_COEFFICIENTS_MAX		255

#define CVI_H26X_BITSTREAM_RESTRICTION_FLAG_DEFAULT	0
#define CVI_H26X_BITSTREAM_RESTRICTION_FLAG_MIN		0
#define CVI_H26X_BITSTREAM_RESTRICTION_FLAG_MAX		1

#define CVI_H26X_TEST_UBR_EN_DEFAULT	0
#define CVI_H26X_TEST_UBR_EN_MIN		0
#define CVI_H26X_TEST_UBR_EN_MAX		1

#define CVI_H26X_FRAME_QP_DEFAULT	38
#define CVI_H26X_FRAME_QP_MIN		0
#define CVI_H26X_FRAME_QP_MAX		51

#define CVI_H26X_FRAME_BITS_DEFAULT	(200 * 1000)
#define CVI_H26X_FRAME_BITS_MIN		1000
#define CVI_H26X_FRAME_BITS_MAX		10000000

#define CVI_H26X_ES_BUFFER_QUEUE_DEFAULT	1
#define CVI_H26X_ES_BUFFER_QUEUE_MIN		0
#define CVI_H26X_ES_BUFFER_QUEUE_MAX		1

#define CVI_H26X_ISO_SEND_FRAME_DEFAUL		1
#define CVI_H26X_ISO_SEND_FRAME_MIN			0
#define CVI_H26X_ISO_SEND_FRAME_MAX			1

#define CVI_H26X_SENSOR_EN_DEFAULT		0
#define CVI_H26X_SENSOR_EN_MIN			0
#define CVI_H26X_SENSOR_EN_MAX			1

#define DEF_STAT_TIME		-1
#define DEF_GOP				30
#define DEF_IQP				32
#define DEF_PQP				32

#define DEF_VARI_FPS_EN				0
#define DEF_264_GOP					60
#define DEF_264_MAXIQP				51
#define DEF_264_MINIQP				1
#define DEF_264_MAXQP				51
#define DEF_264_MINQP				1
#define DEF_264_MAXBITRATE			5000
#define DEF_26X_CHANGE_POS			90

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __CVI_VENC_H__ */
