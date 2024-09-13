/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_tun_buf_ctrl.h
 * Description:
 *
 */

#ifndef _ISP_TUN_BUF_CTRL_H_
#define _ISP_TUN_BUF_CTRL_H_

#include "isp_comm_inc.h"
// #include <linux/vi_uapi.h>
// #include <linux/vi_isp.h>


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct isp_tun_buf_ctrl_runtime {
	CVI_U32 widx;
	CVI_U32 ridx;

	//struct isp_tuning_cfg tun_buf_info;
	CVI_U64 fe_addr;
	CVI_U64 be_addr;
	CVI_U64 post_addr;

	CVI_BOOL is_init;
};

#define GET_PRE_TUN_ADDR(ViPipe, module) \
	isp_tun_get_pre_##module##_tun(ViPipe, struct cvi_vip_##module##config **)

#define GET_POST_TUN_ADDR(ViPipe, module) \
	isp_tun_get_post_##module##_tun(ViPipe, struct cvi_vip_##module##config **)

CVI_S32 isp_tun_buf_ctrl_init(VI_PIPE ViPipe);
CVI_S32 isp_tun_buf_ctrl_uninit(VI_PIPE ViPipe);
CVI_S32 isp_tun_buf_invalid(VI_PIPE ViPipe);
CVI_S32 isp_tun_buf_flush(VI_PIPE ViPipe);
CVI_S32 isp_tun_buf_ctrl_frame_done(VI_PIPE ViPipe);


// TODO@ST Set as private function
struct cvi_vip_isp_fe_cfg *get_pre_fe_tuning_buf_addr(VI_PIPE ViPipe);
struct cvi_vip_isp_be_cfg *get_pre_be_tuning_buf_addr(VI_PIPE ViPipe);
struct cvi_vip_isp_post_cfg *get_post_tuning_buf_addr(VI_PIPE ViPipe);
CVI_U32 get_tuning_buf_idx(VI_PIPE ViPipe);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_TUN_BUF_CTRL_H_
