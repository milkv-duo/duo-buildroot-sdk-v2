/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_interpolate.h
 * Description:
 *
 */

#ifndef _ISP_INTERPOLATE_H_
#define _ISP_INTERPOLATE_H_

#include "isp_comm_inc.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */


// **** CVI_BOOL defined as unsigned char in cvi_type.h ****
#define INTERPOLATE_LINEAR(vipipe, type, u) _Generic((u), \
		const CVI_U8 * : intrplt_u8, \
		CVI_U8 * : intrplt_u8, \
		const CVI_U16 * : intrplt_u16, \
		CVI_U16 * : intrplt_u16, \
		const CVI_U32 * : intrplt_u32, \
		CVI_U32 * : intrplt_u32, \
		const CVI_S8 * : intrplt_s8, \
		CVI_S8 * : intrplt_s8, \
		const CVI_S16 * : intrplt_s16, \
		CVI_S16 * : intrplt_s16, \
		const CVI_S32 * : intrplt_s32, \
		CVI_S32 * : intrplt_s32 \
)(vipipe, type, u)

enum INTPLT_TYPE {
	INTPLT_MIN = 0,
	INTPLT_PRE_ISO,
	INTPLT_POST_ISO,
	INTPLT_PRE_BLCISO,
	INTPLT_POST_BLCISO,
	INTPLT_EXP_RATIO,
	INTPLT_LV,
	INTPLT_MAX
};

CVI_U8 intrplt_u8(VI_PIPE ViPipe, enum INTPLT_TYPE type, const CVI_U8 *arr);
CVI_U16 intrplt_u16(VI_PIPE ViPipe, enum INTPLT_TYPE type, const CVI_U16 *arr);
CVI_U32 intrplt_u32(VI_PIPE ViPipe, enum INTPLT_TYPE type, const CVI_U32 *arr);
CVI_S8 intrplt_s8(VI_PIPE ViPipe, enum INTPLT_TYPE type, const CVI_S8 *arr);
CVI_S16 intrplt_s16(VI_PIPE ViPipe, enum INTPLT_TYPE type, const CVI_S16 *arr);
CVI_S32 intrplt_s32(VI_PIPE ViPipe, enum INTPLT_TYPE type, const CVI_S32 *arr);

CVI_S32 isp_interpolate_update(VI_PIPE ViPipe, CVI_U32 pre_iso, CVI_U32 post_iso,
	CVI_U32 pre_blc_iso, CVI_U32 post_blc_iso, CVI_U32 exp_ratio, CVI_S16 lv);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_INTERPOLATE_H_
