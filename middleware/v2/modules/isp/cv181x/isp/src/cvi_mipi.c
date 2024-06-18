/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cvi_mipi.c
 * Description:
 *
 */
#include <errno.h>
#include <sys/ioctl.h>

#include "cvi_sns_ctrl.h"

#include "isp_main_local.h"
#include "cvi_isp.h"
#include "cvi_comm_isp.h"
#include "isp_debug.h"

#define MIPI_DEV_NODE "/dev/cvi-mipi-rx"

CVI_S32 fd_mipi = -1;

CVI_S32 mipi_open_dev(CVI_VOID)
{
	open_device(MIPI_DEV_NODE, &fd_mipi);
	if (fd_mipi < 0)
		return CVI_FAILURE;

	return CVI_SUCCESS;
}

CVI_S32 CVI_MIPI_SetMipiReset(CVI_S32 devno, CVI_U32 reset)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	CVI_S32 s32Devno = devno;

	if (fd_mipi < 0) {
		s32Ret = mipi_open_dev();
		if (s32Ret != CVI_SUCCESS)
			return s32Ret;
	}

	if (reset == 0) {
		if (ioctl(fd_mipi, CVI_MIPI_UNRESET_MIPI, (void *)(uintptr_t)&s32Devno) < 0) {
			ISP_LOG_ERR("CVI_MIPI_UNRESET_MIPI - %d NG\n", s32Devno);
			return errno;
		}
	} else {
		if (ioctl(fd_mipi, CVI_MIPI_RESET_MIPI, (void *)(uintptr_t)&s32Devno) < 0)  {
			ISP_LOG_ERR("CVI_MIPI_RESET_MIPI - %d NG\n", s32Devno);
			return errno;
		}
	}
	return s32Ret;
}

CVI_S32 CVI_MIPI_SetSensorClock(CVI_S32 devno, CVI_U32 enable)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	CVI_S32 s32Devno = devno;

	if (fd_mipi < 0) {
		s32Ret = mipi_open_dev();
		if (s32Ret != CVI_SUCCESS)
			return s32Ret;
	}

	if (enable == 0) {
		if (ioctl(fd_mipi, CVI_MIPI_DISABLE_SENSOR_CLOCK, (void *)(uintptr_t)&s32Devno) < 0) {
			ISP_LOG_ERR("CVI_MIPI_DISABLE_SENSOR_CLOCK - %d NG\n", s32Devno);
			return errno;
		}
	} else {
		if (ioctl(fd_mipi, CVI_MIPI_ENABLE_SENSOR_CLOCK, (void *)(uintptr_t)&s32Devno) < 0) {
			ISP_LOG_ERR("CVI_MIPI_ENABLE_SENSOR_CLOCK - %d NG\n", s32Devno);
			return errno;
		}
	}
	return s32Ret;
}

CVI_S32 CVI_MIPI_SetSensorReset(CVI_S32 devno, CVI_U32 reset)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	CVI_S32 s32Devno = devno;

	if (fd_mipi < 0) {
		s32Ret = mipi_open_dev();
		if (s32Ret != CVI_SUCCESS)
			return s32Ret;
	}

//  printf("Func %s %d %d fd_mipi %d\n", __func__, devno, reset, fd_mipi);
	if (reset == 0) {
		if (ioctl(fd_mipi, CVI_MIPI_UNRESET_SENSOR, (void *)(uintptr_t)&s32Devno) < 0)  {
			ISP_LOG_ERR("CVI_MIPI_DISABLE_SENSOR_CLOCK - %d NG\n", s32Devno);
			return errno;
		}
	} else {
		if (ioctl(fd_mipi, CVI_MIPI_RESET_SENSOR, (void *)(uintptr_t)&s32Devno) < 0) {
			ISP_LOG_ERR("CVI_MIPI_RESET_SENSOR - %d NG\n", s32Devno);
			return errno;
		}
	}
	return s32Ret;
}

CVI_S32 CVI_MIPI_SetMipiAttr(CVI_S32 ViPipe, const CVI_VOID *devAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	if (devAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 s32Ret = 0;
	SNS_COMBO_DEV_ATTR_S *comboAttr;

	if (fd_mipi < 0) {
		s32Ret = mipi_open_dev();
		if (s32Ret != CVI_SUCCESS)
			return s32Ret;
	}

	comboAttr = (SNS_COMBO_DEV_ATTR_S *)devAttr;
	if (ioctl(fd_mipi, CVI_MIPI_SET_DEV_ATTR, (void *)(uintptr_t)comboAttr) < 0) {
		ISP_LOG_ERR("CVI_MIPI_SET_DEV_ATTR NG\n");
		return errno;
	}
	return s32Ret;
}

CVI_S32 CVI_MIPI_SetClkEdge(CVI_S32 devno, CVI_U32 is_up)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	struct clk_edge_s clk;

	if (fd_mipi < 0) {
		s32Ret = mipi_open_dev();
		if (s32Ret != CVI_SUCCESS)
			return s32Ret;
	}

	clk.devno = devno;
	clk.edge = is_up ? CLK_UP_EDGE : CLK_DOWN_EDGE;
	if (ioctl(fd_mipi, CVI_MIPI_SET_OUTPUT_CLK_EDGE, (void *)(uintptr_t)&clk) < 0)  {
		ISP_LOG_ERR("CVI_MIPI_SET_OUTPUT_CLK_EDGE, - %d NG\n", devno);
		return errno;
	}
	return s32Ret;
}

CVI_S32 CVI_MIPI_SetSnsMclk(SNS_MCLK_S *mclk)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	SNS_MCLK_ATTR_S attr;

	if (mclk == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (fd_mipi < 0) {
		s32Ret = mipi_open_dev();
		if (s32Ret != CVI_SUCCESS)
			return s32Ret;
	}

	attr.cam = mclk->u8Cam;
	attr.freq = (CVI_U32)mclk->enFreq;
	if (ioctl(fd_mipi, CVI_MIPI_SET_SENSOR_CLOCK, (void *)&attr) < 0) {
		ISP_LOG_ERR("CVI_MIPI_SET_SENSOR_CLOCK NG\n");
		return errno;
	}
	return s32Ret;
}
