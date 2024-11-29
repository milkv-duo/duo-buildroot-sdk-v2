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
#include "os02n10_1l_cmos_ex.h"

static void os02n10_1l_linear_1080p15_init(VI_PIPE ViPipe);

CVI_U8 os02n10_1l_i2c_addr = 0x3c;        /* I2C Address of OS02N10_1L */
const CVI_U32 os02n10_1l_addr_byte = 1;
const CVI_U32 os02n10_1l_data_byte = 1;
static int g_fd[VI_MAX_PIPE_NUM] = {[0 ... (VI_MAX_PIPE_NUM - 1)] = -1};
ISP_SNS_MIRRORFLIP_TYPE_E g_aeOs02n10_1l_MirrorFip_Initial[VI_MAX_PIPE_NUM] = {
	ISP_SNS_MIRROR, ISP_SNS_MIRROR, ISP_SNS_MIRROR, ISP_SNS_MIRROR};

int os02n10_1l_i2c_init(VI_PIPE ViPipe)
{
	char acDevFile[16] = {0};
	CVI_U8 u8DevNum;

	if (g_fd[ViPipe] >= 0)
		return CVI_SUCCESS;
	int ret;

	u8DevNum = g_aunOs02n10_1l_BusInfo[ViPipe].s8I2cDev;
	snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);

	g_fd[ViPipe] = open(acDevFile, O_RDWR, 0600);

	if (g_fd[ViPipe] < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Open /dev/i2c-%u error!\n", u8DevNum);
		return CVI_FAILURE;
	}

	ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, os02n10_1l_i2c_addr);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_SLAVE_FORCE error!\n");
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return ret;
	}
	return CVI_SUCCESS;
}

int os02n10_1l_i2c_exit(VI_PIPE ViPipe)
{
	if (g_fd[ViPipe] >= 0) {
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return CVI_SUCCESS;
	}
	return CVI_FAILURE;
}

int os02n10_1l_read_register(VI_PIPE ViPipe, int addr)
{
	int ret, data;
	CVI_U8 buf[8];
	CVI_U8 idx = 0;

	if (g_fd[ViPipe] < 0)
		return CVI_FAILURE;

	if (os02n10_1l_addr_byte == 2)
		buf[idx++] = (addr >> 8) & 0xff;

	// add address byte 0
	buf[idx++] = addr & 0xff;

	ret = write(g_fd[ViPipe], buf, os02n10_1l_addr_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_WRITE error!\n");
		return 0;
	}

	buf[0] = 0;
	buf[1] = 0;
	ret = read(g_fd[ViPipe], buf, os02n10_1l_data_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_READ error!\n");
		return 0;
	}

	// pack read back data
	data = 0;
	if (os02n10_1l_data_byte == 2) {
		data = buf[0] << 8;
		data += buf[1];
	} else {
		data = buf[0];
	}

	syslog(LOG_DEBUG, "i2c r 0x%x = 0x%x\n", addr, data);
	return data;
}

int os02n10_1l_write_register(VI_PIPE ViPipe, int addr, int data)
{
	CVI_U8 idx = 0;
	int ret;
	CVI_U8 buf[8];

	if (g_fd[ViPipe] < 0)
		return CVI_SUCCESS;

	if (os02n10_1l_addr_byte == 2) {
		buf[idx] = (addr >> 8) & 0xff;
		idx++;
	}
	buf[idx] = addr & 0xff;
	idx++;

	if (os02n10_1l_data_byte == 2) {
		buf[idx] = (data >> 8) & 0xff;
		idx++;
	}
	buf[idx] = data & 0xff;
	idx++;

	ret = write(g_fd[ViPipe], buf, os02n10_1l_addr_byte + os02n10_1l_data_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_WRITE error! %#x\n", addr);
		return CVI_FAILURE;
	}

	syslog(LOG_DEBUG, "i2c w 0x%x 0x%x\n", addr, data);
	return CVI_SUCCESS;
}

static void delay_ms(int ms)
{
	usleep(ms * 1000);
}

void os02n10_1l_standby(VI_PIPE ViPipe)
{
	UNUSED(ViPipe);
	printf("os02n10_1l_standby\n");
}

