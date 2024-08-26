#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#ifdef ARCH_CV182X
#include <linux/cvi_vip_snsr.h>
#include "cvi_comm_video.h"
#else
#include <linux/cvi_comm_video.h>
#endif
#include "cvi_sns_ctrl.h"
#include "xs9922b_cmos_ex.h"
#include <pthread.h>
#include <signal.h>

static void xs9922b_set_720p_25fps(VI_PIPE ViPipe);

const CVI_U8 xs9922b_master_i2c_addr = 0x30;        /* I2C slave address of XS9922b master chip  0x5C*/
const CVI_U8 xs9922b_slave_i2c_addr = 0x5F;         /* I2C slave address of XS9922b slave chip*/
const CVI_U32 xs9922b_addr_byte = 2;
const CVI_U32 xs9922b_data_byte = 1;
static int g_fd[VI_MAX_PIPE_NUM] = {[0 ... (VI_MAX_PIPE_NUM - 1)] = -1};

#define XS9922B_ID0_REG 0x40F1 // id1 0x40F0: 0x99  id0 0x40F1: 0x22
#define XS9922B_ID1_REG 0x40F0 // id1 0x40F0: 0x99  id0 0x40F1: 0x22
#define XS9922B_DEVICE_ID  0x9922 // id1 0x40F0: 0x99  id0 0x40F1: 0x22
#define XS9922B_PATTERN_EN_REG 0x5080 // id1 0x40F0: 0x99  id0 0x40F1: 0x22

#define MAX_BUF 64

static void delay_ms(int ms)
{
	usleep(ms * 1000);
}

int xs9922b_i2c_init(VI_PIPE ViPipe, CVI_U8 i2c_addr)
{
	int ret;
	char acDevFile[16] = {0};
	CVI_U8 u8DevNum;

	if (g_fd[ViPipe] >= 0)
		return CVI_SUCCESS;

	u8DevNum = g_aunXS9922b_BusInfo[ViPipe].s8I2cDev;
	snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);
	CVI_TRACE_SNS(CVI_DBG_INFO, "open %s\n", acDevFile);

	g_fd[ViPipe] = open(acDevFile, O_RDWR, 0600);

	if (g_fd[ViPipe] < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Open /dev/cvi_i2c_drv-%u error!\n", u8DevNum);
		return CVI_FAILURE;
	}

	ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, i2c_addr);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_SLAVE_FORCE error!\n");
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return ret;
	}
	return CVI_SUCCESS;
}

int xs9922b_i2c_exit(VI_PIPE ViPipe)
{
	if (g_fd[ViPipe] >= 0) {
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return CVI_SUCCESS;
	}
	return CVI_FAILURE;
}

int xs9922b_read_register(VI_PIPE ViPipe, int addr)
{
	int ret, data;
	CVI_U8 buf[8];
	CVI_U8 idx = 0;

	if (g_fd[ViPipe] < 0)
		return 0;

	if (xs9922b_addr_byte == 2)
		buf[idx++] = (addr >> 8) & 0xff;

	// add address byte 0
	buf[idx++] = addr & 0xff;

	ret = write(g_fd[ViPipe], buf, xs9922b_addr_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_READ error!\n");
		return 0;
	}

	buf[0] = 0;
	buf[1] = 0;
	ret = read(g_fd[ViPipe], buf, xs9922b_data_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_READ error!\n");
		return 0;
	}

	// pack read back data
	data = 0;
	if (xs9922b_data_byte == 2) {
		data = buf[0] << 8;
		data += buf[1];
	} else {
		data = buf[0];
	}

	CVI_TRACE_SNS(CVI_DBG_INFO, "i2c r 0x%x = 0x%x\n", addr, data);
	return data;
}

int xs9922b_write_register(VI_PIPE ViPipe, int addr, int data)
{
	CVI_U8 idx = 0;
	int ret;
	CVI_U8 buf[8];

	if (g_fd[ViPipe] < 0)
		return CVI_SUCCESS;

	if (xs9922b_addr_byte == 2)
		buf[idx++] = (addr >> 8) & 0xff;

	// add address byte 0
	buf[idx++] = addr & 0xff;

	if (xs9922b_data_byte == 2)
		buf[idx++] = (data >> 8) & 0xff;

	// add data byte 0
	buf[idx++] = data & 0xff;

	ret = write(g_fd[ViPipe], buf, xs9922b_addr_byte + xs9922b_data_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_WRITE error!\n");
		return CVI_FAILURE;
	}
	CVI_TRACE_SNS(CVI_DBG_INFO, "i2c w 0x%x 0x%x\n", addr, data);

#if 0 // read back checing
	ret = xs9922b_read_register(ViPipe, addr);
	if (ret != data)
		CVI_TRACE_SNS(CVI_DBG_INFO, "i2c readback-check fail, 0x%x != 0x%x\n", ret, data);
#endif
	return CVI_SUCCESS;
}

