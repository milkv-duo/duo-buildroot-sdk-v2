
#ifndef _CLOG_H_
#define _CLOG_H_

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************/
#define CLOG_LINE_BUF_SIZE  (2 * 1024)

/* clog file cfg */
#define CLOG_FILE_PATH  "/mnt/sd/"
#define CLOG_FILE_MAX_PATH_LEN  64
#define CLOG_FILE_ENABLE_SINGLE_FILE 1
#define CLOG_FILE_SPLIT_SIZE (50 * 1024 * 1024)
#define CLOG_FILE_ASYNC_PINGPONG_BUF_SIZE (50 * 1024)

/*******************************************************************************/
#define CLOG_LVL_ASSERT                      0
#define CLOG_LVL_ERROR                       1
#define CLOG_LVL_WARN                        2
#define CLOG_LVL_INFO                        3
#define CLOG_LVL_DEBUG                       4
#define CLOG_LVL_VERBOSE                     5

#define CLOG_OUTPUT_LVL CLOG_LVL_DEBUG

#if CLOG_OUTPUT_LVL >= CLOG_LVL_ASSERT
	#define clog_assert(tag, ...) \
			clog_output(CLOG_LVL_ASSERT, tag, __func__, __LINE__, __VA_ARGS__)
#else
	#define clog_assert(tag, ...)
#endif

#if CLOG_OUTPUT_LVL >= CLOG_LVL_ERROR
	#define clog_error(tag, ...) \
			clog_output(CLOG_LVL_ERROR, tag, __func__, __LINE__, __VA_ARGS__)
#else
	#define clog_error(tag, ...)
#endif

#if CLOG_OUTPUT_LVL >= CLOG_LVL_WARN
	#define clog_warn(tag, ...) \
			clog_output(CLOG_LVL_WARN, tag, __func__, __LINE__, __VA_ARGS__)
#else
	#define clog_warn(tag, ...)
#endif

#if CLOG_OUTPUT_LVL >= CLOG_LVL_INFO
	#define clog_info(tag, ...) \
			clog_output(CLOG_LVL_INFO, tag, __func__, __LINE__, __VA_ARGS__)
#else
	#define clog_info(tag, ...)
#endif

#if CLOG_OUTPUT_LVL >= CLOG_LVL_DEBUG
	#define clog_debug(tag, ...) \
			clog_output(CLOG_LVL_DEBUG, tag, __func__, __LINE__, __VA_ARGS__)
#else
	#define elog_debug(tag, ...)
#endif

#if CLOG_OUTPUT_LVL >= CLOG_LVL_VERBOSE
	#define clog_verbose(tag, ...) \
			clog_output(CLOG_LVL_VERBOSE, tag, __func__, __LINE__, __VA_ARGS__)
#else
	#define clog_verbose(tag, ...)
#endif

#ifndef CLOG_TAG
#define cloga(...)     clog_assert("isp", __VA_ARGS__)
#define cloge(...)     clog_error("isp", __VA_ARGS__)
#define clogw(...)     clog_warn("isp", __VA_ARGS__)
#define clogi(...)     clog_info("isp", __VA_ARGS__)
#define clogd(...)     clog_debug("isp", __VA_ARGS__)
#define clogv(...)     clog_verbose("isp", __VA_ARGS__)
#else
#define cloga(tag, ...)     clog_assert(tag, __VA_ARGS__)
#define cloge(tag, ...)     clog_error(tag, __VA_ARGS__)
#define clogw(tag, ...)     clog_warn(tag, __VA_ARGS__)
#define clogi(tag, ...)     clog_info(tag, __VA_ARGS__)
#define clogd(tag, ...)     clog_debug(tag, __VA_ARGS__)
#define clogv(tag, ...)     clog_verbose(tag, __VA_ARGS__)
#endif

#define CLOG_ASSERT(EXPR)                        \
	{if (!(EXPR)) {                              \
		cloga("assert", "%s\n", __func__);       \
	}}

int clog_init(void);
int clog_deinit(void);
int clog_set_level(uint8_t level);
int clog_file_enable(void);
int clog_file_disable(void);
void clog_output(uint8_t level, const char *tag, const char *func,
	const long line, const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif
