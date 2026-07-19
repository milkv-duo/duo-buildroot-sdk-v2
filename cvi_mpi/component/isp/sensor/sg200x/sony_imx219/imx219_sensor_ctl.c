#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include <linux/cvi_comm_video.h>
#include "cvi_sns_ctrl.h"
#include "imx219_cmos_ex.h"

static void imx219_linear_1080p30_init(VI_PIPE ViPipe);
static void imx219_linear_2560x1920p27_init(VI_PIPE ViPipe);

CVI_U8 imx219_i2c_addr = 0x10;        /* I2C Address of IMX219 */
const CVI_U32 imx219_addr_byte = 2;
const CVI_U32 imx219_data_byte = 1;
static int g_fd[VI_MAX_PIPE_NUM] = {[0 ... (VI_MAX_PIPE_NUM - 1)] = -1};

int imx219_i2c_init(VI_PIPE ViPipe)
{

	char acDevFile[16] = {0};
	CVI_U8 u8DevNum;

	if (g_fd[ViPipe] >= 0)
		return CVI_SUCCESS;
	int ret;

	u8DevNum = g_aunImx219_BusInfo[ViPipe].s8I2cDev;
	snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);

	g_fd[ViPipe] = open(acDevFile, O_RDWR, 0600);

	if (g_fd[ViPipe] < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Open /dev/i2c-%u error!\n", u8DevNum);
		return CVI_FAILURE;
	}

	ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, imx219_i2c_addr);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_SLAVE_FORCE error!\n");
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return ret;
	}
	return CVI_SUCCESS;
}

int imx219_i2c_exit(VI_PIPE ViPipe)
{
	if (g_fd[ViPipe] >= 0) {
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return CVI_SUCCESS;
	}
	return CVI_FAILURE;
}

int imx219_read_register(VI_PIPE ViPipe, int addr)
{
	int ret, data;
	CVI_U8 buf[8];
	CVI_U8 idx = 0;

	if (g_fd[ViPipe] < 0)
		return CVI_FAILURE;

	if (imx219_addr_byte == 2)
		buf[idx++] = (addr >> 8) & 0xff;

	// add address byte 0
	buf[idx++] = addr & 0xff;

	ret = write(g_fd[ViPipe], buf, imx219_addr_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_READ error!\n");
		return 0;
	}

	buf[0] = 0;
	buf[1] = 0;
	ret = read(g_fd[ViPipe], buf, imx219_data_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_READ error!\n");
		return 0;
	}

	// pack read back data
	data = 0;
	if (imx219_data_byte == 2) {
		data = buf[0] << 8;
		data += buf[1];
	} else {
		data = buf[0];
	}

	syslog(LOG_DEBUG, "i2c r 0x%x = 0x%x\n", addr, data);

	return data;
}

int imx219_write_register(VI_PIPE ViPipe, int addr, int data)
{
	CVI_U8 idx = 0;
	int ret;
	CVI_U8 buf[8];

	if (g_fd[ViPipe] < 0)
		return CVI_SUCCESS;

	if (imx219_addr_byte == 2) {
		buf[idx] = (addr >> 8) & 0xff;
		idx++;
		buf[idx] = addr & 0xff;
		idx++;
	}

	if (imx219_data_byte == 1) {
		buf[idx] = data & 0xff;
		idx++;
	}

	ret = write(g_fd[ViPipe], buf, imx219_addr_byte + imx219_data_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_WRITE error!\n");
		return CVI_FAILURE;
	}
	syslog(LOG_DEBUG, "i2c w 0x%x 0x%x\n", addr, data);
	return CVI_SUCCESS;
}

static void delay_ms(int ms)
{
	usleep(ms * 1000);
}

void imx219_standby(VI_PIPE ViPipe)
{
	imx219_write_register(ViPipe, 0x0100, 0x00); /* STANDBY */
}

void imx219_restart(VI_PIPE ViPipe)
{
	imx219_write_register(ViPipe, 0x0100, 0x00);
	delay_ms(20);
	imx219_write_register(ViPipe, 0x0100, 0x01);
}

