/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_sts_ctrl.h
 * Description:
 *
 */

#ifndef _ISP_STS_CTRL_H_
#define _ISP_STS_CTRL_H_

#include "cvi_comm_isp.h"
#include "isp_3a.h"
#include "isp_sts_ref.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define ISP_MAX_STS_BUF_NUM (2)

struct isp_sts_ctrl_runtime {

	// statistic buffer
	struct cvi_isp_sts_mem sts_mem[ISP_MAX_STS_BUF_NUM];

	// statistic configuration
	struct cvi_vip_isp_ae_config ae_cfg;

	struct cvi_vip_isp_af_config af_cfg;
	struct cvi_vip_isp_gms_config gms_cfg;

	// Preraw
	CVI_U32 pre_frame_cnt;
	CVI_U32 pre_sts_idx;

	CVI_U8 out_of_ae_sts_div_bit;
	ISP_AE_STATISTICS_COMPAT_S ae_sts;

	ISP_WB_STATISTICS_S awb_sts[ISP_CHANNEL_MAX_NUM];

	ISP_AF_STATISTICS_S af_sts;
	ISP_DIS_STATS_INFO gms_sts;

	ISP_MOTION_STATS_INFO motion_sts;

	// Postraw
	CVI_U32 post_frame_cnt;
	CVI_U32 post_sts_idx;

	// ISP_HIST_EDGE_STATISTIC_S hist_edge_v_sts;
	ISP_DCI_STATISTICS_S dci_sts;

	CVI_BOOL initialized;
	CVI_BOOL mapped;
};

CVI_S32 isp_sts_ctrl_init(VI_PIPE ViPipe);
CVI_S32 isp_sts_ctrl_exit(VI_PIPE ViPipe);
CVI_S32 isp_sts_ctrl_pre_fe_eof(VI_PIPE ViPipe);
CVI_S32 isp_sts_ctrl_pre_be_eof(VI_PIPE ViPipe, CVI_U32 frame_cnt);
CVI_S32 isp_sts_ctrl_post_eof(VI_PIPE ViPipe, CVI_U32 frame_cnt);

CVI_S32 isp_sts_ctrl_get_ae_sts(VI_PIPE ViPipe, ISP_AE_STATISTICS_COMPAT_S **ae_sts);
CVI_S32 isp_sts_ctrl_get_awb_sts(VI_PIPE ViPipe, ISP_CHANNEL_LIST_E chn, ISP_WB_STATISTICS_S **awb_sts);
CVI_S32 isp_sts_ctrl_get_af_sts(VI_PIPE ViPipe, ISP_AF_STATISTICS_S **af_sts);
CVI_S32 isp_sts_ctrl_get_gms_sts(VI_PIPE ViPipe, ISP_DIS_STATS_INFO **disStatsInfo);
CVI_S32 isp_sts_ctrl_get_dci_sts(VI_PIPE ViPipe, ISP_DCI_STATISTICS_S **dciStatsInfo);
CVI_S32 isp_sts_ctrl_get_motion_sts(VI_PIPE ViPipe, ISP_MOTION_STATS_INFO **motionStatsInfo);

CVI_S32 isp_sts_ctrl_get_pre_be_frm_idx(VI_PIPE ViPipe, CVI_U32 *frame_cnt);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_STS_CTRL_H_
