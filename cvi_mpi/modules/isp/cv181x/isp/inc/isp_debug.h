/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_debug.h
 * Description:
 *
 */

#ifndef _ISP_DEBUG_H_
#define _ISP_DEBUG_H_

#include <stdio.h>
#include "isp_comm_inc.h"
#include "clog.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

void CVI_DEBUG_SetDebugLevel(int level);
CVI_S32 isp_dbg_dumpFrameRawInfoToFile(VI_PIPE ViPipe, FILE *fp);

void isp_dbg_init(void);
void isp_dbg_deinit(void);
CVI_U32 isp_dbg_get_time_diff_us(const struct timeval *pre, const struct timeval *cur);

#define IGNORE_LOG_DEBUG

#ifndef ISP_LOG_TAG
#define ISP_LOG_ASSERT(...) clog_assert("isp", __VA_ARGS__)

#ifndef IGNORE_LOG_ERR
#define ISP_LOG_ERR(...) clog_error("isp", __VA_ARGS__)
#else
#define ISP_LOG_ERR(...)
#endif // IGNORE_LOG_ERR

#ifndef IGNORE_LOG_WARNING
#define ISP_LOG_WARNING(...) clog_warn("isp", __VA_ARGS__)
#else
#define ISP_LOG_WARNING(...)
#endif // IGNORE_LOG_WARNING

#ifndef IGNORE_LOG_NOTICE
#define ISP_LOG_NOTICE(...) clog_info("isp", __VA_ARGS__)
#else
#define ISP_LOG_NOTICE(...)
#endif // IGNORE_LOG_NOTICE

#ifndef IGNORE_LOG_INFO
#define ISP_LOG_INFO(...) clog_info("isp", __VA_ARGS__)
#else
#define ISP_LOG_INFO(...)
#endif // IGNORE_LOG_INFO

#ifndef IGNORE_LOG_DEBUG
#define ISP_LOG_DEBUG(...) clog_debug("isp", __VA_ARGS__)
#else
#define ISP_LOG_DEBUG(...)
#endif // IGNORE_LOG_DEBUG
#else
#define ISP_LOG_ASSERT(tag, ...) clog_assert(tag, __VA_ARGS__)

#ifndef IGNORE_LOG_ERR
#define ISP_LOG_ERR(tag, ...) clog_error(tag, __VA_ARGS__)
#else
#define ISP_LOG_ERR(tag, ...)
#endif // IGNORE_LOG_ERR

#ifndef IGNORE_LOG_WARNING
#define ISP_LOG_WARNING(tag, ...) clog_warn(tag, __VA_ARGS__)
#else
#define ISP_LOG_WARNING(tag, ...)
#endif // IGNORE_LOG_WARNING

#ifndef IGNORE_LOG_NOTICE
#define ISP_LOG_NOTICE(tag, ...) clog_info(tag, __VA_ARGS__)
#else
#define ISP_LOG_NOTICE(tag, ...)
#endif // IGNORE_LOG_NOTICE

#ifndef IGNORE_LOG_INFO
#define ISP_LOG_INFO(tag, ...) clog_info(tag, __VA_ARGS__)
#else
#define ISP_LOG_INFO(tag, ...)
#endif // IGNORE_LOG_INFO

#ifndef IGNORE_LOG_DEBUG
#define ISP_LOG_DEBUG(tag, ...) clog_debug(tag, __VA_ARGS__)
#else
#define ISP_LOG_DEBUG(tag, ...)
#endif // IGNORE_LOG_DEBUG
#endif // ISP_LOG_TAG

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif //_ISP_DEBUG_H_
