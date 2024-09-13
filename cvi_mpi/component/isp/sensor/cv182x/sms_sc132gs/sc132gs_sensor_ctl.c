#include "cvi_sns_ctrl.h"
#include <linux/cvi_comm_video.h>
#include "cvi_sns_ctrl.h"

// #include "sensor_i2c.h"
#include <unistd.h>

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


#include "sc132gs_cmos_ex.h"

static int g_fd[VI_MAX_PIPE_NUM] = {[0 ... (VI_MAX_PIPE_NUM - 1)] = -1};

#define SC132GS_CHIP_ID_ADDR_H	0x3107
#define SC132GS_CHIP_ID_ADDR_L	0x3108
#define SC132GS_CHIP_ID		0x132

static void sc132gs_1080p_init(VI_PIPE ViPipe);

CVI_U8 sc132gs_i2c_addr = 0x30;
const CVI_U32 sc132gs_addr_byte = 2;
const CVI_U32 sc132gs_data_byte = 1;

int sc132gs_i2c_init(VI_PIPE ViPipe)
{
	char acDevFile[16] = {0};
	CVI_U8 u8DevNum;

	if (g_fd[ViPipe] >= 0)
		return CVI_SUCCESS;
	int ret;

	u8DevNum = g_aunSc132gs_BusInfo[ViPipe].s8I2cDev;
	// printf("iic u8DevNum = %d\n", u8DevNum);
	snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);

	g_fd[ViPipe] = open(acDevFile, O_RDWR, 0600);

	if (g_fd[ViPipe] < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Open /dev/cvi_i2c_drv-%u error!\n", u8DevNum);
		return CVI_FAILURE;
	}

	ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, sc132gs_i2c_addr);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_SLAVE_FORCE error!\n");
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return ret;
	}

	return CVI_SUCCESS;
}

int sc132gs_i2c_exit(VI_PIPE ViPipe)
{
	if (g_fd[ViPipe] >= 0) {
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return CVI_SUCCESS;
	}
	return CVI_FAILURE;
}

int sc132gs_read_register(VI_PIPE ViPipe, int addr)
{
	int ret, data;
	CVI_U8 buf[8];
	CVI_U8 idx = 0;

	if (g_fd[ViPipe] < 0)
		return CVI_FAILURE;

	if (sc132gs_addr_byte == 2)
		buf[idx++] = (addr >> 8) & 0xff;

	// add address byte 0
	buf[idx++] = addr & 0xff;

	ret = write(g_fd[ViPipe], buf, sc132gs_addr_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_WRITE error!\n");
		return ret;
	}

	buf[0] = 0;
	buf[1] = 0;
	ret = read(g_fd[ViPipe], buf, sc132gs_data_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_READ error!\n");
		return ret;
	}

	// pack read back data
	data = 0;
	if (sc132gs_data_byte == 2) {
		data = buf[0] << 8;
		data += buf[1];
	} else {
		data = buf[0];
	}

	syslog(LOG_DEBUG, "i2c r 0x%x = 0x%x\n", addr, data);
	// printf("i2c r 0x%x = 0x%x\n", addr, data);
	return data;
}

int sc132gs_write_register(VI_PIPE ViPipe, int addr, int data)
{
	CVI_U8  idx = 0;
	int ret;
	CVI_U8  buf[8];

	if (g_fd[ViPipe] < 0)
		return CVI_SUCCESS;

	if (sc132gs_addr_byte == 2) {
		buf[idx] = (addr >> 8) & 0xff;
		idx++;
		buf[idx] = addr & 0xff;
		idx++;
	}

	if (sc132gs_data_byte == 1) {
		buf[idx] = data & 0xff;
		idx++;
	}

	ret = write(g_fd[ViPipe], buf, sc132gs_addr_byte + sc132gs_data_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_WRITE error!\n");
		return CVI_FAILURE;
	}
	//syslog(LOG_DEBUG, "i2c w 0x%x 0x%x\n", addr, data);
	return CVI_SUCCESS;
}

static void delay_ms(int ms)
{
	usleep(ms * 1000);
}

void sc132gs_standby(VI_PIPE ViPipe)
{
	sc132gs_write_register(ViPipe, 0x0100, 0x00);

	printf("%s i2c_addr:%x, i2c_dev:%d\n", __func__, sc132gs_i2c_addr, g_aunSc132gs_BusInfo[ViPipe].s8I2cDev);
}

void sc132gs_restart(VI_PIPE ViPipe)
{
	sc132gs_write_register(ViPipe, 0x0100, 0x01);

	printf("%s i2c_addr:%x, i2c_dev:%d\n", __func__, sc132gs_i2c_addr, g_aunSc132gs_BusInfo[ViPipe].s8I2cDev);
}

