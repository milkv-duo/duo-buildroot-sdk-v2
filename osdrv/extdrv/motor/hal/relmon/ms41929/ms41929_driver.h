/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) Cvitek Co., Ltd. 2022-2023. All rights reserved.
 *
 * File Name: ms41929_driver.h
 * Description: motor kernel space driver entry related code

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/sysfs.h>
#include <linux/version.h>
#include <linux/compat.h>
#include <linux/iommu.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/gpio.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_gpio.h>
#include <linux/spi/spi.h>
#include "motor_driver.h"

struct motor_spi {
	struct spi_master *p_master;        /* spi master */
	struct spi_device *p_spi;           /* spi device */
};

int ms41929_probe(struct spi_device *spi);
int ms41929_remove(struct spi_device *spi);
int write(unsigned char addr, unsigned short data);
int read(unsigned char addr);
int apply(void);
int set_zoom_speed(unsigned char speed);
int set_focus_speed(unsigned char speed);
int zoom_in(unsigned char step);
int zoom_out(unsigned char step);
int focus_in(unsigned char step);
int focus_out(unsigned char step);
int get_info(struct cvi_lens_info *info);
