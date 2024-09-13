#include <stdio.h>
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
#include "gc2385_1l_cmos_ex.h"

#define GC2385_1L_CHIP_ID_ADDR_H	0xf0
#define GC2385_1L_CHIP_ID_ADDR_L	0xf1
#define GC2385_1L_CHIP_ID		0x2385

static void gc2385_1l_linear_1200p30_init(VI_PIPE ViPipe);

const CVI_U8 gc2385_1l_i2c_addr = 0x37;
const CVI_U32 gc2385_1l_addr_byte = 1;
const CVI_U32 gc2385_1l_data_byte = 1;
static int g_fd[VI_MAX_PIPE_NUM] = {[0 ... (VI_MAX_PIPE_NUM - 1)] = -1};

int gc2385_1l_i2c_init(VI_PIPE ViPipe)
{
	char acDevFile[16] = {0};
	CVI_U8 u8DevNum;

	if (g_fd[ViPipe] >= 0)
		return CVI_SUCCESS;
	int ret;

	u8DevNum = g_aunGc2385_1L_BusInfo[ViPipe].s8I2cDev;
	snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);

	g_fd[ViPipe] = open(acDevFile, O_RDWR, 0600);

	if (g_fd[ViPipe] < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Open /dev/i2c-%u error!\n", u8DevNum);
		return CVI_FAILURE;
	}

	ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, gc2385_1l_i2c_addr);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_SLAVE_FORCE error!\n");
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return ret;
	}
	return CVI_SUCCESS;
}

int gc2385_1l_i2c_exit(VI_PIPE ViPipe)
{
	if (g_fd[ViPipe] >= 0) {
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return CVI_SUCCESS;
	}
	return CVI_FAILURE;
}

int gc2385_1l_read_register(VI_PIPE ViPipe, int addr)
{
	int ret, data;
	CVI_U8 buf[8];
	CVI_U8 idx = 0;

	if (g_fd[ViPipe] < 0)
		return CVI_FAILURE;

	if (gc2385_1l_addr_byte == 2)
		buf[idx++] = (addr >> 8) & 0xff;

	// add address byte 0
	buf[idx++] = addr & 0xff;

	ret = write(g_fd[ViPipe], buf, gc2385_1l_addr_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_WRITE error!\n");
		return ret;
	}

	buf[0] = 0;
	buf[1] = 0;
	ret = read(g_fd[ViPipe], buf, gc2385_1l_data_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_READ error!\n");
		return ret;
	}

	// pack read back data
	data = 0;
	if (gc2385_1l_data_byte == 2) {
		data = buf[0] << 8;
		data += buf[1];
	} else {
		data = buf[0];
	}

	syslog(LOG_DEBUG, "i2c r 0x%x = 0x%x\n", addr, data);
	return data;
}

int gc2385_1l_write_register(VI_PIPE ViPipe, int addr, int data)
{
	CVI_U8 idx = 0;
	int ret;
	CVI_U8 buf[8];

	if (g_fd[ViPipe] < 0)
		return CVI_SUCCESS;

	if (gc2385_1l_addr_byte == 1) {
		buf[idx] = addr & 0xff;
		idx++;
	}
	if (gc2385_1l_data_byte == 1) {
		buf[idx] = data & 0xff;
		idx++;
	}

	ret = write(g_fd[ViPipe], buf, gc2385_1l_addr_byte + gc2385_1l_data_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_WRITE error!\n");
		return CVI_FAILURE;
	}
	//ret = read(g_fd[ViPipe], buf, gc2385_1l_addr_byte + gc2385_1l_data_byte);
	syslog(LOG_DEBUG, "i2c w 0x%x 0x%x\n", addr, data);
	return CVI_SUCCESS;
}

static void delay_ms(int ms)
{
	usleep(ms * 1000);
}

void gc2385_1l_standby(VI_PIPE ViPipe)
{
	UNUSED(ViPipe);
	printf("gc2385_1l_standby\n");
}

void gc2385_1l_restart(VI_PIPE ViPipe)
{
	UNUSED(ViPipe);
	printf("gc2385_1l_restart\n");
}