void sc132gs_default_reg_init(VI_PIPE ViPipe)
{
	CVI_U32 i;

	for (i = 0; i < g_pastSc132gs[ViPipe]->astSyncInfo[0].snsCfg.u32RegNum; i++) {
		sc132gs_write_register(ViPipe,
				g_pastSc132gs[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32RegAddr,
				g_pastSc132gs[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32Data);
	}
}

void sc132gs_mirror_flip(VI_PIPE ViPipe, ISP_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip)
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

	sc132gs_write_register(ViPipe, 0x3221, val);
}

int sc132gs_probe(VI_PIPE ViPipe)
{
	int nVal;
	CVI_U16 chip_id;

	delay_ms(4);
	if (sc132gs_i2c_init(ViPipe) != CVI_SUCCESS)
		return CVI_FAILURE;

	nVal = sc132gs_read_register(ViPipe, SC132GS_CHIP_ID_ADDR_H);
	if (nVal < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "read sensor id error.\n");
		return nVal;
	}
	chip_id = (nVal & 0xFF) << 8;
	nVal = sc132gs_read_register(ViPipe, SC132GS_CHIP_ID_ADDR_L);
	if (nVal < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "read sensor id error.\n");
		return nVal;
	}
	chip_id |= (nVal & 0xFF);

	if (chip_id != SC132GS_CHIP_ID) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Sensor ID Mismatch! Use the wrong sensor??\n");
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

void sc132gs_init(VI_PIPE ViPipe)
{
	sc132gs_i2c_init(ViPipe);

	//linear mode only
	sc132gs_1080p_init(ViPipe);

	g_pastSc132gs[ViPipe]->bInit = CVI_TRUE;
}

void sc132gs_exit(VI_PIPE ViPipe)
{
	sc132gs_i2c_exit(ViPipe);
}


