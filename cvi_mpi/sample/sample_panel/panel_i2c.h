/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: panel_i2c.h
 * Description:
 */

#ifndef __PANEL_I2C_H__
#define __PANEL_I2C_H__

typedef union _PANEL_COMMBUS_U {
	CVI_S8 s8I2cDev;
	struct {
		CVI_S8 bit4SspDev : 4;
		CVI_S8 bit4SspCs : 4;
	} s8SspDev;
} PANEL_COMMBUS_U;

typedef union _PANEL_COMMADDR_U {
	CVI_S32 s8I2cAddr;
} PANEL_COMMADDR_U;

typedef struct _PANEL_I2C_INSTR_S {
	int addr;
	int data;
} PANEL_I2C_INSTR_S;

extern PANEL_I2C_INSTR_S bt1120_1080p25_pt1000k_init_cmds[437];

extern PANEL_I2C_INSTR_S bt656_720p25_pt1000k_init_cmds[261];

extern PANEL_I2C_INSTR_S bt656_1080p30_pt1000k_init_cmds[188];

int panel_i2c_init(VO_DEV VoDev);

int panel_write_register(VO_DEV VoDev, int addr, int data);

#endif // __PANEL_I2C_H__