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

#include "sc230ai_2L_slave_cmos_ex.h"
#include <unistd.h>

#define SC230AI_2L_SLAVE_CHIP_ID_ADDR_H 0x3107
#define SC230AI_2L_SLAVE_CHIP_ID_ADDR_L 0x3108
#define SC230AI_2L_SLAVE_CHIP_ID 0xcb34

CVI_U8 sc230ai_2l_slave_i2c_addr = 0x32; //0X30
const CVI_U32 sc230ai_2l_slave_addr_byte = 2;
const CVI_U32 sc230ai_2l_slave_data_byte = 1;
static int g_fd[VI_MAX_PIPE_NUM] = {[0 ... (VI_MAX_PIPE_NUM - 1)] = -1};

static void sc230ai_2l_slave_linear_1080p30_init(VI_PIPE ViPipe);

CVI_U8 sc230ai_2l_slave_i2c_addr_list[] = {SC230AI_2L_SLAVE_I2C_ADDR_1, SC230AI_2L_SLAVE_I2C_ADDR_2};
CVI_S8 sc230ai_2l_slave_i2c_dev_list[] = {1, 1};
CVI_U8 sc230ai_2l_slave_cur_idx = 0;

int sc230ai_2l_slave_i2c_init(VI_PIPE ViPipe)
{
	char acDevFile[16] = {0};
	CVI_U8 u8DevNum;

	if (g_fd[ViPipe] >= 0)
		return CVI_SUCCESS;
	int ret;

	u8DevNum = g_aunSc230ai_2L_SLAVE_BusInfo[ViPipe].s8I2cDev;
	snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);

	g_fd[ViPipe] = open(acDevFile, O_RDWR, 0600);

	if (g_fd[ViPipe] < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Open /dev/cvi_i2c_drv-%u error!\n", u8DevNum);
		return CVI_FAILURE;
	}

	ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, sc230ai_2l_slave_i2c_addr);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_SLAVE_FORCE error!\n");
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return ret;
	}

	return CVI_SUCCESS;
}

int sc230ai_2l_slave_i2c_exit(VI_PIPE ViPipe)
{
	if (g_fd[ViPipe] >= 0) {
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return CVI_SUCCESS;
	}
	return CVI_FAILURE;
}

int sc230ai_2l_slave_read_register(VI_PIPE ViPipe, int addr)
{
	int ret, data;
	CVI_U8 buf[8];
	CVI_U8 idx = 0;

	if (g_fd[ViPipe] < 0)
		return CVI_FAILURE;

	if (sc230ai_2l_slave_addr_byte == 2)
		buf[idx++] = (addr >> 8) & 0xff;

	// add address byte 0
	buf[idx++] = addr & 0xff;

	ret = write(g_fd[ViPipe], buf, sc230ai_2l_slave_addr_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_WRITE error!\n");
		return ret;
	}

	buf[0] = 0;
	buf[1] = 0;
	ret = read(g_fd[ViPipe], buf, sc230ai_2l_slave_data_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_READ error!\n");
		return ret;
	}

	// pack read back data
	data = 0;
	if (sc230ai_2l_slave_data_byte == 2) {
		data = buf[0] << 8;
		data += buf[1];
	} else {
		data = buf[0];
	}

	syslog(LOG_DEBUG, "i2c r 0x%x = 0x%x\n", addr, data);
	return data;
}

int sc230ai_2l_slave_write_register(VI_PIPE ViPipe, int addr, int data)
{
	CVI_U8 idx = 0;
	int ret;
	CVI_U8 buf[8];

	if (g_fd[ViPipe] < 0)
		return CVI_SUCCESS;

	if (sc230ai_2l_slave_addr_byte == 2) {
		buf[idx] = (addr >> 8) & 0xff;
		idx++;
		buf[idx] = addr & 0xff;
		idx++;
	}

	if (sc230ai_2l_slave_data_byte == 1) {
		buf[idx] = data & 0xff;
		idx++;
	}

	ret = write(g_fd[ViPipe], buf, sc230ai_2l_slave_addr_byte + sc230ai_2l_slave_data_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_WRITE error!\n");
		return CVI_FAILURE;
	}
	syslog(LOG_DEBUG, "i2c w 0x%x 0x%x\n", addr, data);
	return CVI_SUCCESS;}

