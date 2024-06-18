/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: raw_replay.h
 * Description:
 *
 */

#ifndef _RAW_REPLAY_H_
#define _RAW_REPLAY_H_

#include "cvi_type.h"
#include "isp_comm_inc.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

typedef struct _RAW_REPLAY_INFO {
	CVI_S32 isValid;
	CVI_S32 numFrame;
	CVI_S32 curFrame;
	CVI_S32 width;
	CVI_S32 height;
	CVI_S32 bayerID;
	CVI_S32 enWDR;
	CVI_S32 ISO;
	CVI_FLOAT lightValue;
	CVI_S32 colorTemp;
	CVI_S32 ispDGain;
	CVI_S32 exposureRatio;
	CVI_S32 exposureAGain;
	CVI_S32 exposureDGain;
	CVI_S32 longExposure;
	CVI_S32 shortExposure;
	CVI_S32 WB_RGain;
	CVI_S32 WB_BGain;
	CVI_S32 CCM[9];
	CVI_S32 BLC_Offset[4];
	CVI_S32 BLC_Gain[4];
	CVI_S32 size;

	CVI_S32 roiFrameNum;
	RECT_S stRoiRect;
	CVI_S32 roiFrameSize;

	CVI_S32 op_mode;
	CVI_S32 AGainSF;
	CVI_S32 DGainSF;
	CVI_S32 ispDGainSF;
} RAW_REPLAY_INFO;

typedef enum _RAWPLAY_OP_MODE {
	RAW_OP_MODE_NORMAL = 0,
	RAW_OP_MODE_AE_SIM = 1,
	RAW_OP_MODE_AWB_SIM = 2,
	RAW_OP_MODE_AF_SIM = 3,
} RAWPLAY_OP_MODE;

typedef enum {
	BB,
	GB,
	GR,
	RR,
	BAYER_CHANNEL_SIZE,
} BAYER_CHANNEL;

#define DISABLE_AWB_UPDATE_CTRL       (1 << 0)

void raw_replay_ctrl(CVI_U32 flag);

CVI_S32 set_raw_replay_data(const CVI_VOID *header, const CVI_VOID *data,
							CVI_U32 totalFrame, CVI_U32 curFrame, CVI_U32 rawFrameSize);

CVI_S32 start_raw_replay(VI_PIPE ViPipe);

CVI_BOOL is_raw_replay_ready(void);

CVI_S32 stop_raw_replay(void);

/*
 * yuvIndex: get which yuv
 * src: 0 get from vi, 1 get from vpss
 * pipeOrGrp: vi pipe or vpss grp
 * chn: vi chn or vpss chn
 * pstFrameInfo: video frame
 */
CVI_S32 get_raw_replay_yuv(CVI_U8 yuvIndex, CVI_U8 src, CVI_S32 pipeOrGrp, CVI_S32 chn,
							VIDEO_FRAME_INFO_S *pstFrameInfo);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _RAW_REPLAY_H_