void gc2385_1l_default_reg_init(VI_PIPE ViPipe)
{
	CVI_U32 i;

	for (i = 0; i < g_pastGc2385_1L[ViPipe]->astSyncInfo[0].snsCfg.u32RegNum; i++) {
		gc2385_1l_write_register(ViPipe,
				g_pastGc2385_1L[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32RegAddr,
				g_pastGc2385_1L[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32Data);
	}
}

int gc2385_1l_probe(VI_PIPE ViPipe)
{
	int nVal;
	int nVal2;

	usleep(50);
	if (gc2385_1l_i2c_init(ViPipe) != CVI_SUCCESS)
		return CVI_FAILURE;

	nVal  = gc2385_1l_read_register(ViPipe, GC2385_1L_CHIP_ID_ADDR_H);
	nVal2 = gc2385_1l_read_register(ViPipe, GC2385_1L_CHIP_ID_ADDR_L);
	if (nVal < 0 || nVal2 < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "read sensor id error.\n");
		return nVal;
	}

	if ((((nVal & 0xFF) << 8) | (nVal2 & 0xFF)) != GC2385_1L_CHIP_ID) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Sensor ID Mismatch! Use the wrong sensor??\n");
		return CVI_FAILURE;
	}
	return CVI_SUCCESS;
}

void gc2385_1l_init(VI_PIPE ViPipe)
{
	WDR_MODE_E enWDRMode = g_pastGc2385_1L[ViPipe]->enWDRMode;

	gc2385_1l_i2c_init(ViPipe);

	if (enWDRMode == WDR_MODE_2To1_LINE) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "not surpport this WDR_MODE_E!\n");
	} else {
		gc2385_1l_linear_1200p30_init(ViPipe);
	}
	g_pastGc2385_1L[ViPipe]->bInit = CVI_TRUE;
}

void gc2385_1l_exit(VI_PIPE ViPipe)
{
	gc2385_1l_i2c_exit(ViPipe);
}