void xs9922b_init(VI_PIPE ViPipe)
{
	if (ViPipe) // only init ch0
		return;

	delay_ms(20);

	if (xs9922b_i2c_init(ViPipe, xs9922b_master_i2c_addr) != CVI_SUCCESS) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "XS9922b master i2c init fail\n");
		return;
	}

	// check sensor chip id
	if (((xs9922b_read_register(ViPipe, XS9922B_ID1_REG) << 8) |
		(xs9922b_read_register(ViPipe, XS9922B_ID0_REG))) != XS9922B_DEVICE_ID) {
		CVI_TRACE_SNS(CVI_DBG_ERR,
			"XS9922b  chip id(0x%04x) check fail, read is 0x%02x%02x\n",
			XS9922B_DEVICE_ID,
			xs9922b_read_register(ViPipe, XS9922B_ID1_REG),
			xs9922b_read_register(ViPipe, XS9922B_ID0_REG));
			return;
	}

	CVI_TRACE_SNS(CVI_DBG_INFO, "Loading ChipUp XS9922b sensor\n");
	xs9922b_set_720p_25fps(ViPipe);
	// wait for signal to stabilize
	delay_ms(100);
	xs9922b_write_register(ViPipe, 0xff, 0x00);//page0
}

void xs9922b_exit(VI_PIPE ViPipe)
{
	CVI_TRACE_SNS(CVI_DBG_INFO, "Exit Pixelplus XS9922b Sensor\n");

	xs9922b_i2c_exit(ViPipe);
}

static void xs9922b_set_720p_25fps(VI_PIPE ViPipe)
{
	CVI_U16 i;
	CVI_U16 addr;
	CVI_U8 val;
	CVI_U8 nDelay;

        CVI_U8 u8ImgMode;
	u8ImgMode = g_pastXS9922b[ViPipe]->u8ImgMode;

	i = 0;
	while (xs9922_init_cfg[i].addr != REG_NULL) {
		addr = xs9922_init_cfg[i].addr;
		val = xs9922_init_cfg[i].val;
		nDelay = xs9922_init_cfg[i].nDelay;
		xs9922b_write_register(ViPipe, addr, val);
		usleep(nDelay);
		i++;
	}

	i = 0;
	while (xs9922_mipi_dphy_set[i].addr != REG_NULL) {
		addr = xs9922_mipi_dphy_set[i].addr;
		val = xs9922_mipi_dphy_set[i].val;
		nDelay = xs9922_mipi_dphy_set[i].nDelay;
		xs9922b_write_register(ViPipe, addr, val);
		usleep(nDelay);
		i++;
	}

	i = 0;
	while (xs9922_720p_4lanes_25fps_1500M[i].addr != REG_NULL) {
		addr = xs9922_720p_4lanes_25fps_1500M[i].addr;
		val = xs9922_720p_4lanes_25fps_1500M[i].val;
		nDelay = xs9922_720p_4lanes_25fps_1500M[i].nDelay;
		xs9922b_write_register(ViPipe, addr, val);
		usleep(nDelay);
		i++;
	}

        i = 0;
        while (xs9922_mipi_quick_streams[i].addr != REG_NULL) {
		addr = xs9922_mipi_quick_streams[i].addr;
		val = xs9922_mipi_quick_streams[i].val;
		nDelay = xs9922_mipi_quick_streams[i].nDelay;
                switch (u8ImgMode) {
		case XS9922B_MODE_720P_1CH:
			if ((XS9922B_CH1_EN_REG == addr) || (XS9922B_CH2_EN_REG == addr)
				|| (XS9922B_CH3_EN_REG == addr))/* Just run one ch */
				val = 0x00;
			break;
		case XS9922B_MODE_720P_2CH:
			if ((XS9922B_CH2_EN_REG == addr)
				|| (XS9922B_CH3_EN_REG == addr)) /* Just two chs */
				val = 0x00;
			break;
		case XS9922B_MODE_720P_3CH:
			if (XS9922B_CH3_EN_REG == addr) /* three chs */
				val = 0x00;
			break;
		default:
			break;
		}
		xs9922b_write_register(ViPipe, addr, val);
		usleep(nDelay);
		i++;
	}
	delay_ms(100);
}