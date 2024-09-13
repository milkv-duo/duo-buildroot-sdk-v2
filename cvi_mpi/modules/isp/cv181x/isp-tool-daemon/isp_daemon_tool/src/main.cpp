/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: main.cpp
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <memory>
#include <csignal>
#include <syslog.h>
#include <unistd.h>

#include "cvi_streamer.hpp"
#include "isp_tool_daemon_comm.hpp"

extern "C" {
#include "cvi_ispd2.h"
#include "raw_dump.h"
}

using std::shared_ptr;
using cvitek::system::CVIStreamer;

static CVI_U32 gLoopRun;

static void signalHandler(int iSigNo)
{
	switch (iSigNo) {
		case SIGINT:
		case SIGTERM:
			gLoopRun = 0;
			break;

		default:
			break;
	}
}

static void initSignal()
{
	struct sigaction sigaInst;

	sigaInst.sa_flags = 0;
	sigemptyset(&sigaInst.sa_mask);
	sigaddset(&sigaInst.sa_mask, SIGINT);
	sigaddset(&sigaInst.sa_mask, SIGTERM);

	sigaInst.sa_handler = signalHandler;
	sigaction(SIGINT, &sigaInst, NULL);
	sigaction(SIGTERM, &sigaInst, NULL);
}

static int getRTSPMode()
{
	const char *input = getenv("CVI_RTSP_MODE");

	int single_output = 1;		// default is single output (for internal application)

	if (input) {
		int rtspMode = atoi(input);
		if (rtspMode != 0) {
			single_output = 0;
		}
	}

	return single_output;
}

#define JSONRPC_PORT	(5566)

int main(void)
{
	openlog("ISP Tool Daemon", 0, LOG_USER);

	shared_ptr<CVIStreamer> cvi_streamer =
		make_shared<CVIStreamer>();

	//sample_common_platform.c will sigaction _SAMPLE_PLAT_SYS_HandleSig
	//we substitute signalHander for _SAMPLE_PLAT_SYS_HandleSig
	initSignal();

	isp_daemon2_init(JSONRPC_PORT);

	cvi_raw_dump_init();

	int single_output = getRTSPMode();

	isp_daemon2_enable_device_bind_control(single_output ? 1 : 0);

	gLoopRun = 1;
	while (gLoopRun) {
		sleep(1);
	}

	cvi_raw_dump_uninit();

	isp_daemon2_uninit();

	closelog();
	return 0;
}
