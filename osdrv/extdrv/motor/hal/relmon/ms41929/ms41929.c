// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) Cvitek Co., Ltd. 2022-2023. All rights reserved.
 *
 * File Name: ms41929.c
 * Description: motor kernel space driver entry related code

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
#include "ms41929.h"
#include "pinctrl-cv181x.h"

#define ZOOM_REG	(0x24)
#define ZOOM_SPEED  (0x25)
#define FOCUS_REG   (0x29)
#define FOCUS_SPEED (0x2a)

#define ZOOM_PHASE  (4)
#define FOCUS_PHASE (8)

#define VD_FZ_PIN_NUM 469

static unsigned char msb_to_lsb(unsigned char data)
{
#ifdef MSB_LSB_MODE
	data = ((data & 0x80) >> 7) |
		   ((data & 0x40) >> 5) |
		   ((data & 0x20) >> 3) |
		   ((data & 0x10) >> 1) |
		   ((data & 0x08) << 1) |
		   ((data & 0x04) << 3) |
		   ((data & 0x02) << 5) |
		   ((data & 0x01) << 7);
#endif
	return data;
}

static unsigned char lsb_to_msb(unsigned char data)
{
#ifdef MSB_LSB_MODE
	data = ((data & 0x80) >> 7) |
			((data & 0x40) >> 5) |
			((data & 0x20) >> 3) |
			((data & 0x10) >> 1) |
			((data & 0x08) << 1) |
			((data & 0x04) << 3) |
			((data & 0x02) << 5) |
			((data & 0x01) << 7);
#endif
	return data;
}

void ms41929_pinmux_switch(void)
{
	PINMUX_CONFIG(SD1_CLK, SPI2_SCK);//CLOCK
	PINMUX_CONFIG(SD1_CMD, SPI2_SDO);//OUT
	PINMUX_CONFIG(SD1_D0, SPI2_SDI);//IN
	PINMUX_CONFIG(SD1_D3, PWR_GPIO_18);//CS
	PINMUX_CONFIG(VIVO_D0, XGPIOB_21);//VD_FZ
	gpio_request(VD_FZ_PIN_NUM, NULL);
}

void ms41929_pinmux_resume(void)
{
	PINMUX_CONFIG(SD1_CLK, PWR_SPINOR1_SCK);//CLOCK
	PINMUX_CONFIG(SD1_CMD, PWR_SPINOR1_MOSI);//OUT
	PINMUX_CONFIG(SD1_D0, PWR_SPINOR1_MISO);//IN
	PINMUX_CONFIG(SD1_D3, PWR_SPINOR1_CS_X);//CS
	PINMUX_CONFIG(VIVO_D0, XGPIOB_21);//VD_FZ
	gpio_free(VD_FZ_PIN_NUM);
}

int ms41929_vdfz(struct spi_device *p_spi)
{
	gpio_direction_output(VD_FZ_PIN_NUM, 1);
	usleep_range(50, 100);
	gpio_direction_output(VD_FZ_PIN_NUM, 0);
	return 0;
}

int ms41929_write(struct spi_device *p_spi, unsigned char addr, unsigned short data)
{
	unsigned char send[3];

	send[0] = msb_to_lsb(addr);
	send[1] = (unsigned char)msb_to_lsb(data & 0xff);
	send[2] = (unsigned char)msb_to_lsb(data >> 8);

	return spi_write(p_spi, send, 3);
}

int ms41929_read(struct spi_device *p_spi, unsigned char addr)
{
	unsigned char data = msb_to_lsb(addr | 0x40);

	return lsb_to_msb(spi_w8r16(p_spi, data));
}

int ms41929_default_set(struct spi_device *p_spi)
{
	ms41929_write(p_spi, 0x20, 0x5C01);//DT1:303.4us PWMMODE:0b1101 PWMRES:0b1 PWM frq:27Mhz/(13*8*2)=129.8hz
	ms41929_write(p_spi, 0x20, 0x5C01);//DT1:303.4us PWMMODE:0b1101 PWMRES:0b1 PWM frq:27Mhz/(13*8*2)=129.8hz
	ms41929_write(p_spi, 0x22, 0x0001);//a motor DT2A:303.4us PHMODAB:0
	ms41929_write(p_spi, 0x27, 0x0001);//b motor DT2B:303.4us PHMODAB:0
	ms41929_write(p_spi, 0x23, 0x6868);//PPWA/PPWB:176 max duty cycle: 176/ (13 * 8) = 100%
	ms41929_write(p_spi, 0x28, 0x6868);//PPWC/PPWD:176 max duty cycle: 176/ (13 * 8) = 100%

	ms41929_write(p_spi, 0x25, 0x0110);//a motor speed:528
	ms41929_write(p_spi, 0x2A, 0x0110);//b motor speed:528

	ms41929_write(p_spi, 0x0B, 0x0480);//TESTEN1:1
	ms41929_write(p_spi, 0x21, 0x0087);//TESTEN1 setting

	ms41929_vdfz(p_spi);

	return 0;
}

int ms41929_set_zoom_speed(struct spi_device *p_spi, unsigned char speed)
{
	ms41929_write(p_spi, 0x2A, speed * 16);

	return 0;
}

int ms41929_set_focus_speed(struct spi_device *p_spi, unsigned char speed)
{
	ms41929_write(p_spi, 0x25, speed * 16);

	return 0;
}

int ms41929_zoom_in(struct spi_device *p_spi, unsigned char step)
{
	if (step == 32)
		step = 255;
	else
		step = step * 8;

	ms41929_write(p_spi, 0x29, ((0x4 + 0) << 8) | step);//set dir and step

	return 0;
}

int ms41929_zoom_out(struct spi_device *p_spi, unsigned char step)
{
	if (step == 32)
		step = 255;
	else
		step = step * 8;

	ms41929_write(p_spi, 0x29, ((0x4 + 1) << 8) | step);//set dir and step

	return 0;
}

int ms41929_focus_in(struct spi_device *p_spi, unsigned char step)
{
	if (step == 32)
		step = 255;
	else
		step = step * 8;

	ms41929_write(p_spi, 0x24, ((0x4 + 0) << 8) | step);//set focus dir and step

	return 0;
}

int ms41929_focus_out(struct spi_device *p_spi, unsigned char step)
{
	if (step == 32)
		step = 255;
	else
		step = step * 8;

	ms41929_write(p_spi, 0x24, ((0x4 + 1) << 8) | step);//set focus dir and step

	return 0;
}

int ms41929_get_info(struct spi_device *p_spi, struct cvi_lens_info *info)
{
	info->focus_backlash = FOCUS_BACKLASH;
	info->focus_offset = FOCUS_OFFSET;
	info->focus_range = FOCUS_RANGE;
	info->focus_max_speed = FOCUS_MAX_SPEED;
	info->focus_time_cost_one_step = FOCUS_ONE_STEP_TIME_COST;
	info->focus_max_step = FOCUS_MAX_STEP;
	info->zoom_backlash = ZOOM_BACKLASH;
	info->zoom_offset = ZOOM_OFFSET;
	info->zoom_range = ZOOM_RANGE;
	info->zoom_max_speed = ZOOM_MAX_SPEED;
	info->zoom_time_cost_one_step = ZOOM_ONE_STEP_TIME_COST;
	info->zoom_max_step = ZOOM_MAX_STEP;

	memcpy(info->zoom_focus_table, zoom_focus_tab, sizeof(info->zoom_focus_table));

	return 0;
}

