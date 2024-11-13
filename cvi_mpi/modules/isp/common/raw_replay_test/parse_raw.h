/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: parse_raw.h
 * Description:
 *
 */

#ifndef _PARSE_RAW_H_
#define _PARSE_RAW_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

CVI_S32 parse_raw_init(const char *path, const char *roi_info, CVI_U32 *pu32TotalItem);

CVI_S32 get_raw_data_info(CVI_U32 u32ItemIndex, CVI_U32 *pu32TotalFrame, char **prefix);

CVI_S32 get_raw_data(CVI_U32 u32FrameIndex, void **pHeader, void **pData, CVI_U32 *pu32RawSize);

CVI_S32 parse_raw_deinit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _PARSE_RAW_H_
