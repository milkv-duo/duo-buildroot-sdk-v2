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

#ifndef MS41929_H
#define MS41929_H

#include <linux/types.h>
#include <linux/spi/spi.h>
#include <linux/of_gpio.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include "motor_driver.h"

//CAILB PARAM CONFIG
#define FOCUS_RANGE    (14000) // 1750
#define ZOOM_RANGE     (16065) //1707
#define FOCUS_OFFSET   (0)
#define ZOOM_OFFSET    (6356) // 1207
#define FOCUS_BACKLASH (140) // 25
#define ZOOM_BACKLASH  (220)
#define FOCUS_MAX_SPEED     (20)//The fastest speed that won't cause a slide
#define ZOOM_MAX_SPEED      (20)//The fastest speed that won't cause a slide
#define FOCUS_ONE_STEP_TIME_COST  (500) //us 2768
#define ZOOM_ONE_STEP_TIME_COST   (500) //us
#define FOCUS_MAX_STEP (255)
#define ZOOM_MAX_STEP  (255)
#define MOTOR_TYPE (0)

static const struct cvi_zoom_focus_tab zoom_focus_tab[ZOOM_FOCUS_TAB_SIZE] = {
	{0, 0, 442},
	{2008, 454, 1081},
	{4016, 1030, 1658},
	{6024, 1734, 2426},
	{8032, 2630, 3322},
	{10040, 3718, 4410},
	{12048, 5126, 5882},
	{14056, 7110, 8058},
	{ZOOM_RANGE, 11142, 13690}
};

//SPI CONFIG
#define MSB_LSB_MODE (1) //sub spi device data mode 0:MSB 1:LSB
#define SPI_BUS_NUM (2)
#define SPI_CS_NUM (0)
#define SPI_MODE (SPI_MODE_3)
#define SPI_MAX_SPEED_HZ (2 * 1000 * 1000)
#define SPI_MAX_BISTS_PER_WORD (8)

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

#endif //MS41929_H