static void gc2385_1l_linear_1200p30_init(VI_PIPE ViPipe)
{
	/****system****/
	gc2385_1l_write_register(ViPipe, 0xfe,0x00);
	gc2385_1l_write_register(ViPipe, 0xfe,0x00);
	gc2385_1l_write_register(ViPipe, 0xfe,0x00);
	gc2385_1l_write_register(ViPipe, 0xf2,0x02);
	gc2385_1l_write_register(ViPipe, 0xf4,0x03);
	gc2385_1l_write_register(ViPipe, 0xf7,0x01);
	gc2385_1l_write_register(ViPipe, 0xf8,0x28);
	gc2385_1l_write_register(ViPipe, 0xf9,0x02);
	gc2385_1l_write_register(ViPipe, 0xfa,0x08);
	gc2385_1l_write_register(ViPipe, 0xfc,0x8e);
	gc2385_1l_write_register(ViPipe, 0xe7,0xcc);
	gc2385_1l_write_register(ViPipe, 0x88,0x03);

	/****************************************/
	/*analog*/
	/****************************************/
	gc2385_1l_write_register(ViPipe, 0x03,0x04);
	gc2385_1l_write_register(ViPipe, 0x04,0x80);
	gc2385_1l_write_register(ViPipe, 0x05,0x02);
	gc2385_1l_write_register(ViPipe, 0x06,0x86);
	gc2385_1l_write_register(ViPipe, 0x07,0x00);

	gc2385_1l_write_register(ViPipe, 0x08,0x10);
	gc2385_1l_write_register(ViPipe, 0x09,0x00);
	gc2385_1l_write_register(ViPipe, 0x0a,0x04);//00
	gc2385_1l_write_register(ViPipe, 0x0b,0x00);
	gc2385_1l_write_register(ViPipe, 0x0c,0x02);//04
	gc2385_1l_write_register(ViPipe, 0x17,0xd4);
	gc2385_1l_write_register(ViPipe, 0x18,0x02);
	gc2385_1l_write_register(ViPipe, 0x19,0x17);
	gc2385_1l_write_register(ViPipe, 0x1c,0x18);//10
	gc2385_1l_write_register(ViPipe, 0x20,0x73);//71
	gc2385_1l_write_register(ViPipe, 0x21,0x38);
	gc2385_1l_write_register(ViPipe, 0x22,0xa2);//a1
	gc2385_1l_write_register(ViPipe, 0x29,0x20);
	gc2385_1l_write_register(ViPipe, 0x2f,0x14);
	gc2385_1l_write_register(ViPipe, 0x3f,0x40);
	gc2385_1l_write_register(ViPipe, 0xcd,0x94);
	gc2385_1l_write_register(ViPipe, 0xce,0x45);
	gc2385_1l_write_register(ViPipe, 0xd1,0x0c);
	gc2385_1l_write_register(ViPipe, 0xd7,0x9b);
	gc2385_1l_write_register(ViPipe, 0xd8,0x99);
	gc2385_1l_write_register(ViPipe, 0xda,0x3b);
	gc2385_1l_write_register(ViPipe, 0xd9,0xb5);
	gc2385_1l_write_register(ViPipe, 0xdb,0x75);
	gc2385_1l_write_register(ViPipe, 0xe3,0x1b);
	gc2385_1l_write_register(ViPipe, 0xe4,0xf8);
	/****************************************/
	/*BLk*/
	/****************************************/
	gc2385_1l_write_register(ViPipe, 0x40,0x22);
	gc2385_1l_write_register(ViPipe, 0x43,0x07);
	gc2385_1l_write_register(ViPipe, 0x4e,0x3c);
	gc2385_1l_write_register(ViPipe, 0x4f,0x00);
	gc2385_1l_write_register(ViPipe, 0x68,0x00);
	/****************************************/
	/*gain*/
	/****************************************/
	gc2385_1l_write_register(ViPipe, 0xb0,0x46);
	gc2385_1l_write_register(ViPipe, 0xb1,0x01);
	gc2385_1l_write_register(ViPipe, 0xb2,0x00);
	gc2385_1l_write_register(ViPipe, 0xb6,0x00);
	/****************************************/
	/*out crop*/
	/****************************************/
	gc2385_1l_write_register(ViPipe, 0x90,0x01);
	gc2385_1l_write_register(ViPipe, 0x92,0x03); //crop_win_y1
	gc2385_1l_write_register(ViPipe, 0x94,0x05); //crop_win_x1
	gc2385_1l_write_register(ViPipe, 0x95,0x04);
	gc2385_1l_write_register(ViPipe, 0x96,0xb0);
	gc2385_1l_write_register(ViPipe, 0x97,0x06);
	gc2385_1l_write_register(ViPipe, 0x98,0x40);
	/****************************************/
	/*mipi set*/
	/****************************************/
	gc2385_1l_write_register(ViPipe, 0xfe,0x00);
	gc2385_1l_write_register(ViPipe, 0xed,0x90);
	gc2385_1l_write_register(ViPipe, 0xfe,0x03);
	gc2385_1l_write_register(ViPipe, 0x01,0x03);
	gc2385_1l_write_register(ViPipe, 0x02,0x82);
	gc2385_1l_write_register(ViPipe, 0x03,0xd0);//e0->d0 20170223
	gc2385_1l_write_register(ViPipe, 0x04,0x04);
	gc2385_1l_write_register(ViPipe, 0x05,0x00);
	gc2385_1l_write_register(ViPipe, 0x06,0x80);
	gc2385_1l_write_register(ViPipe, 0x11,0x2b);
	gc2385_1l_write_register(ViPipe, 0x12,0xd0);
	gc2385_1l_write_register(ViPipe, 0x13,0x07);
	gc2385_1l_write_register(ViPipe, 0x15,0x00);
	gc2385_1l_write_register(ViPipe, 0x1b,0x10);
	gc2385_1l_write_register(ViPipe, 0x1c,0x10);
	gc2385_1l_write_register(ViPipe, 0x21,0x08);
	gc2385_1l_write_register(ViPipe, 0x22,0x05);
	gc2385_1l_write_register(ViPipe, 0x23,0x13);
	gc2385_1l_write_register(ViPipe, 0x24,0x02);
	gc2385_1l_write_register(ViPipe, 0x25,0x13);
	gc2385_1l_write_register(ViPipe, 0x26,0x06);//08
	gc2385_1l_write_register(ViPipe, 0x29,0x06);
	gc2385_1l_write_register(ViPipe, 0x2a,0x08);
	gc2385_1l_write_register(ViPipe, 0x2b,0x06);//08
	gc2385_1l_write_register(ViPipe, 0xfe,0x00);

	gc2385_1l_default_reg_init(ViPipe);
	delay_ms(10);

	printf("ViPipe:%d,===GC2385_1L 1200P 30fps 10bit LINEAR Init OK!===\n", ViPipe);
}

