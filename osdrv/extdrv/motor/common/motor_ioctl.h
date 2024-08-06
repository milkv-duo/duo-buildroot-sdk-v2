/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: cvi_motor_ioctl.h
 * Description:
 */

#ifndef __CVI_MOTOR_IOCTL_H__
#define __CVI_MOTOR_IOCTL_H__

#include "cvi_errno.h"

struct cvi_motor_regval {
	unsigned char addr;
	unsigned short val;
};

//refer by cvi_comm_3a.h
struct cvi_lens_info {
	unsigned int focus_range;
	unsigned int zoom_range;
	unsigned int focus_offset;
	unsigned int zoom_offset;
	unsigned int focus_backlash;
	unsigned int zoom_backlash;
	unsigned int time_cost_one_step;//unit is us
};

#define CVI_MOTOR_IOC_MAGIC      'm'
#define CVI_MOTOR_IOC_READ       _IOWR(CVI_MOTOR_IOC_MAGIC, 0x00, unsigned long long)
#define CVI_MOTOR_IOC_WRITE      _IOW(CVI_MOTOR_IOC_MAGIC, 0x01, unsigned long long)
#define CVI_MOTOR_IOC_ZOOM_IN    _IOW(CVI_MOTOR_IOC_MAGIC, 0x02, unsigned long long)
#define CVI_MOTOR_IOC_ZOOM_OUT   _IOW(CVI_MOTOR_IOC_MAGIC, 0x03, unsigned long long)
#define CVI_MOTOR_IOC_FOCUS_IN   _IOW(CVI_MOTOR_IOC_MAGIC, 0x04, unsigned long long)
#define CVI_MOTOR_IOC_FOCUS_OUT  _IOW(CVI_MOTOR_IOC_MAGIC, 0x05, unsigned long long)
#define CVI_MOTOR_IOC_SET_ZOOM_SPEED   _IOW(CVI_MOTOR_IOC_MAGIC, 0x06, unsigned long long)
#define CVI_MOTOR_IOC_SET_FOCUS_SPEED  _IOW(CVI_MOTOR_IOC_MAGIC, 0x07, unsigned long long)
#define CVI_MOTOR_IOC_APPLY            _IOW(CVI_MOTOR_IOC_MAGIC, 0x08, unsigned long long)
#define CVI_MOTOR_IOC_GET_INFO         _IOW(CVI_MOTOR_IOC_MAGIC, 0x09, unsigned long long)

//#define CVI_MOTOR_IOC_ACC_ADJUST _IO(CVI_MOTOR_IOC_MAGIC, 0x11)
#endif /* __CVI_MOTOR_IOCTL_H__ */
