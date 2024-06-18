/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cvi_streamer.cpp
 * Description:
 *
 */

#include <algorithm>
#include "cvi_streamer.hpp"

extern "C" {
#include "ini.h"
#include "cvi_bin.h"
#include "cvi_vi.h"
#include "common/sample_comm.h"
#include "stream_porting.h"
}

#define ISP_DAEMON_TOOL_LOG(level, fmt, ...) do {		\
	if (1)												\
		syslog(level, fmt, ##__VA_ARGS__);				\
	if (1)												\
		printf("[%d]" fmt "\n", level, ##__VA_ARGS__);	\
} while (0)

#define ISP_CONFIG_PATH "./config.ini"
#define ENV_ISP_CONFIG_PATH	"ISP_CONFIG_PATH"
namespace cvitek {
namespace system {

typedef struct {
	CVI_U8 logLevel;
	CVI_BOOL isEnableSetPQBin;
	CVI_CHAR SDR_PQBinName[BIN_FILE_LENGTH];
	CVI_CHAR WDR_PQBinName[BIN_FILE_LENGTH];
	CVI_BOOL isEnableSetSnsCfg;
	CVI_CHAR SnsCfgPath[BIN_FILE_LENGTH];
} ISP_CONFIG_ST;

static CVI_S32 parse_handler(void *user, const char *section, const char *name, const char *value)
{
	ISP_CONFIG_ST *cfg = (ISP_CONFIG_ST *)user;

	if (strcmp(section, "LOG") == 0) {
		if (strcmp(name, "level") == 0) {
			cfg->logLevel = atoi(value);
			ISP_DAEMON_TOOL_LOG(LOG_INFO, "level = %d", cfg->logLevel);
		}
	}

	if (strcmp(section, "PQBIN") == 0) {
		if (strcmp(name, "isEnableSetPQBin") == 0) {
			cfg->isEnableSetPQBin = atoi(value);
			ISP_DAEMON_TOOL_LOG(LOG_INFO, "isEnableSetPQBin = %d", cfg->isEnableSetPQBin);
		} else if (strcmp(name, "SDR_PQBinName") == 0) {
			if (strlen(value) < BIN_FILE_LENGTH) {
				snprintf(cfg->SDR_PQBinName, BIN_FILE_LENGTH, "%s", value);
				ISP_DAEMON_TOOL_LOG(LOG_INFO, "SDR_PQBinName = %s", cfg->SDR_PQBinName);
			} else {
				ISP_DAEMON_TOOL_LOG(LOG_ERR, "The size of SDR_PQBinName[%zu] is larger than %d",
					strlen(value), BIN_FILE_LENGTH);
			}
		} else if (strcmp(name, "WDR_PQBinName") == 0) {
			if (strlen(value) < BIN_FILE_LENGTH) {
				snprintf(cfg->WDR_PQBinName, BIN_FILE_LENGTH, "%s", value);
				ISP_DAEMON_TOOL_LOG(LOG_INFO, "WDR_PQBinName = %s", cfg->WDR_PQBinName);
			} else {
				ISP_DAEMON_TOOL_LOG(LOG_ERR, "The size of WDR_PQBinName[%zu] is larger than %d",
					strlen(value), BIN_FILE_LENGTH);
			}
		} else if (strcmp(name, "isEnableSetSnsCfgPath") == 0) {
			cfg->isEnableSetSnsCfg = atoi(value);
			ISP_DAEMON_TOOL_LOG(LOG_INFO, "isEnableSetSnsCfg = %d", cfg->isEnableSetSnsCfg);
		} else if (strcmp(name, "SnsCfgPath") == 0) {
			if (strlen(value) < BIN_FILE_LENGTH) {
				snprintf(cfg->SnsCfgPath, BIN_FILE_LENGTH, "%s", value);
				ISP_DAEMON_TOOL_LOG(LOG_INFO, "SnsCfgPath = %s", cfg->SnsCfgPath);
			} else {
				ISP_DAEMON_TOOL_LOG(LOG_ERR, "The size of SnsCfgPath[%zu] is larger than %d",
					strlen(value), BIN_FILE_LENGTH);
			}
		}
	}

	return 1;
}

static CVI_S32 parseIspToolDaemonIni(void)
{
	//0. init
	CVI_S32 ret = 0;
	ISP_CONFIG_ST stIspConfig;
	SAMPLE_INI_CFG_S stIniCfg = {};

	stIniCfg.devNum = 0;
	stIniCfg.enWDRMode[0] = WDR_MODE_NONE;
	memset(&stIspConfig, 0, sizeof(ISP_CONFIG_ST));
	stIspConfig.logLevel = CVI_DBG_DEBUG + 1;

	//1. If the partition(/mnt/data) is read-only or does not exist,
	//   we can also manually specify the location of ini file
	char *pIspCfgPath = getenv(ENV_ISP_CONFIG_PATH);

	if (pIspCfgPath == NULL)
		pIspCfgPath = (char *)ISP_CONFIG_PATH;

	//2. parse ini and set
	if (ini_parse(ISP_CONFIG_PATH, parse_handler, &stIspConfig) != -1) {
		if (stIspConfig.isEnableSetSnsCfg == 1) {
			SAMPLE_COMM_VI_SetIniPath(stIspConfig.SnsCfgPath);
		}
		if (stIspConfig.logLevel <= CVI_DBG_DEBUG) {
			LOG_LEVEL_CONF_S log_conf;
			log_conf.enModId = CVI_ID_ISP;
			log_conf.s32Level = stIspConfig.logLevel;
			CVI_LOG_SetLevelConf(&log_conf);
			ISP_DAEMON_TOOL_LOG(LOG_INFO, "Set log level[%d] done", log_conf.s32Level);
		}

		ret = SAMPLE_COMM_VI_ParseIni(&stIniCfg);
		if (stIspConfig.isEnableSetPQBin == 1) {
			// read wdr mode from sensor_cfg.ini
			if (ret) {
				if (stIniCfg.enWDRMode[0] <= WDR_MODE_QUDRA) {
					CVI_BIN_SetBinName(stIniCfg.enWDRMode[0], stIspConfig.SDR_PQBinName);
					ISP_DAEMON_TOOL_LOG(LOG_INFO, "SdrMode[%d] Set SdrPqBin[%s] done",
						stIniCfg.enWDRMode[0], stIspConfig.SDR_PQBinName);
				} else {
					CVI_BIN_SetBinName(stIniCfg.enWDRMode[0], stIspConfig.WDR_PQBinName);
					ISP_DAEMON_TOOL_LOG(LOG_INFO, "WdrMode[%d] Set WdrPqBin[%s] done",
						stIniCfg.enWDRMode[0], stIspConfig.WDR_PQBinName);
				}
			} else {
				ISP_DAEMON_TOOL_LOG(LOG_INFO, "%s", "Sensor Config ini parse fail");
			}
		}
	} else {
		ISP_DAEMON_TOOL_LOG(LOG_INFO, "Not found %s", ISP_CONFIG_PATH);
		ISP_DAEMON_TOOL_LOG(LOG_INFO, "%s",
			"********************************************************************************");
		ISP_DAEMON_TOOL_LOG(LOG_INFO, "%s",
			"if you want to do some settings, you can touch file Contains the following information");
		ISP_DAEMON_TOOL_LOG(LOG_INFO, "%s", "[LOG]");
		ISP_DAEMON_TOOL_LOG(LOG_INFO, "%s", "level = 6");
		ISP_DAEMON_TOOL_LOG(LOG_INFO, "%s", "[PQBIN]");
		ISP_DAEMON_TOOL_LOG(LOG_INFO, "%s", "isEnableSetPQBin = 1");
		ISP_DAEMON_TOOL_LOG(LOG_INFO, "%s", "SDR_PQBinName = /PATH/NAME");
		ISP_DAEMON_TOOL_LOG(LOG_INFO, "%s", "WDR_PQBinName = /PATH/NAME");
		ISP_DAEMON_TOOL_LOG(LOG_INFO, "%s",
			"********************************************************************************");
	}

	//3. we call CVI_VI_SetDevNum because of isp_tool_daemon.tar.gz will rmmod & insmod ko file
	if (ret) {
		CVI_VI_SetDevNum(stIniCfg.devNum);
		ISP_DAEMON_TOOL_LOG(LOG_INFO, "%s %d", "CVI_VI_SetDevNum:", stIniCfg.devNum);
	} else {
		ret = SAMPLE_COMM_VI_ParseIni(&stIniCfg);
		if (ret) {
			CVI_VI_SetDevNum(stIniCfg.devNum);
			ISP_DAEMON_TOOL_LOG(LOG_INFO, "%s %d", "CVI_VI_SetDevNum:", stIniCfg.devNum);
		} else {
			ISP_DAEMON_TOOL_LOG(LOG_INFO, "%s", "Sensor Config ini parse fail");
		}
	}
	return ret;
}

CVI_S32 CVIStreamer::stream_run()
{
	stream_porting_init();
	stream_porting_run();

	return CVI_SUCCESS;
}

CVI_S32 CVIStreamer::stream_exit()
{
	stream_porting_stop();
	stream_porting_uninit();

	return CVI_SUCCESS;
}

CVIStreamer::CVIStreamer()
{
	MMF_VERSION_S		stVersion;

	CVI_SYS_GetVersion(&stVersion);
	printf("MMF Version:%s\n", stVersion.version);

	parseIspToolDaemonIni();

	stream_run();
}

CVIStreamer::~CVIStreamer()
{
	stream_exit();
}

} // namespace system
} // namespace cvitek
