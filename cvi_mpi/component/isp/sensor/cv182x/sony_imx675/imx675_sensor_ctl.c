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
#include "imx675_cmos_ex.h"

static void imx675_linear_5M30_init(VI_PIPE ViPipe);
static void imx675_wdr_5M25_2to1_init(VI_PIPE ViPipe);

const CVI_U8 imx675_i2c_addr = 0x1A;
const CVI_U32 imx675_addr_byte = 2;
const CVI_U32 imx675_data_byte = 1;
static int g_fd[VI_MAX_PIPE_NUM] = {[0 ... (VI_MAX_PIPE_NUM - 1)] = -1};

int imx675_i2c_init(VI_PIPE ViPipe)
{
	char acDevFile[16] = {0};
	CVI_U8 u8DevNum;

	if (g_fd[ViPipe] >= 0)
		return CVI_SUCCESS;
	int ret;

	u8DevNum = g_aunImx675_BusInfo[ViPipe].s8I2cDev;
	snprintf(acDevFile, sizeof(acDevFile),  "/dev/i2c-%u", u8DevNum);

	g_fd[ViPipe] = open(acDevFile, O_RDWR, 0600);

	if (g_fd[ViPipe] < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Open /dev/i2c-%u error!\n", u8DevNum);
		return CVI_FAILURE;
	}

	ret = ioctl(g_fd[ViPipe], I2C_SLAVE_FORCE, imx675_i2c_addr);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_SLAVE_FORCE error!\n");
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return ret;
	}

	return CVI_SUCCESS;
}

int imx675_i2c_exit(VI_PIPE ViPipe)
{
	if (g_fd[ViPipe] >= 0) {
		close(g_fd[ViPipe]);
		g_fd[ViPipe] = -1;
		return CVI_SUCCESS;
	}
	return CVI_FAILURE;
}

int imx675_read_register(VI_PIPE ViPipe, int addr)
{
	int ret, data;
	CVI_U8 buf[8];
	CVI_U8 idx = 0;

	if (g_fd[ViPipe] < 0)
		return CVI_FAILURE;

	if (imx675_addr_byte == 2)
		buf[idx++] = (addr >> 8) & 0xff;

	// add address byte 0
	buf[idx++] = addr & 0xff;

	ret = write(g_fd[ViPipe], buf, imx675_addr_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_WRITE error!\n");
		return ret;
	}

	buf[0] = 0;
	buf[1] = 0;
	ret = read(g_fd[ViPipe], buf, imx675_data_byte);
	if (ret < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "I2C_READ error!\n");
		return ret;
	}

	// pack read back data
	data = 0;
	if (imx675_data_byte == 2) {
		data = buf[0] << 8;
		data += buf[1];
	} else {
		data = buf[0];
	}

	syslog(LOG_DEBUG, "i2c r 0x%x = 0x%x\n", addr, data);
	return data;
}

