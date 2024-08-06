/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) Cvitek Co., Ltd. 2022-2023. All rights reserved.
 *
 * File Name: ms41929.h
 * Description: motor kernel space driver entry related code

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
#include <linux/types.h>
#include <linux/spi/spi.h>
#include <linux/of_gpio.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include "motor_driver.h"

#define MSB_LSB_MODE   (1) //0:MSB 1:LSB
#define FOCUS_RANGE    (18682)
#define ZOOM_RANGE     (15228)
#define FOCUS_OFFSET   (0)
#define ZOOM_OFFSET    (8992)
#define FOCUS_BACKLASH (200)
#define ZOOM_BACKLASH  (200)
#define ONE_STEP_TIME_COST  (590) //us

void ms41929_pinmux_switch(void);
void ms41929_pinmux_resume(void);
int ms41929_vdfz(struct spi_device *p_spi);
int ms41929_write(struct spi_device *p_spi, unsigned char addr, unsigned short data);
int ms41929_read(struct spi_device *p_spi, unsigned char addr);
int ms41929_default_set(struct spi_device *p_spi);
int ms41929_set_zoom_speed(struct spi_device *p_spi, unsigned char speed);
int ms41929_set_focus_speed(struct spi_device *p_spi, unsigned char speed);
int ms41929_zoom_in(struct spi_device *p_spi, unsigned char step);
int ms41929_zoom_out(struct spi_device *p_spi, unsigned char step);
int ms41929_focus_in(struct spi_device *p_spi, unsigned char step);
int ms41929_focus_out(struct spi_device *p_spi, unsigned char step);
int ms41929_get_info(struct spi_device *p_spi, struct cvi_lens_info *info);
