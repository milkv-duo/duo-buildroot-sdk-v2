/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: sample_vio.h
 * Description:
 */

#ifndef __SAMPLE_VIO_H__
#define __SAMPLE_VIO_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include <linux/cvi_common.h>

CVI_S32 SAMPLE_VIO_TWO_DEV_VO(void);
CVI_S32 SAMPLE_VIO_VoRotation(void);
CVI_S32 SAMPLE_VIO_ViVpssAspectRatio(void);
CVI_S32 SAMPLE_VIO_ViRotation(void);
CVI_S32 SAMPLE_VIO_VpssRotation(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __SAMPLE_VIO_H__*/