void imx219_default_reg_init(VI_PIPE ViPipe)
{
	CVI_U32 i;

	for (i = 0; i < g_pastImx219[ViPipe]->astSyncInfo[0].snsCfg.u32RegNum; i++) {
		imx219_write_register(ViPipe,
				g_pastImx219[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32RegAddr,
				g_pastImx219[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32Data);
	}
}

#define IMX219_ORIENTATION	0x0172
#define IMX219_ORIENTATION_BASE	0x3
void imx219_mirror_flip(VI_PIPE ViPipe, ISP_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip)
{
	CVI_U8 orientation = IMX219_ORIENTATION_BASE;

	switch (eSnsMirrorFlip) {
	case ISP_SNS_NORMAL:
		break;
	case ISP_SNS_MIRROR:
		orientation ^= 0x1;
		break;
	case ISP_SNS_FLIP:
		orientation ^= 0x2;
		break;
	case ISP_SNS_MIRROR_FLIP:
		orientation ^= 0x3;
		break;
	default:
		return;
	}

	imx219_write_register(ViPipe, IMX219_ORIENTATION, orientation);
}

#define IMX219_CHIP_ID_ADDR_H		0x0000
#define IMX219_CHIP_ID_ADDR_L		0x0001
#define IMX219_CHIP_ID			0x0219

int imx219_probe(VI_PIPE ViPipe)
{
	int nVal, nVal2;

	usleep(1000);
	if (imx219_i2c_init(ViPipe) != CVI_SUCCESS)
		return CVI_FAILURE;

	nVal  = imx219_read_register(ViPipe, IMX219_CHIP_ID_ADDR_H);
	nVal2 = imx219_read_register(ViPipe, IMX219_CHIP_ID_ADDR_L);
	if (nVal < 0 || nVal2 < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "read sensor id error.\n");
		return nVal;
	}

	if ((((nVal & 0xFF) << 8) | (nVal2 & 0xFF)) != IMX219_CHIP_ID) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Sensor ID Mismatch! Use the wrong sensor??\n");
		return CVI_FAILURE;
	}
	return CVI_SUCCESS;
}

void imx219_init(VI_PIPE ViPipe)
{
	imx219_i2c_init(ViPipe);

	delay_ms(10);

	switch (g_pastImx219[ViPipe]->u8ImgMode) {
	case IMX219_MODE_2560X1920P27:
		imx219_linear_2560x1920p27_init(ViPipe);
		break;
	case IMX219_MODE_1920X1080P30:
	default:
		imx219_linear_1080p30_init(ViPipe);
		break;
	}

	g_pastImx219[ViPipe]->bInit = CVI_TRUE;
}

void imx219_exit(VI_PIPE ViPipe)
{
	imx219_i2c_exit(ViPipe);
}

