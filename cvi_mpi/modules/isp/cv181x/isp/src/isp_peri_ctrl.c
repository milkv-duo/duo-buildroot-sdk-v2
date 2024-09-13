/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_peri_ctrl.c
 * Description:
 *
 */

#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>

#include "cvi_debug.h"
#include "isp_debug.h"

#include "isp_peri_ctrl.h"

static const char *gpio_path = "/sys/class/gpio/";

enum GPIO_OP_TYPE {
	GPIO_OP_GET,
	GPIO_OP_SET
};

static CVI_S32 _isp_peri_gpio_imp(CVI_U32 gpio_pin, enum GPIO_OP_TYPE op, CVI_U32 *value);
static CVI_S32 _gpio_operate_inst(CVI_U32 gpio_pin,  const char *op);
static CVI_S32 _gpio_operate_direction(CVI_U32 gpio_pin);
static CVI_S32 _gpio_operate_value(CVI_U32 gpio_pin, enum GPIO_OP_TYPE op, CVI_U32 *value);

CVI_S32 isp_peri_gpio_set(CVI_U32 gpio_pin, CVI_U32 value)
{
	CVI_S32 ret = _isp_peri_gpio_imp(gpio_pin, GPIO_OP_SET, &value);

	return ret;
}

CVI_S32 isp_peri_gpio_get(CVI_U32 gpio_pin, CVI_U32 *value)
{
	CVI_S32 ret = _isp_peri_gpio_imp(gpio_pin, GPIO_OP_GET, value);

	return ret;
}

static CVI_S32 _isp_peri_gpio_imp(CVI_U32 gpio_pin, enum GPIO_OP_TYPE op, CVI_U32 *value)
{
	CVI_S32 ret = 0;

	ret |= _gpio_operate_inst(gpio_pin, "export");

	ret |= _gpio_operate_direction(gpio_pin);

	ret |= _gpio_operate_value(gpio_pin, op, value);

	ret |= _gpio_operate_inst(gpio_pin, "unexport");

	return ret;
}

static CVI_S32 _gpio_operate_inst(CVI_U32 gpio_pin, const char *op)
{
	DIR *dir;
	FILE *fp;
	char file_full_path[64];
	CVI_BOOL should_operate_gpio = false;

	memset(file_full_path, 0, sizeof(file_full_path));
	sprintf(file_full_path, "%s%s%d", gpio_path, "gpio", gpio_pin);

	dir = opendir(file_full_path);

	if (dir) {
		// Directory exist
		closedir(dir);
		if (!strcmp(op, "unexport")) {
			should_operate_gpio = true;
		} else {
			ISP_LOG_DEBUG("%s exist", file_full_path);
			should_operate_gpio = false;
		}
	} else if (errno == ENOENT) {
		// Directory does not exist
		if (!strcmp(op, "export")) {
			should_operate_gpio = true;
		} else {
			ISP_LOG_DEBUG("%s does not exist", file_full_path);
			should_operate_gpio = false;
		}
	} else {
		ISP_LOG_ERR("opendir errno %d\n", errno);
		return 1;
	}

	if (should_operate_gpio) {
		memset(file_full_path, 0, sizeof(file_full_path));
		sprintf(file_full_path, "%s%s", gpio_path, op);

		fp = fopen(file_full_path, "w");
		if (fp == CVI_NULL) {
			ISP_LOG_ERR("fopen %s failed\n", file_full_path);
			return 1;
		}

		fprintf(fp, "%d", gpio_pin);
		fclose(fp);
	}
	return 0;
}

static CVI_S32 _gpio_operate_direction(CVI_U32 gpio_pin)
{
	FILE *fp;
	char file_full_path[64];

	memset(file_full_path, 0, sizeof(file_full_path));
	sprintf(file_full_path, "%s%s%d%s", gpio_path, "gpio", gpio_pin, "/direction");

	fp = fopen(file_full_path, "r+");
	if (fp == CVI_NULL) {
		ISP_LOG_ERR("fopen %s failed\n", file_full_path);
		return 1;
	}
	char gpio_dir_val[8] = { 0 };

	fgets(gpio_dir_val, 8, fp);
	if (strncmp(gpio_dir_val, "out", 3) != 0) {
		fprintf(fp, "%s", "out");
	}
	fclose(fp);
	return 0;
}

static CVI_S32 _gpio_operate_value(CVI_U32 gpio_pin, enum GPIO_OP_TYPE op, CVI_U32 *value)
{
	FILE *fp;
	char file_full_path[64];

	memset(file_full_path, 0, sizeof(file_full_path));
	sprintf(file_full_path, "%s%s%d%s", gpio_path, "gpio", gpio_pin, "/value");

	if (op == GPIO_OP_GET) {
		fp = fopen(file_full_path, "rb+");
		if (fp == CVI_NULL) {
			ISP_LOG_ERR("fopen %s failed\n", file_full_path);
			return 1;
		}

		char gpio_val[8] = { 0 };

		fgets(gpio_val, 8, fp);
		*value = atoi(gpio_val);
	} else {
		fp = fopen(file_full_path, "w");
		if (fp == CVI_NULL) {
			ISP_LOG_ERR("fopen %s failed\n", file_full_path);
			return 1;
		}

		fprintf(fp, "%d", *value);
	}
	fclose(fp);
	return 0;
}