void os02n10_1l_restart(VI_PIPE ViPipe)
{
	UNUSED(ViPipe);
	printf("os02n10_1l_restart\n");
}

void os02n10_1l_default_reg_init(VI_PIPE ViPipe)
{
	CVI_U32 i;

	for (i = 0; i < g_pastOs02n10_1l[ViPipe]->astSyncInfo[0].snsCfg.u32RegNum; i++) {
		os02n10_1l_write_register(ViPipe,
				g_pastOs02n10_1l[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32RegAddr,
				g_pastOs02n10_1l[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32Data);
	}

}

#define OS02N10_1L_CHIP_ID_ADDR_HH		0x02
#define OS02N10_1L_CHIP_ID_ADDR_HL		0x03
#define OS02N10_1L_CHIP_ID_ADDR_LH		0x04
#define OS02N10_1L_CHIP_ID_ADDR_LL		0x05
#define OS02N10_1L_CHIP_ID			0x53024E10

int os02n10_1l_probe(VI_PIPE ViPipe)
{
	int nVal, nVal2, nVal3, nVal4;

	usleep(500);
	if (os02n10_1l_i2c_init(ViPipe) != CVI_SUCCESS)
		return CVI_FAILURE;

	os02n10_1l_write_register(ViPipe, 0xfd, 0x00);
	nVal  = os02n10_1l_read_register(ViPipe, OS02N10_1L_CHIP_ID_ADDR_HH);
	nVal2 = os02n10_1l_read_register(ViPipe, OS02N10_1L_CHIP_ID_ADDR_HL);
	nVal3 = os02n10_1l_read_register(ViPipe, OS02N10_1L_CHIP_ID_ADDR_LH);
	nVal4 = os02n10_1l_read_register(ViPipe, OS02N10_1L_CHIP_ID_ADDR_LL);

	if (nVal < 0 || nVal2 < 0 || nVal3 < 0 || nVal4 < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "read sensor id error.\n");
		return nVal;
	}

	if ((((nVal & 0xFF) << 24) | ((nVal2 & 0xFF) << 16) | (nVal3 & 0xFF) << 8 | (nVal4 & 0xFF)) != OS02N10_1L_CHIP_ID) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Sensor ID Mismatch! Use the wrong sensor??\n");
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

void os02n10_1l_init(VI_PIPE ViPipe)
{
	WDR_MODE_E        enWDRMode;
	CVI_U8            u8ImgMode;

	enWDRMode   = g_pastOs02n10_1l[ViPipe]->enWDRMode;
	u8ImgMode   = g_pastOs02n10_1l[ViPipe]->u8ImgMode;

	os02n10_1l_i2c_init(ViPipe);

	if (enWDRMode == WDR_MODE_NONE) {
		if (u8ImgMode == OS02N10_1L_MODE_1080P15)
			os02n10_1l_linear_1080p15_init(ViPipe);
	}
	g_pastOs02n10_1l[ViPipe]->bInit = CVI_TRUE;
}

void os02n10_1l_exit(VI_PIPE ViPipe)
{
	os02n10_1l_i2c_exit(ViPipe);
}

//MIPI_1LANE_1920x1080_750M_raw10_15fps
static void os02n10_1l_linear_1080p15_init(VI_PIPE ViPipe)
{
	os02n10_1l_write_register(ViPipe, 0xfc, 0x01);
	os02n10_1l_write_register(ViPipe, 0xfd, 0x00);
	os02n10_1l_write_register(ViPipe, 0xba, 0x02);
	os02n10_1l_write_register(ViPipe, 0xfd, 0x00);
	os02n10_1l_write_register(ViPipe, 0xb1, 0x14);
	os02n10_1l_write_register(ViPipe, 0xba, 0x00);
	os02n10_1l_write_register(ViPipe, 0x1a, 0x00);
	os02n10_1l_write_register(ViPipe, 0x1b, 0x00);

	os02n10_1l_write_register(ViPipe, 0xfd, 0x01);
	os02n10_1l_write_register(ViPipe, 0x0e, 0x01);
	os02n10_1l_write_register(ViPipe, 0x0f, 0x02);
	os02n10_1l_write_register(ViPipe, 0x14, 0x04);
	os02n10_1l_write_register(ViPipe, 0x15, 0x90);
	os02n10_1l_write_register(ViPipe, 0x24, 0xff);
	os02n10_1l_write_register(ViPipe, 0x2f, 0x30);
	os02n10_1l_write_register(ViPipe, 0xfe, 0x02);
	os02n10_1l_write_register(ViPipe, 0x2b, 0xff);
	os02n10_1l_write_register(ViPipe, 0x30, 0x00);
	os02n10_1l_write_register(ViPipe, 0x31, 0x16);
	os02n10_1l_write_register(ViPipe, 0x32, 0x25);
	os02n10_1l_write_register(ViPipe, 0x33, 0xfb);
	os02n10_1l_write_register(ViPipe, 0x35, 0x00); //fix exp error
	os02n10_1l_write_register(ViPipe, 0xfd, 0x01);
	os02n10_1l_write_register(ViPipe, 0x50, 0x03);
	os02n10_1l_write_register(ViPipe, 0x51, 0x07);
	os02n10_1l_write_register(ViPipe, 0x52, 0x04);
	os02n10_1l_write_register(ViPipe, 0x53, 0x05);
	os02n10_1l_write_register(ViPipe, 0x57, 0x40);
	os02n10_1l_write_register(ViPipe, 0x66, 0x04);
	os02n10_1l_write_register(ViPipe, 0x6d, 0x58);
	os02n10_1l_write_register(ViPipe, 0x77, 0x01);
	os02n10_1l_write_register(ViPipe, 0x79, 0x32);
	os02n10_1l_write_register(ViPipe, 0x7c, 0x01);
	os02n10_1l_write_register(ViPipe, 0x90, 0x3b);
	os02n10_1l_write_register(ViPipe, 0x91, 0x0b);
	os02n10_1l_write_register(ViPipe, 0x92, 0x18);
	os02n10_1l_write_register(ViPipe, 0x95, 0x40);
	os02n10_1l_write_register(ViPipe, 0x99, 0x05);
	os02n10_1l_write_register(ViPipe, 0xaa, 0x0e);
	os02n10_1l_write_register(ViPipe, 0xab, 0x0c);
	os02n10_1l_write_register(ViPipe, 0xac, 0x10);
	os02n10_1l_write_register(ViPipe, 0xad, 0x10);
	os02n10_1l_write_register(ViPipe, 0xae, 0x20);
	os02n10_1l_write_register(ViPipe, 0xb0, 0x0e);
	os02n10_1l_write_register(ViPipe, 0xb1, 0x0f);
	os02n10_1l_write_register(ViPipe, 0xb2, 0x1a);
	os02n10_1l_write_register(ViPipe, 0xb3, 0x1c);

	os02n10_1l_write_register(ViPipe, 0xfd, 0x00);
	os02n10_1l_write_register(ViPipe, 0xb0, 0x00);
	os02n10_1l_write_register(ViPipe, 0xb1, 0x14);
	os02n10_1l_write_register(ViPipe, 0xb2, 0x00);
	os02n10_1l_write_register(ViPipe, 0xb3, 0x10);

	os02n10_1l_write_register(ViPipe, 0xfd, 0x03);
	os02n10_1l_write_register(ViPipe, 0x08, 0x00);
	os02n10_1l_write_register(ViPipe, 0x09, 0x20);
	os02n10_1l_write_register(ViPipe, 0x0a, 0x02);
	os02n10_1l_write_register(ViPipe, 0x0b, 0x80);
	os02n10_1l_write_register(ViPipe, 0x11, 0x41);
	os02n10_1l_write_register(ViPipe, 0x12, 0x41);
	os02n10_1l_write_register(ViPipe, 0x13, 0x41);
	os02n10_1l_write_register(ViPipe, 0x14, 0x41);
	os02n10_1l_write_register(ViPipe, 0x17, 0x72);
	os02n10_1l_write_register(ViPipe, 0x18, 0x6f);
	os02n10_1l_write_register(ViPipe, 0x19, 0x70);
	os02n10_1l_write_register(ViPipe, 0x1a, 0x6f);
	os02n10_1l_write_register(ViPipe, 0x1b, 0xc0);
	os02n10_1l_write_register(ViPipe, 0x1d, 0x01);
	os02n10_1l_write_register(ViPipe, 0x1f, 0x80);
	os02n10_1l_write_register(ViPipe, 0x20, 0x40);
	os02n10_1l_write_register(ViPipe, 0x21, 0x80);
	os02n10_1l_write_register(ViPipe, 0x22, 0x40);
	os02n10_1l_write_register(ViPipe, 0x23, 0x88);
	os02n10_1l_write_register(ViPipe, 0x4b, 0x06);
	os02n10_1l_write_register(ViPipe, 0x0e, 0x03);
	os02n10_1l_write_register(ViPipe, 0x58, 0x7b);
	os02n10_1l_write_register(ViPipe, 0x59, 0x17);
	os02n10_1l_write_register(ViPipe, 0x5a, 0x32);

	os02n10_1l_write_register(ViPipe, 0xfd, 0x03);
	os02n10_1l_write_register(ViPipe, 0x4c, 0x01);
	os02n10_1l_write_register(ViPipe, 0x4d, 0x01);
	os02n10_1l_write_register(ViPipe, 0x4e, 0x01);
	os02n10_1l_write_register(ViPipe, 0x4f, 0x02);

	os02n10_1l_write_register(ViPipe, 0xfd, 0x00);
	os02n10_1l_write_register(ViPipe, 0x13, 0xbe);
	os02n10_1l_write_register(ViPipe, 0x14, 0x02);
	os02n10_1l_write_register(ViPipe, 0x4c, 0x24);
	os02n10_1l_write_register(ViPipe, 0xb6, 0x00);
	os02n10_1l_write_register(ViPipe, 0xb7, 0x08);
	os02n10_1l_write_register(ViPipe, 0xb9, 0xd6);
	os02n10_1l_write_register(ViPipe, 0xc6, 0x95);
	os02n10_1l_write_register(ViPipe, 0xc7, 0x77);
	os02n10_1l_write_register(ViPipe, 0xc9, 0x22);
	os02n10_1l_write_register(ViPipe, 0xca, 0x32);
	os02n10_1l_write_register(ViPipe, 0xd7, 0xaa);
	os02n10_1l_write_register(ViPipe, 0xbc, 0x1f);
	os02n10_1l_write_register(ViPipe, 0xbd, 0x60);
	os02n10_1l_write_register(ViPipe, 0xbe, 0x78);
	os02n10_1l_write_register(ViPipe, 0xbf, 0xa5);
	os02n10_1l_write_register(ViPipe, 0xcb, 0x00);
	os02n10_1l_write_register(ViPipe, 0xcc, 0x00);
	os02n10_1l_write_register(ViPipe, 0xce, 0x20);
	os02n10_1l_write_register(ViPipe, 0xcf, 0x3f);
	os02n10_1l_write_register(ViPipe, 0xd0, 0x76);
	os02n10_1l_write_register(ViPipe, 0xd1, 0xec);

	os02n10_1l_write_register(ViPipe, 0xfd, 0x04);
	os02n10_1l_write_register(ViPipe, 0x1b, 0x01);

	// os02n10_1l_write_register(ViPipe, 0xfd, 0x03);
	// os02n10_1l_write_register(ViPipe, 0x01, 0x04);
	// os02n10_1l_write_register(ViPipe, 0x02, 0x07);
	// os02n10_1l_write_register(ViPipe, 0x03, 0x80);
	// os02n10_1l_write_register(ViPipe, 0x05, 0x04);
	// os02n10_1l_write_register(ViPipe, 0x06, 0x04);
	// os02n10_1l_write_register(ViPipe, 0x07, 0x38); //modify output size

	os02n10_1l_write_register(ViPipe, 0xfd, 0x00);
	os02n10_1l_write_register(ViPipe, 0x1e, 0x5f);
	os02n10_1l_write_register(ViPipe, 0x3e, 0x00);
	os02n10_1l_write_register(ViPipe, 0x0a, 0x00);
	os02n10_1l_write_register(ViPipe, 0x1d, 0xa0);
	os02n10_1l_write_register(ViPipe, 0x21, 0x04);
	os02n10_1l_write_register(ViPipe, 0x24, 0x02);
	os02n10_1l_write_register(ViPipe, 0x27, 0x07);
	os02n10_1l_write_register(ViPipe, 0x28, 0x88); //780 1920 to 788 1928
	os02n10_1l_write_register(ViPipe, 0x29, 0x04);
	os02n10_1l_write_register(ViPipe, 0x2a, 0x40); //438 1080 to 440 1088

	os02n10_1l_write_register(ViPipe, 0x2d, 0x04);
	os02n10_1l_write_register(ViPipe, 0x2e, 0x03);
	os02n10_1l_write_register(ViPipe, 0x2f, 0x0c);
	os02n10_1l_write_register(ViPipe, 0x31, 0x04);
	os02n10_1l_write_register(ViPipe, 0x32, 0x1a);
	os02n10_1l_write_register(ViPipe, 0x33, 0x04);
	os02n10_1l_write_register(ViPipe, 0x34, 0x03);
	os02n10_1l_write_register(ViPipe, 0x3f, 0x41);
	os02n10_1l_write_register(ViPipe, 0x40, 0x34);
	os02n10_1l_write_register(ViPipe, 0x23, 0x01);

	os02n10_1l_write_register(ViPipe, 0xfd, 0x03);
	os02n10_1l_write_register(ViPipe, 0x26, 0x01);
	os02n10_1l_write_register(ViPipe, 0xe8, 0x00); //color bar

	os02n10_1l_write_register(ViPipe, 0x28, 0x0a);
	os02n10_1l_write_register(ViPipe, 0x29, 0x0a);
	os02n10_1l_write_register(ViPipe, 0x2a, 0x52);
	os02n10_1l_write_register(ViPipe, 0x2b, 0x5a);

	os02n10_1l_write_register(ViPipe, 0x2c, 0x0a);
	os02n10_1l_write_register(ViPipe, 0x2d, 0x0a);
	os02n10_1l_write_register(ViPipe, 0x2e, 0x52);
	os02n10_1l_write_register(ViPipe, 0x2f, 0x5a);
	os02n10_1l_write_register(ViPipe, 0x31, 0x0a);
	os02n10_1l_write_register(ViPipe, 0x32, 0x0a);
	os02n10_1l_write_register(ViPipe, 0x33, 0x52);
	os02n10_1l_write_register(ViPipe, 0x34, 0x5a);
	os02n10_1l_write_register(ViPipe, 0x35, 0x0c);
	os02n10_1l_write_register(ViPipe, 0x36, 0x10);
	os02n10_1l_write_register(ViPipe, 0x37, 0x07);
	os02n10_1l_write_register(ViPipe, 0x38, 0x0a);
	os02n10_1l_write_register(ViPipe, 0x39, 0x0c);
	os02n10_1l_write_register(ViPipe, 0x3a, 0x10);
	os02n10_1l_write_register(ViPipe, 0x3b, 0x07);
	os02n10_1l_write_register(ViPipe, 0x3c, 0x0a);
	os02n10_1l_write_register(ViPipe, 0x3d, 0x0c);
	os02n10_1l_write_register(ViPipe, 0x3e, 0x10);
	os02n10_1l_write_register(ViPipe, 0x3f, 0x07);
	os02n10_1l_write_register(ViPipe, 0x40, 0x0a);
	os02n10_1l_write_register(ViPipe, 0x41, 0x07);
	os02n10_1l_write_register(ViPipe, 0x42, 0x07);
	os02n10_1l_write_register(ViPipe, 0x43, 0x07);
	os02n10_1l_write_register(ViPipe, 0x44, 0x07);
	os02n10_1l_write_register(ViPipe, 0x46, 0x10);
	os02n10_1l_write_register(ViPipe, 0x47, 0xe2);
	os02n10_1l_write_register(ViPipe, 0x45, 0x50);
	os02n10_1l_write_register(ViPipe, 0xfb, 0x03);

	os02n10_1l_default_reg_init(ViPipe);
	delay_ms(100);
	printf("ViPipe:%d,===OS02N10_1L 1080P 15fps 10bit LINE Init OK!===\n", ViPipe);
}
