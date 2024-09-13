/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: cvi_ispd2_log.h
 * Description:
 */

#ifndef _CVI_ISPD2_LOG_H_
#define _CVI_ISPD2_LOG_H_

#include <stdint.h>
#include <syslog.h>

extern uint8_t gu8ISPD2_LogExportLevel;

#define ISP_DAEMON2_LOG_DEBUG_ENABLE

// LOG_EMERG	0	/* system is unusable */
// LOG_ALERT	1	/* action must be taken immediately */
// LOG_CRIT		2	/* critical conditions */
// LOG_ERR		3	/* error conditions */
// LOG_WARNING	4	/* warning conditions */
// LOG_NOTICE	5	/* normal but significant condition */
// LOG_INFO		6	/* informational */
// LOG_DEBUG	7	/* debug-level messages */

#ifdef ISP_DAEMON2_LOG_DEBUG_ENABLE
static const char ISP_DAEMON2_DEBUG_LEVEL_SYMBOL[] = {'e', 'A', 'C', 'E', 'W', 'N', 'I', 'D'};

#define ISP_DAEMON2_DEBUG(u8Level, fmt) { \
	if (u8Level <= gu8ISPD2_LogExportLevel) { \
		printf("[LV]:%c [MSG]:" fmt "\n", ISP_DAEMON2_DEBUG_LEVEL_SYMBOL[u8Level]); \
	} \
}
#define ISP_DAEMON2_DEBUG_EX(u8Level, fmt, ...) { \
	if (u8Level <= gu8ISPD2_LogExportLevel) { \
		printf("[LV]:%c [MSG]:" fmt "\n", ISP_DAEMON2_DEBUG_LEVEL_SYMBOL[u8Level], ##__VA_ARGS__); \
	} \
}
#else
#define ISP_DAEMON2_DEBUG(u8Level, fmt)
#define ISP_DAEMON2_DEBUG_EX(u8Level, fmt, ...)
#endif // ISP_DAEMON2_LOG_DEBUG_ENABLE

#endif // _CVI_ISPD2_LOG_H_
