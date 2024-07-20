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
#include <linux/vi_snsr.h>
#include <linux/cvi_comm_video.h>
#endif
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

	imx675_write_register(ViPipe, 0X3000, 0X01); //STANDBY
	imx675_write_register(ViPipe, 0X3001, 0X00); //REGHOLD
	imx675_write_register(ViPipe, 0X3002, 0X01); //XMSTA
	imx675_write_register(ViPipe, 0X3014, 0X03); //INCK_SEL[3:0] 3:27 1:37.125
	imx675_write_register(ViPipe, 0X3015, 0X04); //DATARATE_SEL[3:0]
	imx675_write_register(ViPipe, 0X3018, 0X00); //WINMODE[3:0]
	imx675_write_register(ViPipe, 0X3019, 0X00); //CFMODE
	imx675_write_register(ViPipe, 0X301A, 0X00); //WDMODE[7:0]
	imx675_write_register(ViPipe, 0X301B, 0X00); //ADDMODE[1:0]
	imx675_write_register(ViPipe, 0X301C, 0X00); //THIN_V_EN[7:0]
	imx675_write_register(ViPipe, 0X301E, 0X01); //VCMODE[7:0]
	imx675_write_register(ViPipe, 0X3020, 0X00); //HREVERSE
	imx675_write_register(ViPipe, 0X3021, 0X00); //VREVERSE
	imx675_write_register(ViPipe, 0X3022, 0X01); //ADBIT[1:0]
	imx675_write_register(ViPipe, 0X3023, 0X01); //MDBIT
	imx675_write_register(ViPipe, 0X3028, 0X52); //VMAX[19:0]
	imx675_write_register(ViPipe, 0X3029, 0X0E); //VMAX[19:0]
	imx675_write_register(ViPipe, 0X302A, 0X00); //VMAX[19:0]
	imx675_write_register(ViPipe, 0X302C, 0XA3); //HMAX[15:0]
	imx675_write_register(ViPipe, 0X302D, 0X02); //HMAX[15:0]
	imx675_write_register(ViPipe, 0X3030, 0X00); //FDG_SEL0[1:0]
	imx675_write_register(ViPipe, 0X3031, 0X00); //FDG_SEL1[1:0]
	imx675_write_register(ViPipe, 0X3032, 0X00); //FDG_SEL2[1:0]
	imx675_write_register(ViPipe, 0X303C, 0X00); //PIX_HST[12:0]
	imx675_write_register(ViPipe, 0X303D, 0X00); //PIX_HST[12:0]
	imx675_write_register(ViPipe, 0X303E, 0X30); //PIX_HWIDTH[12:0]
	imx675_write_register(ViPipe, 0X303F, 0X0A); //PIX_HWIDTH[12:0]
	imx675_write_register(ViPipe, 0X3040, 0X03); //LANEMODE[2:0]
	imx675_write_register(ViPipe, 0X3044, 0X00); //PIX_VST[11:0]
	imx675_write_register(ViPipe, 0X3045, 0X00); //PIX_VST[11:0]
	imx675_write_register(ViPipe, 0X3046, 0XAC); //PIX_VWIDTH[11:0]
	imx675_write_register(ViPipe, 0X3047, 0X07); //PIX_VWIDTH[11:0]
	imx675_write_register(ViPipe, 0X304C, 0X00); //GAIN_HG0[10:0]
	imx675_write_register(ViPipe, 0X304D, 0X00); //GAIN_HG0[10:0]
	imx675_write_register(ViPipe, 0X3050, 0X04); //SHR0[19:0]
	imx675_write_register(ViPipe, 0X3051, 0X00); //SHR0[19:0]
	imx675_write_register(ViPipe, 0X3052, 0X00); //SHR0[19:0]
	imx675_write_register(ViPipe, 0X3054, 0X93); //SHR1[19:0]
	imx675_write_register(ViPipe, 0X3055, 0X00); //SHR1[19:0]
	imx675_write_register(ViPipe, 0X3056, 0X00); //SHR1[19:0]
	imx675_write_register(ViPipe, 0X3058, 0X53); //SHR2[19:0]
	imx675_write_register(ViPipe, 0X3059, 0X00); //SHR2[19:0]
	imx675_write_register(ViPipe, 0X305A, 0X00); //SHR2[19:0]
	imx675_write_register(ViPipe, 0X3060, 0X95); //RHS1[19:0]
	imx675_write_register(ViPipe, 0X3061, 0X00); //RHS1[19:0]
	imx675_write_register(ViPipe, 0X3062, 0X00); //RHS1[19:0]
	imx675_write_register(ViPipe, 0X3064, 0X56); //RHS2[19:0]
	imx675_write_register(ViPipe, 0X3065, 0X00); //RHS2[19:0]
	imx675_write_register(ViPipe, 0X3066, 0X00); //RHS2[19:0]
	imx675_write_register(ViPipe, 0X3070, 0X00); //GAIN_0[10:0]
	imx675_write_register(ViPipe, 0X3071, 0X00); //GAIN_0[10:0]
	imx675_write_register(ViPipe, 0X3072, 0X00); //GAIN_1[10:0]
	imx675_write_register(ViPipe, 0X3073, 0X00); //GAIN_1[10:0]
	imx675_write_register(ViPipe, 0X3074, 0X00); //GAIN_2[10:0]
	imx675_write_register(ViPipe, 0X3075, 0X00); //GAIN_2[10:0]
	imx675_write_register(ViPipe, 0X30A4, 0XAA); //XVSOUTSEL[1:0]
	imx675_write_register(ViPipe, 0X30A6, 0X00); //XVS_DRV[1:0]
	imx675_write_register(ViPipe, 0X30CC, 0X00);
	imx675_write_register(ViPipe, 0X30CD, 0X00);
	imx675_write_register(ViPipe, 0X30CE, 0X02);
	imx675_write_register(ViPipe, 0X30DC, 0X32); //BLKLEVEL[9:0]
	imx675_write_register(ViPipe, 0X30DD, 0X40); //BLKLEVEL[9:0]
	imx675_write_register(ViPipe, 0X310C, 0X01);
	imx675_write_register(ViPipe, 0X3130, 0X01);
	imx675_write_register(ViPipe, 0X3148, 0X00);
	imx675_write_register(ViPipe, 0X315E, 0X10);
	imx675_write_register(ViPipe, 0X3400, 0X01); //GAIN_PGC_FIDMD
	imx675_write_register(ViPipe, 0X3460, 0X22);
	imx675_write_register(ViPipe, 0X347B, 0X02);
	imx675_write_register(ViPipe, 0X3492, 0X08);
	imx675_write_register(ViPipe, 0X3890, 0X08); //HFR_EN[3:0]
	imx675_write_register(ViPipe, 0X3891, 0X00); //HFR_EN[3:0]
	imx675_write_register(ViPipe, 0X3893, 0X00);
	imx675_write_register(ViPipe, 0X3B1D, 0X17);
	imx675_write_register(ViPipe, 0X3B44, 0X3F);
	imx675_write_register(ViPipe, 0X3B60, 0X03);
	imx675_write_register(ViPipe, 0X3C03, 0X04);
	imx675_write_register(ViPipe, 0X3C04, 0X04);
	imx675_write_register(ViPipe, 0X3C0A, 0X1F);
	imx675_write_register(ViPipe, 0X3C0B, 0X1F);
	imx675_write_register(ViPipe, 0X3C0C, 0X1F);
	imx675_write_register(ViPipe, 0X3C0D, 0X1F);
	imx675_write_register(ViPipe, 0X3C0E, 0X1F);
	imx675_write_register(ViPipe, 0X3C0F, 0X1F);
	imx675_write_register(ViPipe, 0X3C30, 0X73);
	imx675_write_register(ViPipe, 0X3C3C, 0X20);
	imx675_write_register(ViPipe, 0X3C44, 0X06);
	imx675_write_register(ViPipe, 0X3C7C, 0XB9);
	imx675_write_register(ViPipe, 0X3C7D, 0X01);
	imx675_write_register(ViPipe, 0X3C7E, 0XB7);
	imx675_write_register(ViPipe, 0X3C7F, 0X01);
	imx675_write_register(ViPipe, 0X3CB0, 0X00);
	imx675_write_register(ViPipe, 0X3CB2, 0XFF);
	imx675_write_register(ViPipe, 0X3CB3, 0X03);
	imx675_write_register(ViPipe, 0X3CB4, 0XFF);
	imx675_write_register(ViPipe, 0X3CB5, 0X03);
	imx675_write_register(ViPipe, 0X3CBA, 0XFF);
	imx675_write_register(ViPipe, 0X3CBB, 0X03);
	imx675_write_register(ViPipe, 0X3CC0, 0XFF);
	imx675_write_register(ViPipe, 0X3CC1, 0X03);
	imx675_write_register(ViPipe, 0X3CC2, 0X00);
	imx675_write_register(ViPipe, 0X3CC6, 0XFF);
	imx675_write_register(ViPipe, 0X3CC7, 0X03);
	imx675_write_register(ViPipe, 0X3CC8, 0XFF);
	imx675_write_register(ViPipe, 0X3CC9, 0X03);
	imx675_write_register(ViPipe, 0X3E00, 0X1E);
	imx675_write_register(ViPipe, 0X3E02, 0X04);
	imx675_write_register(ViPipe, 0X3E03, 0X00);
	imx675_write_register(ViPipe, 0X3E20, 0X04);
	imx675_write_register(ViPipe, 0X3E21, 0X00);
	imx675_write_register(ViPipe, 0X3E22, 0X1E);
	imx675_write_register(ViPipe, 0X3E24, 0XBA);
	imx675_write_register(ViPipe, 0X3E72, 0X85);
	imx675_write_register(ViPipe, 0X3E76, 0X0C);
	imx675_write_register(ViPipe, 0X3E77, 0X01);
	imx675_write_register(ViPipe, 0X3E7A, 0X85);
	imx675_write_register(ViPipe, 0X3E7E, 0X1F);
	imx675_write_register(ViPipe, 0X3E82, 0XA6);
	imx675_write_register(ViPipe, 0X3E86, 0X2D);
	imx675_write_register(ViPipe, 0X3EE2, 0X33);
	imx675_write_register(ViPipe, 0X3EE3, 0X03);
	imx675_write_register(ViPipe, 0X4490, 0X07);
	imx675_write_register(ViPipe, 0X4494, 0X19);
	imx675_write_register(ViPipe, 0X4495, 0X00);
	imx675_write_register(ViPipe, 0X4496, 0XBB);
	imx675_write_register(ViPipe, 0X4497, 0X00);
	imx675_write_register(ViPipe, 0X4498, 0X55);
	imx675_write_register(ViPipe, 0X449A, 0X50);
	imx675_write_register(ViPipe, 0X449C, 0X50);
	imx675_write_register(ViPipe, 0X449E, 0X50);
	imx675_write_register(ViPipe, 0X44A0, 0X3C);
	imx675_write_register(ViPipe, 0X44A2, 0X19);
	imx675_write_register(ViPipe, 0X44A4, 0X19);
	imx675_write_register(ViPipe, 0X44A6, 0X19);
	imx675_write_register(ViPipe, 0X44A8, 0X4B);
	imx675_write_register(ViPipe, 0X44AA, 0X4B);
	imx675_write_register(ViPipe, 0X44AC, 0X4B);
	imx675_write_register(ViPipe, 0X44AE, 0X4B);
	imx675_write_register(ViPipe, 0X44B0, 0X3C);
	imx675_write_register(ViPipe, 0X44B2, 0X19);
	imx675_write_register(ViPipe, 0X44B4, 0X19);
	imx675_write_register(ViPipe, 0X44B6, 0X19);
	imx675_write_register(ViPipe, 0X44B8, 0X4B);
	imx675_write_register(ViPipe, 0X44BA, 0X4B);
	imx675_write_register(ViPipe, 0X44BC, 0X4B);
	imx675_write_register(ViPipe, 0X44BE, 0X4B);
	imx675_write_register(ViPipe, 0X44C0, 0X3C);
	imx675_write_register(ViPipe, 0X44C2, 0X19);
	imx675_write_register(ViPipe, 0X44C4, 0X19);
	imx675_write_register(ViPipe, 0X44C6, 0X19);
	imx675_write_register(ViPipe, 0X44C8, 0XF0);
	imx675_write_register(ViPipe, 0X44CA, 0XEB);
	imx675_write_register(ViPipe, 0X44CC, 0XEB);
	imx675_write_register(ViPipe, 0X44CE, 0XE6);
	imx675_write_register(ViPipe, 0X44D0, 0XE6);
	imx675_write_register(ViPipe, 0X44D2, 0XBB);
	imx675_write_register(ViPipe, 0X44D4, 0XBB);
	imx675_write_register(ViPipe, 0X44D6, 0XBB);
	imx675_write_register(ViPipe, 0X44D8, 0XE6);
	imx675_write_register(ViPipe, 0X44DA, 0XE6);
	imx675_write_register(ViPipe, 0X44DC, 0XE6);
	imx675_write_register(ViPipe, 0X44DE, 0XE6);
	imx675_write_register(ViPipe, 0X44E0, 0XE6);
	imx675_write_register(ViPipe, 0X44E2, 0XBB);
	imx675_write_register(ViPipe, 0X44E4, 0XBB);
	imx675_write_register(ViPipe, 0X44E6, 0XBB);
	imx675_write_register(ViPipe, 0X44E8, 0XE6);
	imx675_write_register(ViPipe, 0X44EA, 0XE6);
	imx675_write_register(ViPipe, 0X44EC, 0XE6);
	imx675_write_register(ViPipe, 0X44EE, 0XE6);
	imx675_write_register(ViPipe, 0X44F0, 0XE6);
	imx675_write_register(ViPipe, 0X44F2, 0XBB);
	imx675_write_register(ViPipe, 0X44F4, 0XBB);
	imx675_write_register(ViPipe, 0X44F6, 0XBB);
	imx675_write_register(ViPipe, 0X4538, 0X15);
	imx675_write_register(ViPipe, 0X4539, 0X15);
	imx675_write_register(ViPipe, 0X453A, 0X15);
	imx675_write_register(ViPipe, 0X4544, 0X15);
	imx675_write_register(ViPipe, 0X4545, 0X15);
	imx675_write_register(ViPipe, 0X4546, 0X15);
	imx675_write_register(ViPipe, 0X4550, 0X10);
	imx675_write_register(ViPipe, 0X4551, 0X10);
	imx675_write_register(ViPipe, 0X4552, 0X10);
	imx675_write_register(ViPipe, 0X4553, 0X10);
	imx675_write_register(ViPipe, 0X4554, 0X10);
	imx675_write_register(ViPipe, 0X4555, 0X10);
	imx675_write_register(ViPipe, 0X4556, 0X10);
	imx675_write_register(ViPipe, 0X4557, 0X10);
	imx675_write_register(ViPipe, 0X4558, 0X10);
	imx675_write_register(ViPipe, 0X455C, 0X10);
	imx675_write_register(ViPipe, 0X455D, 0X10);
	imx675_write_register(ViPipe, 0X455E, 0X10);
	imx675_write_register(ViPipe, 0X455F, 0X10);
	imx675_write_register(ViPipe, 0X4560, 0X10);
	imx675_write_register(ViPipe, 0X4561, 0X10);
	imx675_write_register(ViPipe, 0X4562, 0X10);
	imx675_write_register(ViPipe, 0X4563, 0X10);
	imx675_write_register(ViPipe, 0X4564, 0X10);
	imx675_write_register(ViPipe, 0X4569, 0X01);
	imx675_write_register(ViPipe, 0X456A, 0X01);
	imx675_write_register(ViPipe, 0X456B, 0X06);
	imx675_write_register(ViPipe, 0X456C, 0X06);
	imx675_write_register(ViPipe, 0X456D, 0X06);
	imx675_write_register(ViPipe, 0X456E, 0X06);
	imx675_write_register(ViPipe, 0X456F, 0X06);
	imx675_write_register(ViPipe, 0X4570, 0X06);

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

	imx675_write_register(ViPipe, 0X3000, 0X01); //STANDBY
	imx675_write_register(ViPipe, 0X3001, 0X00); //REGHOLD
	imx675_write_register(ViPipe, 0X3002, 0X01); //XMSTA
	imx675_write_register(ViPipe, 0X3014, 0X03); //INCK_SEL[3:0]
	imx675_write_register(ViPipe, 0X3015, 0X03); //DATARATE_SEL[3:0]
	imx675_write_register(ViPipe, 0X3018, 0X00); //WINMODE[3:0]
	imx675_write_register(ViPipe, 0X3019, 0X00); //CFMODE
	imx675_write_register(ViPipe, 0X301A, 0X01); //WDMODE[7:0]
	imx675_write_register(ViPipe, 0X301B, 0X00); //ADDMODE[1:0]
	imx675_write_register(ViPipe, 0X301C, 0X01); //THIN_V_EN[7:0]
	imx675_write_register(ViPipe, 0X301E, 0X01); //VCMODE[7:0]
	imx675_write_register(ViPipe, 0X3020, 0X00); //HREVERSE
	imx675_write_register(ViPipe, 0X3021, 0X00); //VREVERSE
	imx675_write_register(ViPipe, 0X3022, 0X01); //ADBIT[1:0]
	imx675_write_register(ViPipe, 0X3023, 0X01); //MDBIT
	imx675_write_register(ViPipe, 0X3028, 0X98); //VMAX[19:0]
	imx675_write_register(ViPipe, 0X3029, 0X08); //VMAX[19:0]
	imx675_write_register(ViPipe, 0X302A, 0X00); //VMAX[19:0]
	imx675_write_register(ViPipe, 0X302C, 0XA3); //HMAX[15:0]
	imx675_write_register(ViPipe, 0X302D, 0X02); //HMAX[15:0]
	imx675_write_register(ViPipe, 0X3030, 0X00); //FDG_SEL0[1:0]
	imx675_write_register(ViPipe, 0X3031, 0X00); //FDG_SEL1[1:0]
	imx675_write_register(ViPipe, 0X3032, 0X00); //FDG_SEL2[1:0]
	imx675_write_register(ViPipe, 0X303C, 0X00); //PIX_HST[12:0]
	imx675_write_register(ViPipe, 0X303D, 0X00); //PIX_HST[12:0]
	imx675_write_register(ViPipe, 0X303E, 0X30); //PIX_HWIDTH[12:0]
	imx675_write_register(ViPipe, 0X303F, 0X0A); //PIX_HWIDTH[12:0]
	imx675_write_register(ViPipe, 0X3040, 0X03); //LANEMODE[2:0]
	imx675_write_register(ViPipe, 0X3044, 0X00); //PIX_VST[11:0]
	imx675_write_register(ViPipe, 0X3045, 0X00); //PIX_VST[11:0]
	imx675_write_register(ViPipe, 0X3046, 0XAC); //PIX_VWIDTH[11:0]
	imx675_write_register(ViPipe, 0X3047, 0X07); //PIX_VWIDTH[11:0]
	imx675_write_register(ViPipe, 0X304C, 0X00); //GAIN_HG0[10:0]
	imx675_write_register(ViPipe, 0X304D, 0X00); //GAIN_HG0[10:0]
	imx675_write_register(ViPipe, 0X3050, 0X50); //SHR0[19:0]
	imx675_write_register(ViPipe, 0X3051, 0X0A); //SHR0[19:0]
	imx675_write_register(ViPipe, 0X3052, 0X00); //SHR0[19:0]
	imx675_write_register(ViPipe, 0X3054, 0X05); //SHR1[19:0]
	imx675_write_register(ViPipe, 0X3055, 0X00); //SHR1[19:0]
	imx675_write_register(ViPipe, 0X3056, 0X00); //SHR1[19:0]
	imx675_write_register(ViPipe, 0X3058, 0X53); //SHR2[19:0]
	imx675_write_register(ViPipe, 0X3059, 0X00); //SHR2[19:0]
	imx675_write_register(ViPipe, 0X305A, 0X00); //SHR2[19:0]
	imx675_write_register(ViPipe, 0X3060, 0X75); //RHS1[19:0]
	imx675_write_register(ViPipe, 0X3061, 0X00); //RHS1[19:0]
	imx675_write_register(ViPipe, 0X3062, 0X00); //RHS1[19:0]
	imx675_write_register(ViPipe, 0X3064, 0X56); //RHS2[19:0]
	imx675_write_register(ViPipe, 0X3065, 0X00); //RHS2[19:0]
	imx675_write_register(ViPipe, 0X3066, 0X00); //RHS2[19:0]
	imx675_write_register(ViPipe, 0X3070, 0X00); //GAIN_0[10:0]
	imx675_write_register(ViPipe, 0X3071, 0X00); //GAIN_0[10:0]
	imx675_write_register(ViPipe, 0X3072, 0X00); //GAIN_1[10:0]
	imx675_write_register(ViPipe, 0X3073, 0X00); //GAIN_1[10:0]
	imx675_write_register(ViPipe, 0X3074, 0X00); //GAIN_2[10:0]
	imx675_write_register(ViPipe, 0X3075, 0X00); //GAIN_2[10:0]
	imx675_write_register(ViPipe, 0X30A4, 0XAA); //XVSOUTSEL[1:0]
	imx675_write_register(ViPipe, 0X30A6, 0X00); //XVS_DRV[1:0]
	imx675_write_register(ViPipe, 0X30CC, 0X00);
	imx675_write_register(ViPipe, 0X30CD, 0X00);
	imx675_write_register(ViPipe, 0X30CE, 0X02);
	imx675_write_register(ViPipe, 0X30DC, 0X32); //BLKLEVEL[9:0]
	imx675_write_register(ViPipe, 0X30DD, 0X40); //BLKLEVEL[9:0]
	imx675_write_register(ViPipe, 0X310C, 0X01);
	imx675_write_register(ViPipe, 0X3130, 0X01);
	imx675_write_register(ViPipe, 0X3148, 0X00);
	imx675_write_register(ViPipe, 0X315E, 0X10);
	imx675_write_register(ViPipe, 0X3400, 0X01); //GAIN_PGC_FIDMD
	imx675_write_register(ViPipe, 0X3460, 0X22);
	imx675_write_register(ViPipe, 0X347B, 0X02);
	imx675_write_register(ViPipe, 0X3492, 0X08);
	imx675_write_register(ViPipe, 0X3890, 0X08); //HFR_EN[3:0]
	imx675_write_register(ViPipe, 0X3891, 0X00); //HFR_EN[3:0]
	imx675_write_register(ViPipe, 0X3893, 0X00);
	imx675_write_register(ViPipe, 0X3B1D, 0X17);
	imx675_write_register(ViPipe, 0X3B44, 0X3F);
	imx675_write_register(ViPipe, 0X3B60, 0X03);
	imx675_write_register(ViPipe, 0X3C03, 0X04);
	imx675_write_register(ViPipe, 0X3C04, 0X04);
	imx675_write_register(ViPipe, 0X3C0A, 0X1F);
	imx675_write_register(ViPipe, 0X3C0B, 0X1F);
	imx675_write_register(ViPipe, 0X3C0C, 0X1F);
	imx675_write_register(ViPipe, 0X3C0D, 0X1F);
	imx675_write_register(ViPipe, 0X3C0E, 0X1F);
	imx675_write_register(ViPipe, 0X3C0F, 0X1F);
	imx675_write_register(ViPipe, 0X3C30, 0X73);
	imx675_write_register(ViPipe, 0X3C3C, 0X20);
	imx675_write_register(ViPipe, 0X3C44, 0X06);
	imx675_write_register(ViPipe, 0X3C7C, 0XB9);
	imx675_write_register(ViPipe, 0X3C7D, 0X01);
	imx675_write_register(ViPipe, 0X3C7E, 0XB7);
	imx675_write_register(ViPipe, 0X3C7F, 0X01);
	imx675_write_register(ViPipe, 0X3CB0, 0X00);
	imx675_write_register(ViPipe, 0X3CB2, 0XFF);
	imx675_write_register(ViPipe, 0X3CB3, 0X03);
	imx675_write_register(ViPipe, 0X3CB4, 0XFF);
	imx675_write_register(ViPipe, 0X3CB5, 0X03);
	imx675_write_register(ViPipe, 0X3CBA, 0XFF);
	imx675_write_register(ViPipe, 0X3CBB, 0X03);
	imx675_write_register(ViPipe, 0X3CC0, 0XFF);
	imx675_write_register(ViPipe, 0X3CC1, 0X03);
	imx675_write_register(ViPipe, 0X3CC2, 0X00);
	imx675_write_register(ViPipe, 0X3CC6, 0XFF);
	imx675_write_register(ViPipe, 0X3CC7, 0X03);
	imx675_write_register(ViPipe, 0X3CC8, 0XFF);
	imx675_write_register(ViPipe, 0X3CC9, 0X03);
	imx675_write_register(ViPipe, 0X3E00, 0X1E);
	imx675_write_register(ViPipe, 0X3E02, 0X04);
	imx675_write_register(ViPipe, 0X3E03, 0X00);
	imx675_write_register(ViPipe, 0X3E20, 0X04);
	imx675_write_register(ViPipe, 0X3E21, 0X00);
	imx675_write_register(ViPipe, 0X3E22, 0X1E);
	imx675_write_register(ViPipe, 0X3E24, 0XBA);
	imx675_write_register(ViPipe, 0X3E72, 0X85);
	imx675_write_register(ViPipe, 0X3E76, 0X0C);
	imx675_write_register(ViPipe, 0X3E77, 0X01);
	imx675_write_register(ViPipe, 0X3E7A, 0X85);
	imx675_write_register(ViPipe, 0X3E7E, 0X1F);
	imx675_write_register(ViPipe, 0X3E82, 0XA6);
	imx675_write_register(ViPipe, 0X3E86, 0X2D);
	imx675_write_register(ViPipe, 0X3EE2, 0X33);
	imx675_write_register(ViPipe, 0X3EE3, 0X03);
	imx675_write_register(ViPipe, 0X4490, 0X07);
	imx675_write_register(ViPipe, 0X4494, 0X19);
	imx675_write_register(ViPipe, 0X4495, 0X00);
	imx675_write_register(ViPipe, 0X4496, 0XBB);
	imx675_write_register(ViPipe, 0X4497, 0X00);
	imx675_write_register(ViPipe, 0X4498, 0X55);
	imx675_write_register(ViPipe, 0X449A, 0X50);
	imx675_write_register(ViPipe, 0X449C, 0X50);
	imx675_write_register(ViPipe, 0X449E, 0X50);
	imx675_write_register(ViPipe, 0X44A0, 0X3C);
	imx675_write_register(ViPipe, 0X44A2, 0X19);
	imx675_write_register(ViPipe, 0X44A4, 0X19);
	imx675_write_register(ViPipe, 0X44A6, 0X19);
	imx675_write_register(ViPipe, 0X44A8, 0X4B);
	imx675_write_register(ViPipe, 0X44AA, 0X4B);
	imx675_write_register(ViPipe, 0X44AC, 0X4B);
	imx675_write_register(ViPipe, 0X44AE, 0X4B);
	imx675_write_register(ViPipe, 0X44B0, 0X3C);
	imx675_write_register(ViPipe, 0X44B2, 0X19);
	imx675_write_register(ViPipe, 0X44B4, 0X19);
	imx675_write_register(ViPipe, 0X44B6, 0X19);
	imx675_write_register(ViPipe, 0X44B8, 0X4B);
	imx675_write_register(ViPipe, 0X44BA, 0X4B);
	imx675_write_register(ViPipe, 0X44BC, 0X4B);
	imx675_write_register(ViPipe, 0X44BE, 0X4B);
	imx675_write_register(ViPipe, 0X44C0, 0X3C);
	imx675_write_register(ViPipe, 0X44C2, 0X19);
	imx675_write_register(ViPipe, 0X44C4, 0X19);
	imx675_write_register(ViPipe, 0X44C6, 0X19);
	imx675_write_register(ViPipe, 0X44C8, 0XF0);
	imx675_write_register(ViPipe, 0X44CA, 0XEB);
	imx675_write_register(ViPipe, 0X44CC, 0XEB);
	imx675_write_register(ViPipe, 0X44CE, 0XE6);
	imx675_write_register(ViPipe, 0X44D0, 0XE6);
	imx675_write_register(ViPipe, 0X44D2, 0XBB);
	imx675_write_register(ViPipe, 0X44D4, 0XBB);
	imx675_write_register(ViPipe, 0X44D6, 0XBB);
	imx675_write_register(ViPipe, 0X44D8, 0XE6);
	imx675_write_register(ViPipe, 0X44DA, 0XE6);
	imx675_write_register(ViPipe, 0X44DC, 0XE6);
	imx675_write_register(ViPipe, 0X44DE, 0XE6);
	imx675_write_register(ViPipe, 0X44E0, 0XE6);
	imx675_write_register(ViPipe, 0X44E2, 0XBB);
	imx675_write_register(ViPipe, 0X44E4, 0XBB);
	imx675_write_register(ViPipe, 0X44E6, 0XBB);
	imx675_write_register(ViPipe, 0X44E8, 0XE6);
	imx675_write_register(ViPipe, 0X44EA, 0XE6);
	imx675_write_register(ViPipe, 0X44EC, 0XE6);
	imx675_write_register(ViPipe, 0X44EE, 0XE6);
	imx675_write_register(ViPipe, 0X44F0, 0XE6);
	imx675_write_register(ViPipe, 0X44F2, 0XBB);
	imx675_write_register(ViPipe, 0X44F4, 0XBB);
	imx675_write_register(ViPipe, 0X44F6, 0XBB);
	imx675_write_register(ViPipe, 0X4538, 0X15);
	imx675_write_register(ViPipe, 0X4539, 0X15);
	imx675_write_register(ViPipe, 0X453A, 0X15);
	imx675_write_register(ViPipe, 0X4544, 0X15);
	imx675_write_register(ViPipe, 0X4545, 0X15);
	imx675_write_register(ViPipe, 0X4546, 0X15);
	imx675_write_register(ViPipe, 0X4550, 0X10);
	imx675_write_register(ViPipe, 0X4551, 0X10);
	imx675_write_register(ViPipe, 0X4552, 0X10);
	imx675_write_register(ViPipe, 0X4553, 0X10);
	imx675_write_register(ViPipe, 0X4554, 0X10);
	imx675_write_register(ViPipe, 0X4555, 0X10);
	imx675_write_register(ViPipe, 0X4556, 0X10);
	imx675_write_register(ViPipe, 0X4557, 0X10);
	imx675_write_register(ViPipe, 0X4558, 0X10);
	imx675_write_register(ViPipe, 0X455C, 0X10);
	imx675_write_register(ViPipe, 0X455D, 0X10);
	imx675_write_register(ViPipe, 0X455E, 0X10);
	imx675_write_register(ViPipe, 0X455F, 0X10);
	imx675_write_register(ViPipe, 0X4560, 0X10);
	imx675_write_register(ViPipe, 0X4561, 0X10);
	imx675_write_register(ViPipe, 0X4562, 0X10);
	imx675_write_register(ViPipe, 0X4563, 0X10);
	imx675_write_register(ViPipe, 0X4564, 0X10);
	imx675_write_register(ViPipe, 0X4569, 0X01);
	imx675_write_register(ViPipe, 0X456A, 0X01);
	imx675_write_register(ViPipe, 0X456B, 0X06);
	imx675_write_register(ViPipe, 0X456C, 0X06);
	imx675_write_register(ViPipe, 0X456D, 0X06);
	imx675_write_register(ViPipe, 0X456E, 0X06);
	imx675_write_register(ViPipe, 0X456F, 0X06);
	imx675_write_register(ViPipe, 0X4570, 0X06);

	imx675_default_reg_init(ViPipe);

	imx675_write_register(ViPipe, 0x3000, 0x00); /* standby */
	delay_ms(80);
	imx675_write_register(ViPipe, 0x3002, 0x00); /* master mode start */

	printf("ViPipe:%d,===Imx675 5M25fps 12bit 2to1 WDR Init Ok!====\n", ViPipe);
}