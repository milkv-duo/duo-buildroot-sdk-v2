/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) Cvitek Co., Ltd. 2022-2023. All rights reserved.
 *
 * File Name: motor_driver.h
 * Description: motor kernel space driver entry related code

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef __CVI_MOTOR_DRIVER_H__
#define __CVI_MOTOR_DRIVER_H__

#include <linux/cdev.h>
#include <linux/completion.h>
#include <linux/version.h>
#include <linux/spi/spi.h>
#include "motor_ioctl.h"

#ifdef DEBUG
#define CVI_DBG_INFO(fmt, ...) pr_info(fmt, ##__VA_ARGS__)
#else
#define CVI_DBG_INFO(fmt, ...)
#endif

struct cvi_motor_device {
	int						major;
	int						minor;
	dev_t					devid;
	spinlock_t				lock;//
	struct cdev				cdev;
	struct class			*class;
	struct device			*device;
	struct proc_dir_entry	*proc_dir;
};

#endif /* __CVI_MOTOR_DRIVER_H__ */