/* 1080P30 */
static void imx219_linear_1080p30_init(VI_PIPE ViPipe)
{
	imx219_write_register(ViPipe, 0x0100, 0x00);
	imx219_write_register(ViPipe, 0x30eb, 0x05);
	imx219_write_register(ViPipe, 0x30eb, 0x0c);
	imx219_write_register(ViPipe, 0x300a, 0xff);
	imx219_write_register(ViPipe, 0x300b, 0xff);
	imx219_write_register(ViPipe, 0x30eb, 0x05);
	imx219_write_register(ViPipe, 0x30eb, 0x09);
	imx219_write_register(ViPipe, 0x0114, 0x01);
	imx219_write_register(ViPipe, 0x0128, 0x00);
	imx219_write_register(ViPipe, 0x012a, 0x18);
	imx219_write_register(ViPipe, 0x012b, 0x00);
	imx219_write_register(ViPipe, 0x0160, 0x06);
	imx219_write_register(ViPipe, 0x0161, 0xe3);
	imx219_write_register(ViPipe, 0x0162, 0x0d);
	imx219_write_register(ViPipe, 0x0163, 0x78);
	imx219_write_register(ViPipe, 0x0164, 0x02);
	imx219_write_register(ViPipe, 0x0165, 0xa8);
	imx219_write_register(ViPipe, 0x0166, 0x0a);
	imx219_write_register(ViPipe, 0x0167, 0x27);
	imx219_write_register(ViPipe, 0x0168, 0x02);
	imx219_write_register(ViPipe, 0x0169, 0xb4);
	imx219_write_register(ViPipe, 0x016a, 0x06);
	imx219_write_register(ViPipe, 0x016b, 0xeb);
	imx219_write_register(ViPipe, 0x016c, 0x07);
	imx219_write_register(ViPipe, 0x016d, 0x80);
	imx219_write_register(ViPipe, 0x016e, 0x04);
	imx219_write_register(ViPipe, 0x016f, 0x38);
	imx219_write_register(ViPipe, 0x0170, 0x01);
	imx219_write_register(ViPipe, 0x0171, 0x01);
	imx219_write_register(ViPipe, 0x0172, 0x03); /* ORIENTATION, module is mounted 180 deg */
	imx219_write_register(ViPipe, 0x0174, 0x00);
	imx219_write_register(ViPipe, 0x0175, 0x00);
	imx219_write_register(ViPipe, 0x018c, 0x0a);
	imx219_write_register(ViPipe, 0x018d, 0x0a);
	imx219_write_register(ViPipe, 0x0301, 0x05);
	imx219_write_register(ViPipe, 0x0303, 0x01);
	imx219_write_register(ViPipe, 0x0304, 0x03);
	imx219_write_register(ViPipe, 0x0305, 0x03);
	imx219_write_register(ViPipe, 0x0306, 0x00);
	imx219_write_register(ViPipe, 0x0307, 0x39);
	imx219_write_register(ViPipe, 0x0309, 0x0a);
	imx219_write_register(ViPipe, 0x030b, 0x01);
	imx219_write_register(ViPipe, 0x030c, 0x00);
	imx219_write_register(ViPipe, 0x030d, 0x72);
	imx219_write_register(ViPipe, 0x0624, 0x07);
	imx219_write_register(ViPipe, 0x0625, 0x80);
	imx219_write_register(ViPipe, 0x0626, 0x04);
	imx219_write_register(ViPipe, 0x0627, 0x38);
	imx219_write_register(ViPipe, 0x455e, 0x00);
	imx219_write_register(ViPipe, 0x471e, 0x4b);
	imx219_write_register(ViPipe, 0x4767, 0x0f);
	imx219_write_register(ViPipe, 0x4750, 0x14);
	imx219_write_register(ViPipe, 0x4540, 0x00);
	imx219_write_register(ViPipe, 0x47b4, 0x14);
	imx219_write_register(ViPipe, 0x4713, 0x30);
	imx219_write_register(ViPipe, 0x478b, 0x10);
	imx219_write_register(ViPipe, 0x478f, 0x10);
	imx219_write_register(ViPipe, 0x4793, 0x10);
	imx219_write_register(ViPipe, 0x4797, 0x0e);
	imx219_write_register(ViPipe, 0x479b, 0x0e);

	imx219_default_reg_init(ViPipe);

	imx219_write_register(ViPipe, 0x0100, 0x01); /* streaming */

	delay_ms(100);

	printf("ViPipe:%d,===IMX219 1080P 30fps 10bit LINE Init OK!\n", ViPipe);
}