void sc230ai_2l_slave_standby(VI_PIPE ViPipe)
{
	sc230ai_2l_slave_write_register(ViPipe, 0x3019, 0xff);
	sc230ai_2l_slave_write_register(ViPipe, 0x0100, 0x00);

	printf("%s i2c_addr:%x, i2c_dev:%d\n", __func__, sc230ai_2l_slave_i2c_addr,
		   g_aunSc230ai_2L_SLAVE_BusInfo[ViPipe].s8I2cDev);
}

void sc230ai_2l_slave_restart(VI_PIPE ViPipe)
{
	sc230ai_2l_slave_write_register(ViPipe, 0x3019, 0xfe);
	sc230ai_2l_slave_write_register(ViPipe, 0x0100, 0x01);

	printf("%s i2c_addr:%x, i2c_dev:%d\n", __func__, sc230ai_2l_slave_i2c_addr,
		   g_aunSc230ai_2L_SLAVE_BusInfo[ViPipe].s8I2cDev);
}

void sc230ai_2l_slave_default_reg_init(VI_PIPE ViPipe)
{
	CVI_U32 i;

	for (i = 0; i < g_pastSc230ai_2L_SLAVE[ViPipe]->astSyncInfo[0].snsCfg.u32RegNum; i++) {
		sc230ai_2l_slave_write_register(
			ViPipe, g_pastSc230ai_2L_SLAVE[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32RegAddr,
			g_pastSc230ai_2L_SLAVE[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32Data);
	}
}

void sc230ai_2l_slave_mirror_flip(VI_PIPE ViPipe, ISP_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip)
{
	CVI_U8 val = 0;

	switch (eSnsMirrorFlip) {
	case ISP_SNS_NORMAL:
		break;
	case ISP_SNS_MIRROR:
		val |= 0x6;
		break;
	case ISP_SNS_FLIP:
		val |= 0x60;
		break;
	case ISP_SNS_MIRROR_FLIP:
		val |= 0x66;
		break;
	default:
		return;
	}

	sc230ai_2l_slave_write_register(ViPipe, 0x3221, val);
}

int sc230ai_2l_slave_probe(VI_PIPE ViPipe)
{
	int nVal;
	int nVal2;

	if (sc230ai_2l_slave_i2c_init(ViPipe) != CVI_SUCCESS)
		return CVI_FAILURE;

	nVal = sc230ai_2l_slave_read_register(ViPipe, SC230AI_2L_SLAVE_CHIP_ID_ADDR_H);
	nVal2 = sc230ai_2l_slave_read_register(ViPipe, SC230AI_2L_SLAVE_CHIP_ID_ADDR_L);
	if (nVal < 0 || nVal2 < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "read sensor id error.\n");
		return nVal;
	}

	if ((((nVal & 0xFF) << 8) | (nVal2 & 0xFF)) != SC230AI_2L_SLAVE_CHIP_ID) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Sensor ID Mismatch! Use the wrong sensor??\n");
		CVI_TRACE_SNS(CVI_DBG_ERR, "nVal:%#x, nVal2:%#x\n", nVal, nVal2);
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

void sc230ai_2l_slave_init(VI_PIPE ViPipe)
{
	sc230ai_2l_slave_linear_1080p30_init(ViPipe);
	g_pastSc230ai_2L_SLAVE[ViPipe]->bInit = CVI_TRUE;
}

void sc230ai_2l_slave_exit(VI_PIPE ViPipe)
{
	sc230ai_2l_slave_i2c_exit(ViPipe);
}

// cleaned_0x01_FT_SC230AI_MIPI_27Minput_2l_slaveane_371.25Mbps_10bit_1920x1080_30fps
static void sc230ai_2l_slave_linear_1080p30_init(VI_PIPE ViPipe)
{
	sc230ai_2l_slave_write_register(ViPipe, 0x0103, 0x01);
	sc230ai_2l_slave_write_register(ViPipe, 0x0100, 0x00);
	sc230ai_2l_slave_write_register(ViPipe, 0x36e9, 0x80);
	sc230ai_2l_slave_write_register(ViPipe, 0x37f9, 0x80);
	sc230ai_2l_slave_write_register(ViPipe, 0x301f, 0x01);

	// //slave fsync
	sc230ai_2l_slave_write_register(ViPipe, 0x3222, 0x01);//bit[0]:Slave mode en
	sc230ai_2l_slave_write_register(ViPipe, 0x3224, 0x92);//slave pin：fsync 上升沿触发; bit[0]:0上升沿 1下降沿

	//slave efsync
	// sc230ai_2l_slave_write_register(ViPipe, 0x3224, 0x82);//slave pin：efsync
	// sc230ai_2l_slave_write_register(ViPipe, 0x3614, 0x01);//efsync作为触发脚时，对应更改

	sc230ai_2l_slave_write_register(ViPipe, 0x3301, 0x07);
	sc230ai_2l_slave_write_register(ViPipe, 0x3304, 0x50);
	sc230ai_2l_slave_write_register(ViPipe, 0x3306, 0x70);
	sc230ai_2l_slave_write_register(ViPipe, 0x3308, 0x18);
	sc230ai_2l_slave_write_register(ViPipe, 0x3309, 0x68);
	sc230ai_2l_slave_write_register(ViPipe, 0x330a, 0x01);
	sc230ai_2l_slave_write_register(ViPipe, 0x330b, 0x20);
	sc230ai_2l_slave_write_register(ViPipe, 0x3314, 0x15);
	sc230ai_2l_slave_write_register(ViPipe, 0x331e, 0x41);
	sc230ai_2l_slave_write_register(ViPipe, 0x331f, 0x59);
	sc230ai_2l_slave_write_register(ViPipe, 0x3333, 0x10);
	sc230ai_2l_slave_write_register(ViPipe, 0x3334, 0x40);
	sc230ai_2l_slave_write_register(ViPipe, 0x335d, 0x60);
	sc230ai_2l_slave_write_register(ViPipe, 0x335e, 0x06);
	sc230ai_2l_slave_write_register(ViPipe, 0x335f, 0x08);
	sc230ai_2l_slave_write_register(ViPipe, 0x3364, 0x5e);
	sc230ai_2l_slave_write_register(ViPipe, 0x337c, 0x02);
	sc230ai_2l_slave_write_register(ViPipe, 0x337d, 0x0a);
	sc230ai_2l_slave_write_register(ViPipe, 0x3390, 0x01);
	sc230ai_2l_slave_write_register(ViPipe, 0x3391, 0x0b);
	sc230ai_2l_slave_write_register(ViPipe, 0x3392, 0x0f);
	sc230ai_2l_slave_write_register(ViPipe, 0x3393, 0x09);
	sc230ai_2l_slave_write_register(ViPipe, 0x3394, 0x0d);
	sc230ai_2l_slave_write_register(ViPipe, 0x3395, 0x60);
	sc230ai_2l_slave_write_register(ViPipe, 0x3396, 0x48);
	sc230ai_2l_slave_write_register(ViPipe, 0x3397, 0x49);
	sc230ai_2l_slave_write_register(ViPipe, 0x3398, 0x4b);
	sc230ai_2l_slave_write_register(ViPipe, 0x3399, 0x06);
	sc230ai_2l_slave_write_register(ViPipe, 0x339a, 0x0a);
	sc230ai_2l_slave_write_register(ViPipe, 0x339b, 0x0d);
	sc230ai_2l_slave_write_register(ViPipe, 0x339c, 0x60);
	sc230ai_2l_slave_write_register(ViPipe, 0x33a2, 0x04);
	sc230ai_2l_slave_write_register(ViPipe, 0x33ad, 0x2c);
	sc230ai_2l_slave_write_register(ViPipe, 0x33af, 0x40);
	sc230ai_2l_slave_write_register(ViPipe, 0x33b1, 0x80);
	sc230ai_2l_slave_write_register(ViPipe, 0x33b3, 0x40);
	sc230ai_2l_slave_write_register(ViPipe, 0x33b9, 0x0a);
	sc230ai_2l_slave_write_register(ViPipe, 0x33f9, 0x78);
	sc230ai_2l_slave_write_register(ViPipe, 0x33fb, 0xa0);
	sc230ai_2l_slave_write_register(ViPipe, 0x33fc, 0x4f);
	sc230ai_2l_slave_write_register(ViPipe, 0x33fd, 0x5f);
	sc230ai_2l_slave_write_register(ViPipe, 0x349f, 0x03);
	sc230ai_2l_slave_write_register(ViPipe, 0x34a6, 0x4b);
	sc230ai_2l_slave_write_register(ViPipe, 0x34a7, 0x5f);
	sc230ai_2l_slave_write_register(ViPipe, 0x34a8, 0x30);
	sc230ai_2l_slave_write_register(ViPipe, 0x34a9, 0x20);
	sc230ai_2l_slave_write_register(ViPipe, 0x34aa, 0x01);
	sc230ai_2l_slave_write_register(ViPipe, 0x34ab, 0x28);
	sc230ai_2l_slave_write_register(ViPipe, 0x34ac, 0x01);
	sc230ai_2l_slave_write_register(ViPipe, 0x34ad, 0x58);
	sc230ai_2l_slave_write_register(ViPipe, 0x34f8, 0x7f);
	sc230ai_2l_slave_write_register(ViPipe, 0x34f9, 0x10);
	sc230ai_2l_slave_write_register(ViPipe, 0x3630, 0xc0);
	sc230ai_2l_slave_write_register(ViPipe, 0x3632, 0x54);
	sc230ai_2l_slave_write_register(ViPipe, 0x3633, 0x44);
	sc230ai_2l_slave_write_register(ViPipe, 0x363b, 0x20);
	sc230ai_2l_slave_write_register(ViPipe, 0x363c, 0x08);
	sc230ai_2l_slave_write_register(ViPipe, 0x3670, 0x09);
	sc230ai_2l_slave_write_register(ViPipe, 0x3674, 0xb0);
	sc230ai_2l_slave_write_register(ViPipe, 0x3675, 0x80);
	sc230ai_2l_slave_write_register(ViPipe, 0x3676, 0x88);
	sc230ai_2l_slave_write_register(ViPipe, 0x367c, 0x40);
	sc230ai_2l_slave_write_register(ViPipe, 0x367d, 0x49);
	sc230ai_2l_slave_write_register(ViPipe, 0x3690, 0x33);
	sc230ai_2l_slave_write_register(ViPipe, 0x3691, 0x33);
	sc230ai_2l_slave_write_register(ViPipe, 0x3692, 0x43);
	sc230ai_2l_slave_write_register(ViPipe, 0x369c, 0x49);
	sc230ai_2l_slave_write_register(ViPipe, 0x369d, 0x4f);
	sc230ai_2l_slave_write_register(ViPipe, 0x36ae, 0x4b);
	sc230ai_2l_slave_write_register(ViPipe, 0x36af, 0x4f);
	sc230ai_2l_slave_write_register(ViPipe, 0x36b0, 0x87);
	sc230ai_2l_slave_write_register(ViPipe, 0x36b1, 0x9b);
	sc230ai_2l_slave_write_register(ViPipe, 0x36b2, 0xb7);
	sc230ai_2l_slave_write_register(ViPipe, 0x36d0, 0x01);
	sc230ai_2l_slave_write_register(ViPipe, 0x3722, 0x97);
	sc230ai_2l_slave_write_register(ViPipe, 0x3724, 0x22);
	sc230ai_2l_slave_write_register(ViPipe, 0x3728, 0x90);
	sc230ai_2l_slave_write_register(ViPipe, 0x3901, 0x02);
	sc230ai_2l_slave_write_register(ViPipe, 0x3902, 0xc5);
	sc230ai_2l_slave_write_register(ViPipe, 0x3904, 0x04);
	sc230ai_2l_slave_write_register(ViPipe, 0x3907, 0x00);
	sc230ai_2l_slave_write_register(ViPipe, 0x3908, 0x41);
	sc230ai_2l_slave_write_register(ViPipe, 0x3909, 0x00);
	sc230ai_2l_slave_write_register(ViPipe, 0x390a, 0x00);
	sc230ai_2l_slave_write_register(ViPipe, 0x3933, 0x84);
	sc230ai_2l_slave_write_register(ViPipe, 0x3934, 0x0a);
	sc230ai_2l_slave_write_register(ViPipe, 0x3940, 0x64);
	sc230ai_2l_slave_write_register(ViPipe, 0x3941, 0x00);
	sc230ai_2l_slave_write_register(ViPipe, 0x3942, 0x04);
	sc230ai_2l_slave_write_register(ViPipe, 0x3943, 0x0b);
	sc230ai_2l_slave_write_register(ViPipe, 0x3e00, 0x00);
	sc230ai_2l_slave_write_register(ViPipe, 0x3e01, 0x8c);
	sc230ai_2l_slave_write_register(ViPipe, 0x3e02, 0x10);
	sc230ai_2l_slave_write_register(ViPipe, 0x440e, 0x02);
	sc230ai_2l_slave_write_register(ViPipe, 0x450d, 0x11);
	sc230ai_2l_slave_write_register(ViPipe, 0x5010, 0x01);
	sc230ai_2l_slave_write_register(ViPipe, 0x5787, 0x08);
	sc230ai_2l_slave_write_register(ViPipe, 0x5788, 0x03);
	sc230ai_2l_slave_write_register(ViPipe, 0x5789, 0x00);
	sc230ai_2l_slave_write_register(ViPipe, 0x578a, 0x10);
	sc230ai_2l_slave_write_register(ViPipe, 0x578b, 0x08);
	sc230ai_2l_slave_write_register(ViPipe, 0x578c, 0x00);
	sc230ai_2l_slave_write_register(ViPipe, 0x5790, 0x08);
	sc230ai_2l_slave_write_register(ViPipe, 0x5791, 0x04);
	sc230ai_2l_slave_write_register(ViPipe, 0x5792, 0x00);
	sc230ai_2l_slave_write_register(ViPipe, 0x5793, 0x10);
	sc230ai_2l_slave_write_register(ViPipe, 0x5794, 0x08);
	sc230ai_2l_slave_write_register(ViPipe, 0x5795, 0x00);
	sc230ai_2l_slave_write_register(ViPipe, 0x5799, 0x06);
	sc230ai_2l_slave_write_register(ViPipe, 0x57ad, 0x00);
	sc230ai_2l_slave_write_register(ViPipe, 0x5ae0, 0xfe);
	sc230ai_2l_slave_write_register(ViPipe, 0x5ae1, 0x40);
	sc230ai_2l_slave_write_register(ViPipe, 0x5ae2, 0x3f);
	sc230ai_2l_slave_write_register(ViPipe, 0x5ae3, 0x38);
	sc230ai_2l_slave_write_register(ViPipe, 0x5ae4, 0x28);
	sc230ai_2l_slave_write_register(ViPipe, 0x5ae5, 0x3f);
	sc230ai_2l_slave_write_register(ViPipe, 0x5ae6, 0x38);
	sc230ai_2l_slave_write_register(ViPipe, 0x5ae7, 0x28);
	sc230ai_2l_slave_write_register(ViPipe, 0x5ae8, 0x3f);
	sc230ai_2l_slave_write_register(ViPipe, 0x5ae9, 0x3c);
	sc230ai_2l_slave_write_register(ViPipe, 0x5aea, 0x2c);
	sc230ai_2l_slave_write_register(ViPipe, 0x5aeb, 0x3f);
	sc230ai_2l_slave_write_register(ViPipe, 0x5aec, 0x3c);
	sc230ai_2l_slave_write_register(ViPipe, 0x5aed, 0x2c);
	sc230ai_2l_slave_write_register(ViPipe, 0x5af4, 0x3f);
	sc230ai_2l_slave_write_register(ViPipe, 0x5af5, 0x38);
	sc230ai_2l_slave_write_register(ViPipe, 0x5af6, 0x28);
	sc230ai_2l_slave_write_register(ViPipe, 0x5af7, 0x3f);
	sc230ai_2l_slave_write_register(ViPipe, 0x5af8, 0x38);
	sc230ai_2l_slave_write_register(ViPipe, 0x5af9, 0x28);
	sc230ai_2l_slave_write_register(ViPipe, 0x5afa, 0x3f);
	sc230ai_2l_slave_write_register(ViPipe, 0x5afb, 0x3c);
	sc230ai_2l_slave_write_register(ViPipe, 0x5afc, 0x2c);
	sc230ai_2l_slave_write_register(ViPipe, 0x5afd, 0x3f);
	sc230ai_2l_slave_write_register(ViPipe, 0x5afe, 0x3c);
	sc230ai_2l_slave_write_register(ViPipe, 0x5aff, 0x2c);
	sc230ai_2l_slave_write_register(ViPipe, 0x36e9, 0x20);
	sc230ai_2l_slave_write_register(ViPipe, 0x37f9, 0x27);
	sc230ai_2l_slave_write_register(ViPipe, 0x0100, 0x01);

	sc230ai_2l_slave_default_reg_init(ViPipe);

	printf("ViPipe:%d,===SC230AI_2L_SLAVE 1080P 30fps 10bit LINEAR Init OK!===\n", ViPipe);
}