int imx675_write_register(VI_PIPE ViPipe, int addr, int data)
{
	CVI_U8 idx = 0;
	int ret;
	CVI_U8 buf[8];

	if (g_fd[ViPipe] < 0)
		return CVI_SUCCESS;

	if (imx675_addr_byte == 2) {
		buf[idx] = (addr >> 8) & 0xff;
		idx++;
		buf[idx] = addr & 0xff;
		idx++;
	}

	if (imx675_data_byte == 1) {
		buf[idx] = data & 0xff;
		idx++;
	}

	ret = write(g_fd[ViPipe], buf, imx675_addr_byte + imx675_data_byte);
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

void imx675_standby(VI_PIPE ViPipe)
{
	imx675_write_register(ViPipe, 0x3000, 0x01); /* STANDBY */
	imx675_write_register(ViPipe, 0x3002, 0x01); /* XTMSTA */
}

void imx675_restart(VI_PIPE ViPipe)
{
	imx675_write_register(ViPipe, 0x3000, 0x00); /* standby */
	delay_ms(20);
	imx675_write_register(ViPipe, 0x3002, 0x00); /* master mode start */
}

void imx675_default_reg_init(VI_PIPE ViPipe)
{
	CVI_U32 i;

	for (i = 0; i < g_pastImx675[ViPipe]->astSyncInfo[0].snsCfg.u32RegNum; i++) {
		imx675_write_register(ViPipe,
				g_pastImx675[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32RegAddr,
				g_pastImx675[ViPipe]->astSyncInfo[0].snsCfg.astI2cData[i].u32Data);
	}
}

void imx675_mirror_flip(VI_PIPE ViPipe, ISP_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip)
{
	CVI_U8 flip, mirror;

	flip = imx675_read_register(ViPipe, 0x3021);
	mirror = imx675_read_register(ViPipe, 0x3020);

	flip &= ~0x1;
	mirror &= ~0x1;

	switch (eSnsMirrorFlip) {
	case ISP_SNS_NORMAL:
		break;
	case ISP_SNS_MIRROR:
		mirror |= 0x1;
		break;
	case ISP_SNS_FLIP:
		flip |= 0x1;
		break;
	case ISP_SNS_MIRROR_FLIP:
		mirror |= 0x1;
		flip |= 0x1;
		break;
	default:
		return;
	}

	imx675_write_register(ViPipe, 0x3021, flip);
	imx675_write_register(ViPipe, 0x3020, mirror);
}

#define IMX675_CHIP_ID_ADDR_1	0x4d13
#define IMX675_CHIP_ID_ADDR_2	0x4d12
#define IMX675_CHIP_ID	        0x576

int imx675_probe(VI_PIPE ViPipe)
{
	int nVal_1;
	int nVal_2;

	delay_ms(1); //waitting i2c stable
	if (imx675_i2c_init(ViPipe) != CVI_SUCCESS)
		return CVI_FAILURE;

	imx675_write_register(ViPipe, 0x3000, 0x00);
	delay_ms(80); //registers become accessible after waiting 80 ms after standby cancel (STANDBY= 0)

	nVal_1 = imx675_read_register(ViPipe, IMX675_CHIP_ID_ADDR_1);
	nVal_2 = imx675_read_register(ViPipe, IMX675_CHIP_ID_ADDR_2);

	if (nVal_1 < 0 || nVal_2 < 0) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "read sensor id error\n");
		return nVal_1;
	}

	if ((((nVal_1 & 0xFF) << 8) | (nVal_2 & 0xFF)) != IMX675_CHIP_ID) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Sensor ID Mismatch! Use the wrong sensor??\n");
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

void imx675_init(VI_PIPE ViPipe)
{
	WDR_MODE_E       enWDRMode;
	CVI_U8            u8ImgMode;

	enWDRMode   = g_pastImx675[ViPipe]->enWDRMode;
	u8ImgMode   = g_pastImx675[ViPipe]->u8ImgMode;

	delay_ms(1); //waitting i2c stable
	if (imx675_i2c_init(ViPipe) != CVI_SUCCESS) {
		return ;
	}

	if (enWDRMode == WDR_MODE_2To1_LINE) {
		if (u8ImgMode == IMX675_MODE_5M25_WDR) {
			imx675_wdr_5M25_2to1_init(ViPipe);
		}
	} else {
		imx675_linear_5M30_init(ViPipe);
	}
	g_pastImx675[ViPipe]->bInit = CVI_TRUE;
}

void imx675_exit(VI_PIPE ViPipe)
{
	imx675_i2c_exit(ViPipe);
}

static void imx675_linear_5M30_init(VI_PIPE ViPipe)
{
	delay_ms(4);

	imx675_write_register(ViPipe, 0x3000, 0x01); //STANDBY
	imx675_write_register(ViPipe, 0x3001, 0x00); //REGHOLD
	imx675_write_register(ViPipe, 0x3002, 0x01); //XMSTA
	imx675_write_register(ViPipe, 0x3014, 0x03); //INCK_SEL[3:0] 3:27 1:37.125
	imx675_write_register(ViPipe, 0x3015, 0x04); //DATARATE_SEL[3:0]
	imx675_write_register(ViPipe, 0x3018, 0x00); //WINMODE[3:0]
	imx675_write_register(ViPipe, 0x3019, 0x00); //CFMODE
	imx675_write_register(ViPipe, 0x301A, 0x00); //WDMODE[7:0]
	imx675_write_register(ViPipe, 0x301B, 0x00); //ADDMODE[1:0]
	imx675_write_register(ViPipe, 0x301C, 0x00); //THIN_V_EN[7:0]
	imx675_write_register(ViPipe, 0x301E, 0x01); //VCMODE[7:0]
	imx675_write_register(ViPipe, 0x3020, 0x00); //HREVERSE
	imx675_write_register(ViPipe, 0x3021, 0x00); //VREVERSE
	imx675_write_register(ViPipe, 0x3022, 0x01); //ADBIT[1:0]
	imx675_write_register(ViPipe, 0x3023, 0x01); //MDBIT
	imx675_write_register(ViPipe, 0x3028, 0x52); //VMAX[19:0]
	imx675_write_register(ViPipe, 0x3029, 0x0E); //VMAX[19:0]
	imx675_write_register(ViPipe, 0x302A, 0x00); //VMAX[19:0]
	imx675_write_register(ViPipe, 0x302C, 0xA3); //HMAX[15:0]
	imx675_write_register(ViPipe, 0x302D, 0x02); //HMAX[15:0]
	imx675_write_register(ViPipe, 0x3030, 0x00); //FDG_SEL0[1:0]
	imx675_write_register(ViPipe, 0x3031, 0x00); //FDG_SEL1[1:0]
	imx675_write_register(ViPipe, 0x3032, 0x00); //FDG_SEL2[1:0]
	imx675_write_register(ViPipe, 0x303C, 0x00); //PIX_HST[12:0]
	imx675_write_register(ViPipe, 0x303D, 0x00); //PIX_HST[12:0]
	imx675_write_register(ViPipe, 0x303E, 0x30); //PIX_HWIDTH[12:0]
	imx675_write_register(ViPipe, 0x303F, 0x0A); //PIX_HWIDTH[12:0]
	imx675_write_register(ViPipe, 0x3040, 0x03); //LANEMODE[2:0]
	imx675_write_register(ViPipe, 0x3044, 0x00); //PIX_VST[11:0]
	imx675_write_register(ViPipe, 0x3045, 0x00); //PIX_VST[11:0]
	imx675_write_register(ViPipe, 0x3046, 0xAC); //PIX_VWIDTH[11:0]
	imx675_write_register(ViPipe, 0x3047, 0x07); //PIX_VWIDTH[11:0]
	imx675_write_register(ViPipe, 0x304C, 0x00); //GAIN_HG0[10:0]
	imx675_write_register(ViPipe, 0x304D, 0x00); //GAIN_HG0[10:0]
	imx675_write_register(ViPipe, 0x3050, 0x04); //SHR0[19:0]
	imx675_write_register(ViPipe, 0x3051, 0x00); //SHR0[19:0]
	imx675_write_register(ViPipe, 0x3052, 0x00); //SHR0[19:0]
	imx675_write_register(ViPipe, 0x3054, 0x93); //SHR1[19:0]
	imx675_write_register(ViPipe, 0x3055, 0x00); //SHR1[19:0]
	imx675_write_register(ViPipe, 0x3056, 0x00); //SHR1[19:0]
	imx675_write_register(ViPipe, 0x3058, 0x53); //SHR2[19:0]
	imx675_write_register(ViPipe, 0x3059, 0x00); //SHR2[19:0]
	imx675_write_register(ViPipe, 0x305A, 0x00); //SHR2[19:0]
	imx675_write_register(ViPipe, 0x3060, 0x95); //RHS1[19:0]
	imx675_write_register(ViPipe, 0x3061, 0x00); //RHS1[19:0]
	imx675_write_register(ViPipe, 0x3062, 0x00); //RHS1[19:0]
	imx675_write_register(ViPipe, 0x3064, 0x56); //RHS2[19:0]
	imx675_write_register(ViPipe, 0x3065, 0x00); //RHS2[19:0]
	imx675_write_register(ViPipe, 0x3066, 0x00); //RHS2[19:0]
	imx675_write_register(ViPipe, 0x3070, 0x00); //GAIN_0[10:0]
	imx675_write_register(ViPipe, 0x3071, 0x00); //GAIN_0[10:0]
	imx675_write_register(ViPipe, 0x3072, 0x00); //GAIN_1[10:0]
	imx675_write_register(ViPipe, 0x3073, 0x00); //GAIN_1[10:0]
	imx675_write_register(ViPipe, 0x3074, 0x00); //GAIN_2[10:0]
	imx675_write_register(ViPipe, 0x3075, 0x00); //GAIN_2[10:0]
	imx675_write_register(ViPipe, 0x30A4, 0xAA); //XVSOUTSEL[1:0]
	imx675_write_register(ViPipe, 0x30A6, 0x00); //XVS_DRV[1:0]
	imx675_write_register(ViPipe, 0x30CC, 0x00);
	imx675_write_register(ViPipe, 0x30CD, 0x00);
	imx675_write_register(ViPipe, 0x30CE, 0x02);
	imx675_write_register(ViPipe, 0x30DC, 0x32); //BLKLEVEL[9:0]
	imx675_write_register(ViPipe, 0x30DD, 0x40); //BLKLEVEL[9:0]
	imx675_write_register(ViPipe, 0x310C, 0x01);
	imx675_write_register(ViPipe, 0x3130, 0x01);
	imx675_write_register(ViPipe, 0x3148, 0x00);
	imx675_write_register(ViPipe, 0x315E, 0x10);
	imx675_write_register(ViPipe, 0x3400, 0x01); //GAIN_PGC_FIDMD
	imx675_write_register(ViPipe, 0x3460, 0x22);
	imx675_write_register(ViPipe, 0x347B, 0x02);
	imx675_write_register(ViPipe, 0x3492, 0x08);
	imx675_write_register(ViPipe, 0x3890, 0x08); //HFR_EN[3:0]
	imx675_write_register(ViPipe, 0x3891, 0x00); //HFR_EN[3:0]
	imx675_write_register(ViPipe, 0x3893, 0x00);
	imx675_write_register(ViPipe, 0x3B1D, 0x17);
	imx675_write_register(ViPipe, 0x3B44, 0x3F);
	imx675_write_register(ViPipe, 0x3B60, 0x03);
	imx675_write_register(ViPipe, 0x3C03, 0x04);
	imx675_write_register(ViPipe, 0x3C04, 0x04);
	imx675_write_register(ViPipe, 0x3C0A, 0x1F);
	imx675_write_register(ViPipe, 0x3C0B, 0x1F);
	imx675_write_register(ViPipe, 0x3C0C, 0x1F);
	imx675_write_register(ViPipe, 0x3C0D, 0x1F);
	imx675_write_register(ViPipe, 0x3C0E, 0x1F);
	imx675_write_register(ViPipe, 0x3C0F, 0x1F);
	imx675_write_register(ViPipe, 0x3C30, 0x73);
	imx675_write_register(ViPipe, 0x3C3C, 0x20);
	imx675_write_register(ViPipe, 0x3C44, 0x06);
	imx675_write_register(ViPipe, 0x3C7C, 0xB9);
	imx675_write_register(ViPipe, 0x3C7D, 0x01);
	imx675_write_register(ViPipe, 0x3C7E, 0xB7);
	imx675_write_register(ViPipe, 0x3C7F, 0x01);
	imx675_write_register(ViPipe, 0x3CB0, 0x00);
	imx675_write_register(ViPipe, 0x3CB2, 0xFF);
	imx675_write_register(ViPipe, 0x3CB3, 0x03);
	imx675_write_register(ViPipe, 0x3CB4, 0xFF);
	imx675_write_register(ViPipe, 0x3CB5, 0x03);
	imx675_write_register(ViPipe, 0x3CBA, 0xFF);
	imx675_write_register(ViPipe, 0x3CBB, 0x03);
	imx675_write_register(ViPipe, 0x3CC0, 0xFF);
	imx675_write_register(ViPipe, 0x3CC1, 0x03);
	imx675_write_register(ViPipe, 0x3CC2, 0x00);
	imx675_write_register(ViPipe, 0x3CC6, 0xFF);
	imx675_write_register(ViPipe, 0x3CC7, 0x03);
	imx675_write_register(ViPipe, 0x3CC8, 0xFF);
	imx675_write_register(ViPipe, 0x3CC9, 0x03);
	imx675_write_register(ViPipe, 0x3E00, 0x1E);
	imx675_write_register(ViPipe, 0x3E02, 0x04);
	imx675_write_register(ViPipe, 0x3E03, 0x00);
	imx675_write_register(ViPipe, 0x3E20, 0x04);
	imx675_write_register(ViPipe, 0x3E21, 0x00);
	imx675_write_register(ViPipe, 0x3E22, 0x1E);
	imx675_write_register(ViPipe, 0x3E24, 0xBA);
	imx675_write_register(ViPipe, 0x3E72, 0x85);
	imx675_write_register(ViPipe, 0x3E76, 0x0C);
	imx675_write_register(ViPipe, 0x3E77, 0x01);
	imx675_write_register(ViPipe, 0x3E7A, 0x85);
	imx675_write_register(ViPipe, 0x3E7E, 0x1F);
	imx675_write_register(ViPipe, 0x3E82, 0xA6);
	imx675_write_register(ViPipe, 0x3E86, 0x2D);
	imx675_write_register(ViPipe, 0x3EE2, 0x33);
	imx675_write_register(ViPipe, 0x3EE3, 0x03);
	imx675_write_register(ViPipe, 0x4490, 0x07);
	imx675_write_register(ViPipe, 0x4494, 0x19);
	imx675_write_register(ViPipe, 0x4495, 0x00);
	imx675_write_register(ViPipe, 0x4496, 0xBB);
	imx675_write_register(ViPipe, 0x4497, 0x00);
	imx675_write_register(ViPipe, 0x4498, 0x55);
	imx675_write_register(ViPipe, 0x449A, 0x50);
	imx675_write_register(ViPipe, 0x449C, 0x50);
	imx675_write_register(ViPipe, 0x449E, 0x50);
	imx675_write_register(ViPipe, 0x44A0, 0x3C);
	imx675_write_register(ViPipe, 0x44A2, 0x19);
	imx675_write_register(ViPipe, 0x44A4, 0x19);
	imx675_write_register(ViPipe, 0x44A6, 0x19);
	imx675_write_register(ViPipe, 0x44A8, 0x4B);
	imx675_write_register(ViPipe, 0x44AA, 0x4B);
	imx675_write_register(ViPipe, 0x44AC, 0x4B);
	imx675_write_register(ViPipe, 0x44AE, 0x4B);
	imx675_write_register(ViPipe, 0x44B0, 0x3C);
	imx675_write_register(ViPipe, 0x44B2, 0x19);
	imx675_write_register(ViPipe, 0x44B4, 0x19);
	imx675_write_register(ViPipe, 0x44B6, 0x19);
	imx675_write_register(ViPipe, 0x44B8, 0x4B);
	imx675_write_register(ViPipe, 0x44BA, 0x4B);
	imx675_write_register(ViPipe, 0x44BC, 0x4B);
	imx675_write_register(ViPipe, 0x44BE, 0x4B);
	imx675_write_register(ViPipe, 0x44C0, 0x3C);
	imx675_write_register(ViPipe, 0x44C2, 0x19);
	imx675_write_register(ViPipe, 0x44C4, 0x19);
	imx675_write_register(ViPipe, 0x44C6, 0x19);
	imx675_write_register(ViPipe, 0x44C8, 0xF0);
	imx675_write_register(ViPipe, 0x44CA, 0xEB);
	imx675_write_register(ViPipe, 0x44CC, 0xEB);
	imx675_write_register(ViPipe, 0x44CE, 0xE6);
	imx675_write_register(ViPipe, 0x44D0, 0xE6);
	imx675_write_register(ViPipe, 0x44D2, 0xBB);
	imx675_write_register(ViPipe, 0x44D4, 0xBB);
	imx675_write_register(ViPipe, 0x44D6, 0xBB);
	imx675_write_register(ViPipe, 0x44D8, 0xE6);
	imx675_write_register(ViPipe, 0x44DA, 0xE6);
	imx675_write_register(ViPipe, 0x44DC, 0xE6);
	imx675_write_register(ViPipe, 0x44DE, 0xE6);
	imx675_write_register(ViPipe, 0x44E0, 0xE6);
	imx675_write_register(ViPipe, 0x44E2, 0xBB);
	imx675_write_register(ViPipe, 0x44E4, 0xBB);
	imx675_write_register(ViPipe, 0x44E6, 0xBB);
	imx675_write_register(ViPipe, 0x44E8, 0xE6);
	imx675_write_register(ViPipe, 0x44EA, 0xE6);
	imx675_write_register(ViPipe, 0x44EC, 0xE6);
	imx675_write_register(ViPipe, 0x44EE, 0xE6);
	imx675_write_register(ViPipe, 0x44F0, 0xE6);
	imx675_write_register(ViPipe, 0x44F2, 0xBB);
	imx675_write_register(ViPipe, 0x44F4, 0xBB);
	imx675_write_register(ViPipe, 0x44F6, 0xBB);
	imx675_write_register(ViPipe, 0x4538, 0x15);
	imx675_write_register(ViPipe, 0x4539, 0x15);
	imx675_write_register(ViPipe, 0x453A, 0x15);
	imx675_write_register(ViPipe, 0x4544, 0x15);
	imx675_write_register(ViPipe, 0x4545, 0x15);
	imx675_write_register(ViPipe, 0x4546, 0x15);
	imx675_write_register(ViPipe, 0x4550, 0x10);
	imx675_write_register(ViPipe, 0x4551, 0x10);
	imx675_write_register(ViPipe, 0x4552, 0x10);
	imx675_write_register(ViPipe, 0x4553, 0x10);
	imx675_write_register(ViPipe, 0x4554, 0x10);
	imx675_write_register(ViPipe, 0x4555, 0x10);
	imx675_write_register(ViPipe, 0x4556, 0x10);
	imx675_write_register(ViPipe, 0x4557, 0x10);
	imx675_write_register(ViPipe, 0x4558, 0x10);
	imx675_write_register(ViPipe, 0x455C, 0x10);
	imx675_write_register(ViPipe, 0x455D, 0x10);
	imx675_write_register(ViPipe, 0x455E, 0x10);
	imx675_write_register(ViPipe, 0x455F, 0x10);
	imx675_write_register(ViPipe, 0x4560, 0x10);
	imx675_write_register(ViPipe, 0x4561, 0x10);
	imx675_write_register(ViPipe, 0x4562, 0x10);
	imx675_write_register(ViPipe, 0x4563, 0x10);
	imx675_write_register(ViPipe, 0x4564, 0x10);
	imx675_write_register(ViPipe, 0x4569, 0x01);
	imx675_write_register(ViPipe, 0x456A, 0x01);
	imx675_write_register(ViPipe, 0x456B, 0x06);
	imx675_write_register(ViPipe, 0x456C, 0x06);
	imx675_write_register(ViPipe, 0x456D, 0x06);
	imx675_write_register(ViPipe, 0x456E, 0x06);
	imx675_write_register(ViPipe, 0x456F, 0x06);
	imx675_write_register(ViPipe, 0x4570, 0x06);

	imx675_default_reg_init(ViPipe);

	imx675_write_register(ViPipe, 0x3000, 0x00); /* standby */
	delay_ms(80);

	imx675_write_register(ViPipe, 0x3002, 0x00); /* master mode start */

	printf("ViPipe:%d,===IMX675 5M30fps 12bit LINE Init OK!===\n", ViPipe);
}

//25fps 1440Mbps vmax 2200
static void imx675_wdr_5M25_2to1_init(VI_PIPE ViPipe)
{
	delay_ms(4);

	imx675_write_register(ViPipe, 0x3000, 0x01); //STANDBY
	imx675_write_register(ViPipe, 0x3001, 0x00); //REGHOLD
	imx675_write_register(ViPipe, 0x3002, 0x01); //XMSTA
	imx675_write_register(ViPipe, 0x3014, 0x03); //INCK_SEL[3:0]
	imx675_write_register(ViPipe, 0x3015, 0x03); //DATARATE_SEL[3:0]
	imx675_write_register(ViPipe, 0x3018, 0x00); //WINMODE[3:0]
	imx675_write_register(ViPipe, 0x3019, 0x00); //CFMODE
	imx675_write_register(ViPipe, 0x301A, 0x01); //WDMODE[7:0]
	imx675_write_register(ViPipe, 0x301B, 0x00); //ADDMODE[1:0]
	imx675_write_register(ViPipe, 0x301C, 0x01); //THIN_V_EN[7:0]
	imx675_write_register(ViPipe, 0x301E, 0x01); //VCMODE[7:0]
	imx675_write_register(ViPipe, 0x3020, 0x00); //HREVERSE
	imx675_write_register(ViPipe, 0x3021, 0x00); //VREVERSE
	imx675_write_register(ViPipe, 0x3022, 0x01); //ADBIT[1:0]
	imx675_write_register(ViPipe, 0x3023, 0x01); //MDBIT
	imx675_write_register(ViPipe, 0x3028, 0x98); //VMAX[19:0]
	imx675_write_register(ViPipe, 0x3029, 0x08); //VMAX[19:0]
	imx675_write_register(ViPipe, 0x302A, 0x00); //VMAX[19:0]
	imx675_write_register(ViPipe, 0x302C, 0xA3); //HMAX[15:0]
	imx675_write_register(ViPipe, 0x302D, 0x02); //HMAX[15:0]
	imx675_write_register(ViPipe, 0x3030, 0x00); //FDG_SEL0[1:0]
	imx675_write_register(ViPipe, 0x3031, 0x00); //FDG_SEL1[1:0]
	imx675_write_register(ViPipe, 0x3032, 0x00); //FDG_SEL2[1:0]
	imx675_write_register(ViPipe, 0x303C, 0x00); //PIX_HST[12:0]
	imx675_write_register(ViPipe, 0x303D, 0x00); //PIX_HST[12:0]
	imx675_write_register(ViPipe, 0x303E, 0x30); //PIX_HWIDTH[12:0]
	imx675_write_register(ViPipe, 0x303F, 0x0A); //PIX_HWIDTH[12:0]
	imx675_write_register(ViPipe, 0x3040, 0x03); //LANEMODE[2:0]
	imx675_write_register(ViPipe, 0x3044, 0x00); //PIX_VST[11:0]
	imx675_write_register(ViPipe, 0x3045, 0x00); //PIX_VST[11:0]
	imx675_write_register(ViPipe, 0x3046, 0xAC); //PIX_VWIDTH[11:0]
	imx675_write_register(ViPipe, 0x3047, 0x07); //PIX_VWIDTH[11:0]
	imx675_write_register(ViPipe, 0x304C, 0x00); //GAIN_HG0[10:0]
	imx675_write_register(ViPipe, 0x304D, 0x00); //GAIN_HG0[10:0]
	imx675_write_register(ViPipe, 0x3050, 0x50); //SHR0[19:0]
	imx675_write_register(ViPipe, 0x3051, 0x0A); //SHR0[19:0]
	imx675_write_register(ViPipe, 0x3052, 0x00); //SHR0[19:0]
	imx675_write_register(ViPipe, 0x3054, 0x05); //SHR1[19:0]
	imx675_write_register(ViPipe, 0x3055, 0x00); //SHR1[19:0]
	imx675_write_register(ViPipe, 0x3056, 0x00); //SHR1[19:0]
	imx675_write_register(ViPipe, 0x3058, 0x53); //SHR2[19:0]
	imx675_write_register(ViPipe, 0x3059, 0x00); //SHR2[19:0]
	imx675_write_register(ViPipe, 0x305A, 0x00); //SHR2[19:0]
	imx675_write_register(ViPipe, 0x3060, 0x75); //RHS1[19:0]
	imx675_write_register(ViPipe, 0x3061, 0x00); //RHS1[19:0]
	imx675_write_register(ViPipe, 0x3062, 0x00); //RHS1[19:0]
	imx675_write_register(ViPipe, 0x3064, 0x56); //RHS2[19:0]
	imx675_write_register(ViPipe, 0x3065, 0x00); //RHS2[19:0]
	imx675_write_register(ViPipe, 0x3066, 0x00); //RHS2[19:0]
	imx675_write_register(ViPipe, 0x3070, 0x00); //GAIN_0[10:0]
	imx675_write_register(ViPipe, 0x3071, 0x00); //GAIN_0[10:0]
	imx675_write_register(ViPipe, 0x3072, 0x00); //GAIN_1[10:0]
	imx675_write_register(ViPipe, 0x3073, 0x00); //GAIN_1[10:0]
	imx675_write_register(ViPipe, 0x3074, 0x00); //GAIN_2[10:0]
	imx675_write_register(ViPipe, 0x3075, 0x00); //GAIN_2[10:0]
	imx675_write_register(ViPipe, 0x30A4, 0xAA); //XVSOUTSEL[1:0]
	imx675_write_register(ViPipe, 0x30A6, 0x00); //XVS_DRV[1:0]
	imx675_write_register(ViPipe, 0x30CC, 0x00);
	imx675_write_register(ViPipe, 0x30CD, 0x00);
	imx675_write_register(ViPipe, 0x30CE, 0x02);
	imx675_write_register(ViPipe, 0x30DC, 0x32); //BLKLEVEL[9:0]
	imx675_write_register(ViPipe, 0x30DD, 0x40); //BLKLEVEL[9:0]
	imx675_write_register(ViPipe, 0x310C, 0x01);
	imx675_write_register(ViPipe, 0x3130, 0x01);
	imx675_write_register(ViPipe, 0x3148, 0x00);
	imx675_write_register(ViPipe, 0x315E, 0x10);
	imx675_write_register(ViPipe, 0x3400, 0x01); //GAIN_PGC_FIDMD
	imx675_write_register(ViPipe, 0x3460, 0x22);
	imx675_write_register(ViPipe, 0x347B, 0x02);
	imx675_write_register(ViPipe, 0x3492, 0x08);
	imx675_write_register(ViPipe, 0x3890, 0x08); //HFR_EN[3:0]
	imx675_write_register(ViPipe, 0x3891, 0x00); //HFR_EN[3:0]
	imx675_write_register(ViPipe, 0x3893, 0x00);
	imx675_write_register(ViPipe, 0x3B1D, 0x17);
	imx675_write_register(ViPipe, 0x3B44, 0x3F);
	imx675_write_register(ViPipe, 0x3B60, 0x03);
	imx675_write_register(ViPipe, 0x3C03, 0x04);
	imx675_write_register(ViPipe, 0x3C04, 0x04);
	imx675_write_register(ViPipe, 0x3C0A, 0x1F);
	imx675_write_register(ViPipe, 0x3C0B, 0x1F);
	imx675_write_register(ViPipe, 0x3C0C, 0x1F);
	imx675_write_register(ViPipe, 0x3C0D, 0x1F);
	imx675_write_register(ViPipe, 0x3C0E, 0x1F);
	imx675_write_register(ViPipe, 0x3C0F, 0x1F);
	imx675_write_register(ViPipe, 0x3C30, 0x73);
	imx675_write_register(ViPipe, 0x3C3C, 0x20);
	imx675_write_register(ViPipe, 0x3C44, 0x06);
	imx675_write_register(ViPipe, 0x3C7C, 0xB9);
	imx675_write_register(ViPipe, 0x3C7D, 0x01);
	imx675_write_register(ViPipe, 0x3C7E, 0xB7);
	imx675_write_register(ViPipe, 0x3C7F, 0x01);
	imx675_write_register(ViPipe, 0x3CB0, 0x00);
	imx675_write_register(ViPipe, 0x3CB2, 0xFF);
	imx675_write_register(ViPipe, 0x3CB3, 0x03);
	imx675_write_register(ViPipe, 0x3CB4, 0xFF);
	imx675_write_register(ViPipe, 0x3CB5, 0x03);
	imx675_write_register(ViPipe, 0x3CBA, 0xFF);
	imx675_write_register(ViPipe, 0x3CBB, 0x03);
	imx675_write_register(ViPipe, 0x3CC0, 0xFF);
	imx675_write_register(ViPipe, 0x3CC1, 0x03);
	imx675_write_register(ViPipe, 0x3CC2, 0x00);
	imx675_write_register(ViPipe, 0x3CC6, 0xFF);
	imx675_write_register(ViPipe, 0x3CC7, 0x03);
	imx675_write_register(ViPipe, 0x3CC8, 0xFF);
	imx675_write_register(ViPipe, 0x3CC9, 0x03);
	imx675_write_register(ViPipe, 0x3E00, 0x1E);
	imx675_write_register(ViPipe, 0x3E02, 0x04);
	imx675_write_register(ViPipe, 0x3E03, 0x00);
	imx675_write_register(ViPipe, 0x3E20, 0x04);
	imx675_write_register(ViPipe, 0x3E21, 0x00);
	imx675_write_register(ViPipe, 0x3E22, 0x1E);
	imx675_write_register(ViPipe, 0x3E24, 0xBA);
	imx675_write_register(ViPipe, 0x3E72, 0x85);
	imx675_write_register(ViPipe, 0x3E76, 0x0C);
	imx675_write_register(ViPipe, 0x3E77, 0x01);
	imx675_write_register(ViPipe, 0x3E7A, 0x85);
	imx675_write_register(ViPipe, 0x3E7E, 0x1F);
	imx675_write_register(ViPipe, 0x3E82, 0xA6);
	imx675_write_register(ViPipe, 0x3E86, 0x2D);
	imx675_write_register(ViPipe, 0x3EE2, 0x33);
	imx675_write_register(ViPipe, 0x3EE3, 0x03);
	imx675_write_register(ViPipe, 0x4490, 0x07);
	imx675_write_register(ViPipe, 0x4494, 0x19);
	imx675_write_register(ViPipe, 0x4495, 0x00);
	imx675_write_register(ViPipe, 0x4496, 0xBB);
	imx675_write_register(ViPipe, 0x4497, 0x00);
	imx675_write_register(ViPipe, 0x4498, 0x55);
	imx675_write_register(ViPipe, 0x449A, 0x50);
	imx675_write_register(ViPipe, 0x449C, 0x50);
	imx675_write_register(ViPipe, 0x449E, 0x50);
	imx675_write_register(ViPipe, 0x44A0, 0x3C);
	imx675_write_register(ViPipe, 0x44A2, 0x19);
	imx675_write_register(ViPipe, 0x44A4, 0x19);
	imx675_write_register(ViPipe, 0x44A6, 0x19);
	imx675_write_register(ViPipe, 0x44A8, 0x4B);
	imx675_write_register(ViPipe, 0x44AA, 0x4B);
	imx675_write_register(ViPipe, 0x44AC, 0x4B);
	imx675_write_register(ViPipe, 0x44AE, 0x4B);
	imx675_write_register(ViPipe, 0x44B0, 0x3C);
	imx675_write_register(ViPipe, 0x44B2, 0x19);
	imx675_write_register(ViPipe, 0x44B4, 0x19);
	imx675_write_register(ViPipe, 0x44B6, 0x19);
	imx675_write_register(ViPipe, 0x44B8, 0x4B);
	imx675_write_register(ViPipe, 0x44BA, 0x4B);
	imx675_write_register(ViPipe, 0x44BC, 0x4B);
	imx675_write_register(ViPipe, 0x44BE, 0x4B);
	imx675_write_register(ViPipe, 0x44C0, 0x3C);
	imx675_write_register(ViPipe, 0x44C2, 0x19);
	imx675_write_register(ViPipe, 0x44C4, 0x19);
	imx675_write_register(ViPipe, 0x44C6, 0x19);
	imx675_write_register(ViPipe, 0x44C8, 0xF0);
	imx675_write_register(ViPipe, 0x44CA, 0xEB);
	imx675_write_register(ViPipe, 0x44CC, 0xEB);
	imx675_write_register(ViPipe, 0x44CE, 0xE6);
	imx675_write_register(ViPipe, 0x44D0, 0xE6);
	imx675_write_register(ViPipe, 0x44D2, 0xBB);
	imx675_write_register(ViPipe, 0x44D4, 0xBB);
	imx675_write_register(ViPipe, 0x44D6, 0xBB);
	imx675_write_register(ViPipe, 0x44D8, 0xE6);
	imx675_write_register(ViPipe, 0x44DA, 0xE6);
	imx675_write_register(ViPipe, 0x44DC, 0xE6);
	imx675_write_register(ViPipe, 0x44DE, 0xE6);
	imx675_write_register(ViPipe, 0x44E0, 0xE6);
	imx675_write_register(ViPipe, 0x44E2, 0xBB);
	imx675_write_register(ViPipe, 0x44E4, 0xBB);
	imx675_write_register(ViPipe, 0x44E6, 0xBB);
	imx675_write_register(ViPipe, 0x44E8, 0xE6);
	imx675_write_register(ViPipe, 0x44EA, 0xE6);
	imx675_write_register(ViPipe, 0x44EC, 0xE6);
	imx675_write_register(ViPipe, 0x44EE, 0xE6);
	imx675_write_register(ViPipe, 0x44F0, 0xE6);
	imx675_write_register(ViPipe, 0x44F2, 0xBB);
	imx675_write_register(ViPipe, 0x44F4, 0xBB);
	imx675_write_register(ViPipe, 0x44F6, 0xBB);
	imx675_write_register(ViPipe, 0x4538, 0x15);
	imx675_write_register(ViPipe, 0x4539, 0x15);
	imx675_write_register(ViPipe, 0x453A, 0x15);
	imx675_write_register(ViPipe, 0x4544, 0x15);
	imx675_write_register(ViPipe, 0x4545, 0x15);
	imx675_write_register(ViPipe, 0x4546, 0x15);
	imx675_write_register(ViPipe, 0x4550, 0x10);
	imx675_write_register(ViPipe, 0x4551, 0x10);
	imx675_write_register(ViPipe, 0x4552, 0x10);
	imx675_write_register(ViPipe, 0x4553, 0x10);
	imx675_write_register(ViPipe, 0x4554, 0x10);
	imx675_write_register(ViPipe, 0x4555, 0x10);
	imx675_write_register(ViPipe, 0x4556, 0x10);
	imx675_write_register(ViPipe, 0x4557, 0x10);
	imx675_write_register(ViPipe, 0x4558, 0x10);
	imx675_write_register(ViPipe, 0x455C, 0x10);
	imx675_write_register(ViPipe, 0x455D, 0x10);
	imx675_write_register(ViPipe, 0x455E, 0x10);
	imx675_write_register(ViPipe, 0x455F, 0x10);
	imx675_write_register(ViPipe, 0x4560, 0x10);
	imx675_write_register(ViPipe, 0x4561, 0x10);
	imx675_write_register(ViPipe, 0x4562, 0x10);
	imx675_write_register(ViPipe, 0x4563, 0x10);
	imx675_write_register(ViPipe, 0x4564, 0x10);
	imx675_write_register(ViPipe, 0x4569, 0x01);
	imx675_write_register(ViPipe, 0x456A, 0x01);
	imx675_write_register(ViPipe, 0x456B, 0x06);
	imx675_write_register(ViPipe, 0x456C, 0x06);
	imx675_write_register(ViPipe, 0x456D, 0x06);
	imx675_write_register(ViPipe, 0x456E, 0x06);
	imx675_write_register(ViPipe, 0x456F, 0x06);
	imx675_write_register(ViPipe, 0x4570, 0x06);

	imx675_default_reg_init(ViPipe);

	imx675_write_register(ViPipe, 0x3000, 0x00); /* standby */
	delay_ms(80);
	imx675_write_register(ViPipe, 0x3002, 0x00); /* master mode start */

	printf("ViPipe:%d,===Imx675 5M25fps 12bit 2to1 WDR Init Ok!====\n", ViPipe);
}