/* 2560x1920P27 (5MP, centered crop of the full array, no binning; 64-aligned width) */
static void imx219_linear_2560x1920p27_init(VI_PIPE ViPipe)
{
	imx219_write_register(ViPipe, 0x0100, 0x00);
	imx219_write_register(ViPipe, 0x30eb, 0x05);
	imx219_write_register(ViPipe, 0x30eb, 0x0c);
	imx219_write_register(ViPipe, 0x300a, 0xff);
	imx219_write_register(ViPipe, 0x300b, 0xff);
	imx219_write_register(ViPipe, 0x30eb, 0x05);
	imx219_write_register(ViPipe, 0x30eb, 0x09);
	imx219_write_register(ViPipe, 0x0114, 0x01);
	imx219_write_register(ViPipe, 0x0128, 0x00);
	imx219_write_register(ViPipe, 0x012a, 0x18);
	imx219_write_register(ViPipe, 0x012b, 0x00);
	imx219_write_register(ViPipe, 0x0160, 0x07);
	imx219_write_register(ViPipe, 0x0161, 0xa7);
	imx219_write_register(ViPipe, 0x0162, 0x0d);
	imx219_write_register(ViPipe, 0x0163, 0x78);
	imx219_write_register(ViPipe, 0x0164, 0x01);
	imx219_write_register(ViPipe, 0x0165, 0x68);
	imx219_write_register(ViPipe, 0x0166, 0x0b);
	imx219_write_register(ViPipe, 0x0167, 0x67);
	imx219_write_register(ViPipe, 0x0168, 0x01);
	imx219_write_register(ViPipe, 0x0169, 0x10);
	imx219_write_register(ViPipe, 0x016a, 0x08);
	imx219_write_register(ViPipe, 0x016b, 0x8f);
	imx219_write_register(ViPipe, 0x016c, 0x0a);
	imx219_write_register(ViPipe, 0x016d, 0x00);
	imx219_write_register(ViPipe, 0x016e, 0x07);
	imx219_write_register(ViPipe, 0x016f, 0x80);
	imx219_write_register(ViPipe, 0x0170, 0x01);
	imx219_write_register(ViPipe, 0x0171, 0x01);
	imx219_write_register(ViPipe, 0x0172, 0x03); /* ORIENTATION, module is mounted 180 deg */
	imx219_write_register(ViPipe, 0x0174, 0x00);
	imx219_write_register(ViPipe, 0x0175, 0x00);
	imx219_write_register(ViPipe, 0x018c, 0x0a);
	imx219_write_register(ViPipe, 0x018d, 0x0a);
	imx219_write_register(ViPipe, 0x0301, 0x05);
	imx219_write_register(ViPipe, 0x0303, 0x01);
	imx219_write_register(ViPipe, 0x0304, 0x03);
	imx219_write_register(ViPipe, 0x0305, 0x03);
	imx219_write_register(ViPipe, 0x0306, 0x00);
	imx219_write_register(ViPipe, 0x0307, 0x39);
	imx219_write_register(ViPipe, 0x0309, 0x0a);
	imx219_write_register(ViPipe, 0x030b, 0x01);
	imx219_write_register(ViPipe, 0x030c, 0x00);
	imx219_write_register(ViPipe, 0x030d, 0x72);
	imx219_write_register(ViPipe, 0x0624, 0x0a);
	imx219_write_register(ViPipe, 0x0625, 0x00);
	imx219_write_register(ViPipe, 0x0626, 0x07);
	imx219_write_register(ViPipe, 0x0627, 0x80);
	imx219_write_register(ViPipe, 0x455e, 0x00);
	imx219_write_register(ViPipe, 0x471e, 0x4b);
	imx219_write_register(ViPipe, 0x4767, 0x0f);
	imx219_write_register(ViPipe, 0x4750, 0x14);
	imx219_write_register(ViPipe, 0x4540, 0x00);
	imx219_write_register(ViPipe, 0x47b4, 0x14);
	imx219_write_register(ViPipe, 0x4713, 0x30);
	imx219_write_register(ViPipe, 0x478b, 0x10);
	imx219_write_register(ViPipe, 0x478f, 0x10);
	imx219_write_register(ViPipe, 0x4793, 0x10);
	imx219_write_register(ViPipe, 0x4797, 0x0e);
	imx219_write_register(ViPipe, 0x479b, 0x0e);

	imx219_default_reg_init(ViPipe);

	imx219_write_register(ViPipe, 0x0100, 0x01); /* streaming */

	delay_ms(100);

	printf("ViPipe:%d,===IMX219 2560x1920 27fps 10bit LINE Init OK!\n", ViPipe);
}

