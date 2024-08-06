// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) Cvitek Co., Ltd. 2022-2023. All rights reserved.
 *
 * File Name: ms41929_driver.c
 * Description: motor kernel space driver entry related code

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
#include "ms41929_driver.h"
#include "ms41929.h"

#define SPI_BUS_NUM (2)
#define SPI_CS_NUM (0)
#define SPI_MODE (SPI_MODE_3)
#define SPI_MAX_SPEED_HZ (2 * 1000 * 1000)
#define SPI_MAX_BISTS_PER_WORD (8)

struct motor_spi motor;

int ms41929_probe(struct spi_device *spi)
{
	struct device *device;
	char *spi_name;
	unsigned int len;

	//pin mux
	ms41929_pinmux_switch();

	//get spi master
	motor.p_master = spi_busnum_to_master(SPI_BUS_NUM);
	if (!motor.p_master) {
		pr_err("[MOTOR] spi_busnum_to_master failed!\n");
		return -1;
	}

	//get spi device
	len = strlen(dev_name(&motor.p_master->dev)) + 3;
	spi_name = kzalloc(len, GFP_KERNEL);
	snprintf(spi_name, len, "%s.%u", dev_name(&motor.p_master->dev), SPI_CS_NUM);

	device = bus_find_device_by_name(&spi_bus_type, NULL, spi_name);
	if (!device) {
		pr_err("[MOTOR] failed to find device!\n");
		return -1;
	}

	motor.p_spi = to_spi_device(device);
	if (!motor.p_spi) {
		pr_err("[MOTOR] failed to find ms41929 spi device!\n");
		return -1;
	}

	kfree(spi_name);

	motor.p_spi->mode = SPI_MODE;
	motor.p_spi->max_speed_hz = SPI_MAX_SPEED_HZ;
	motor.p_spi->bits_per_word = SPI_MAX_BISTS_PER_WORD;

	spi_setup(motor.p_spi);

	//default set
	ms41929_default_set(motor.p_spi);

	return 0;
}

int ms41929_remove(struct spi_device *spi)
{
	motor.p_spi = NULL;
	motor.p_master = NULL;

	ms41929_pinmux_resume();

	return 0;
}

int write(unsigned char addr, unsigned short data)
{
	ms41929_write(motor.p_spi, addr, data);
	return 1;
}

int read(unsigned char addr)
{
	return ms41929_read(motor.p_spi, addr);
}

int apply(void)
{
	ms41929_vdfz(motor.p_spi);
	return 1;
}

int set_zoom_speed(unsigned short speed)
{
	ms41929_set_zoom_speed(motor.p_spi, speed);
	return 1;
}

int set_focus_speed(unsigned short speed)
{
	ms41929_set_focus_speed(motor.p_spi, speed);
	return 1;
}

int zoom_in(unsigned char step)
{
	ms41929_zoom_in(motor.p_spi, step);
	return 1;
}

int zoom_out(unsigned char step)
{
	ms41929_zoom_out(motor.p_spi, step);
	return 1;
}

int focus_in(unsigned char step)
{
	ms41929_focus_in(motor.p_spi, step);
	return 1;
}

int focus_out(unsigned char step)
{
	ms41929_focus_out(motor.p_spi, step);
	return 1;
}

int get_info(struct cvi_lens_info *info)
{
	ms41929_get_info(motor.p_spi, info);
	return 1;
}