static void sc132gs_1080p_init(VI_PIPE ViPipe)
{
	sc132gs_write_register(ViPipe,0x0103,0x01);
	sc132gs_write_register(ViPipe,0x0100,0x00);
	sc132gs_write_register(ViPipe,0x36e9,0x80);
	sc132gs_write_register(ViPipe,0x36f9,0x80);
	sc132gs_write_register(ViPipe,0x3000,0x01);
	sc132gs_write_register(ViPipe,0x3018,0x32);
	sc132gs_write_register(ViPipe,0x3019,0x0c);
	sc132gs_write_register(ViPipe,0x301a,0xb4);
	sc132gs_write_register(ViPipe,0x301f,0x86);
	sc132gs_write_register(ViPipe,0x3031,0x0a);
	sc132gs_write_register(ViPipe,0x3032,0x60);
	sc132gs_write_register(ViPipe,0x3038,0x44);
	sc132gs_write_register(ViPipe,0x3207,0x17);
	sc132gs_write_register(ViPipe,0x320c,0x02);
	sc132gs_write_register(ViPipe,0x320d,0xee);
	sc132gs_write_register(ViPipe,0x3250,0xcc);
	sc132gs_write_register(ViPipe,0x3251,0x02);
	sc132gs_write_register(ViPipe,0x3252,0x05);
	sc132gs_write_register(ViPipe,0x3253,0x41);
	sc132gs_write_register(ViPipe,0x3254,0x05);
	sc132gs_write_register(ViPipe,0x3255,0x3b);
	sc132gs_write_register(ViPipe,0x3306,0x78);
	sc132gs_write_register(ViPipe,0x330a,0x00);
	sc132gs_write_register(ViPipe,0x330b,0xc8);
	sc132gs_write_register(ViPipe,0x330f,0x24);
	sc132gs_write_register(ViPipe,0x3314,0x80);
	sc132gs_write_register(ViPipe,0x3315,0x40);
	sc132gs_write_register(ViPipe,0x3317,0xf0);
	sc132gs_write_register(ViPipe,0x331f,0x12);
	sc132gs_write_register(ViPipe,0x3364,0x00);
	sc132gs_write_register(ViPipe,0x3385,0x41);
	sc132gs_write_register(ViPipe,0x3387,0x41);
	sc132gs_write_register(ViPipe,0x3389,0x09);
	sc132gs_write_register(ViPipe,0x33ab,0x00);
	sc132gs_write_register(ViPipe,0x33ac,0x00);
	sc132gs_write_register(ViPipe,0x33b1,0x03);
	sc132gs_write_register(ViPipe,0x33b2,0x12);
	sc132gs_write_register(ViPipe,0x33f8,0x02);
	sc132gs_write_register(ViPipe,0x33fa,0x01);
	sc132gs_write_register(ViPipe,0x3409,0x08);
	sc132gs_write_register(ViPipe,0x34f0,0xc0);
	sc132gs_write_register(ViPipe,0x34f1,0x20);
	sc132gs_write_register(ViPipe,0x34f2,0x03);
	sc132gs_write_register(ViPipe,0x3622,0xf5);
	sc132gs_write_register(ViPipe,0x3630,0x5c);
	sc132gs_write_register(ViPipe,0x3631,0x80);
	sc132gs_write_register(ViPipe,0x3632,0xc8);
	sc132gs_write_register(ViPipe,0x3633,0x32);
	sc132gs_write_register(ViPipe,0x3638,0x2a);
	sc132gs_write_register(ViPipe,0x3639,0x07);
	sc132gs_write_register(ViPipe,0x363b,0x48);
	sc132gs_write_register(ViPipe,0x363c,0x83);
	sc132gs_write_register(ViPipe,0x363d,0x10);
	sc132gs_write_register(ViPipe,0x36ea,0x37);
	sc132gs_write_register(ViPipe,0x36eb,0x04);
	sc132gs_write_register(ViPipe,0x36ec,0x03);
	sc132gs_write_register(ViPipe,0x36ed,0x24);
	sc132gs_write_register(ViPipe,0x36fa,0x25);
	sc132gs_write_register(ViPipe,0x36fb,0x15);
	sc132gs_write_register(ViPipe,0x36fc,0x10);
	sc132gs_write_register(ViPipe,0x36fd,0x04);
	sc132gs_write_register(ViPipe,0x3900,0x11);
	sc132gs_write_register(ViPipe,0x3901,0x05);
	sc132gs_write_register(ViPipe,0x3902,0xc5);
	sc132gs_write_register(ViPipe,0x3904,0x04);
	sc132gs_write_register(ViPipe,0x3908,0x91);
	sc132gs_write_register(ViPipe,0x391e,0x00);
	sc132gs_write_register(ViPipe,0x3e01,0x53);
	sc132gs_write_register(ViPipe,0x3e02,0xe0);
	sc132gs_write_register(ViPipe,0x3e09,0x20);
	sc132gs_write_register(ViPipe,0x3e0e,0xd2);
	sc132gs_write_register(ViPipe,0x3e14,0xb0);
	sc132gs_write_register(ViPipe,0x3e1e,0x7c);
	sc132gs_write_register(ViPipe,0x3e26,0x20);
	sc132gs_write_register(ViPipe,0x4418,0x38);
	sc132gs_write_register(ViPipe,0x4503,0x10);
	sc132gs_write_register(ViPipe,0x4800,0x24);
	sc132gs_write_register(ViPipe,0x4837,0x1a);
	sc132gs_write_register(ViPipe,0x5000,0x0e);
	sc132gs_write_register(ViPipe,0x540c,0x51);
	sc132gs_write_register(ViPipe,0x550f,0x38);
	sc132gs_write_register(ViPipe,0x5780,0x67);
	sc132gs_write_register(ViPipe,0x5784,0x10);
	sc132gs_write_register(ViPipe,0x5785,0x06);
	sc132gs_write_register(ViPipe,0x5787,0x02);
	sc132gs_write_register(ViPipe,0x5788,0x00);
	sc132gs_write_register(ViPipe,0x5789,0x00);
	sc132gs_write_register(ViPipe,0x578a,0x02);
	sc132gs_write_register(ViPipe,0x578b,0x00);
	sc132gs_write_register(ViPipe,0x578c,0x00);
	sc132gs_write_register(ViPipe,0x5790,0x00);
	sc132gs_write_register(ViPipe,0x5791,0x00);
	sc132gs_write_register(ViPipe,0x5792,0x00);
	sc132gs_write_register(ViPipe,0x5793,0x00);
	sc132gs_write_register(ViPipe,0x5794,0x00);
	sc132gs_write_register(ViPipe,0x5795,0x00);
	sc132gs_write_register(ViPipe,0x5799,0x04);
	sc132gs_write_register(ViPipe,0x36e9,0x20);
	sc132gs_write_register(ViPipe,0x36f9,0x24);

	sc132gs_default_reg_init(ViPipe);

	sc132gs_write_register(ViPipe,0x0100,0x01);

	printf("ViPipe:%d,===SC132GS 1080P 60fps 10bit LINEAR Init OK!===\n", ViPipe);
}
