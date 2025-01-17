#pragma once
#include <inttypes.h>
#include <syslog.h>

#define MODULE_NAME "ive"
#define CVIAI_LOG_CHN LOG_LOCAL7
#ifndef NDEBUG
#define LOGD(fmt, ...) syslog(CVIAI_LOG_CHN | LOG_DEBUG, "[%s] " fmt, MODULE_NAME, ##__VA_ARGS__)
#else
#define LOGD(fmt, ...)
#endif
#define LOGI(fmt, ...) syslog(CVIAI_LOG_CHN | LOG_INFO, "[%s] " fmt, MODULE_NAME, ##__VA_ARGS__)
#define LOGN(fmt, ...) syslog(CVIAI_LOG_CHN | LOG_NOTICE, "[%s] " fmt, MODULE_NAME, ##__VA_ARGS__)
#define LOGW(fmt, ...) syslog(CVIAI_LOG_CHN | LOG_WARNING, "[%s] " fmt, MODULE_NAME, ##__VA_ARGS__)
#define LOGE(fmt, ...) syslog(CVIAI_LOG_CHN | LOG_ERR, "[%s] " fmt, MODULE_NAME, ##__VA_ARGS__)
#define LOGC(fmt, ...) syslog(CVIAI_LOG_CHN | LOG_CRIT, "[%s] " fmt, MODULE_NAME, ##__VA_ARGS__)