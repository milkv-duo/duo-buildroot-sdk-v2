/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: stream_porting.h
 * Description:
 *
 */

#ifndef __STREAM_PORTING_H
#define __STREAM_PORTING_H

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */
#include <stdio.h>

/* do not modify this, it match with Makefile */
#define STREAM_TYPE_LIVE555 2
#define STREAM_TYPE_PANEL 3

#if STREAM_TYPE == STREAM_TYPE_LIVE555
#include <service.h>
#else

#endif

CVI_BOOL stream_porting_init(void);
CVI_BOOL stream_porting_run(void);
CVI_BOOL stream_porting_stop(void);
CVI_BOOL stream_porting_uninit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif //__STREAM_PORTING_H
