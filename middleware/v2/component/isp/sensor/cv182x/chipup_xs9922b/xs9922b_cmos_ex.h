#ifndef __XS9922B_CMOS_EX_H_
#define __XS9922B_CMOS_EX_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef ARCH_CV182X
#include <linux/cvi_vip_cif.h>
#include <linux/cvi_vip_snsr.h>
#include "cvi_type.h"
#else
#include <linux/cif_uapi.h>
#include <linux/vi_snsr.h>
#include <linux/cvi_type.h>
#endif
#include "cvi_sns_ctrl.h"

typedef enum _XS9922B_MODE_E {
	XS9922B_MODE_NONE,
	XS9922B_MODE_720P_1CH,
	XS9922B_MODE_720P_2CH,
	XS9922B_MODE_720P_3CH,
	XS9922B_MODE_720P_4CH,
	XS9922B_MODE_NUM
} XS9922B_MODE_E;

typedef struct _XS9922B_MODE_S {
	ISP_WDR_SIZE_S astImg[2];
	CVI_FLOAT f32MaxFps;
	CVI_FLOAT f32MinFps;
	CVI_U32 u32HtsDef;
	CVI_U32 u32VtsDef;
	SNS_ATTR_S stExp[2];
	SNS_ATTR_S stAgain[2];
	SNS_ATTR_S stDgain[2];
	CVI_U8 u8DgainReg;
	char name[64];
} XS9922B_MODE_S;


/****************************************************************************
* The config of xs9922b *
****************************************************************************/
typedef struct regval {
	unsigned short addr;
	unsigned char val;
	unsigned char nDelay; // ms
}REG_VAL;


#define REG_NULL            0xFFFF
#define REG_DELAY           0xFFFE
/*
* 0x100 ~ 0x1E2
*/
#define USE_HD_CAM 1

/*
* 0x300 ~ 0x349
*/
#define USE_SD_CAM 0

/*
* Coaxial communication
* 0x600 ~ 659
*/
#define USE_COAXIAL 0

/*
* 0x700 ~ 0x714
* 0x400c ~ 0x4012
* 0x4350~0x4362
* 0x4407 ~ 0x4429
*/
#define USE_AUDIO 0


//去斜纹的配置，从配置分辨率部分挪到了初始化部分
 #define RM_STRIPES 1

//#define MIPILANESWAP 1

#define USE_YUVGAIN 1
#define Y_GAIN 0x80
#define U_GAIN 0x80
#define V_GAIN 0x80

#define XS9922B_FREERUN_COLOR_VAL 0x8   //0x6 is blue

#define XS9922B_CH0_EN_REG    0x0e08
#define XS9922B_CH1_EN_REG    0x1e08
#define XS9922B_CH2_EN_REG    0x2e08
#define XS9922B_CH3_EN_REG    0x3e08


static const REG_VAL xs9922_init_cfg[] = {
	{0x4200, 0x3f, 0x00},
	{0x4210, 0x3f, 0x00},
	{0x4220, 0x3f, 0x00},
	{0x4230, 0x3f, 0x00},
	{0x0e08, 0x00, 0x00},
	{0x1e08, 0x00, 0x00},
	{0x2e08, 0x00, 0x00},
	{0x3e08, 0x00, 0x00},
	//{0x4500, 0x02},
	{0x4f00, 0x01, 0x00},
	{0x4030, 0x3f, 0x00},
	{0x0e02, 0x00, 0x00},
	{0x1e02, 0x00, 0x00},
	{0x2e02, 0x00, 0x00},
	{0x3e02, 0x00, 0x00},
	{0x0803, 0x00, 0x00},
	{0x1803, 0x00, 0x00},
	{0x2803, 0x00, 0x00},
	{0x3803, 0x00, 0x00},
	{0x4020, 0x00, 0x00},
	{0x080e, 0x00, 0x00},
	{0x080e, 0x20, 0x00},
	{0x080e, 0x28, 0x00},
	{0x180e, 0x00, 0x00},
	{0x180e, 0x20, 0x00},
	{0x180e, 0x28, 0x00},
	{0x280e, 0x00, 0x00},
	{0x280e, 0x20, 0x00},
	{0x280e, 0x28, 0x00},
	{0x380e, 0x00, 0x00},
	{0x380e, 0x20, 0x00},
	{0x380e, 0x28, 0x00},
	{0x4020, 0x03, 0x00},
	{0x0803, 0x0f, 0x00},
	{0x1803, 0x0f, 0x00},
	{0x2803, 0x0f, 0x00},
	{0x3803, 0x0f, 0x00},
#if RM_STRIPES
	{0x0800, 0x16, 0x00},
	{0x1800, 0x16, 0x00},
	{0x2800, 0x16, 0x00},
	{0x3800, 0x16, 0x00},
	{0x4205, 0x36, 0x00},
	{0x4215, 0x36, 0x00},
	{0x4225, 0x36, 0x00},
	{0x4235, 0x36, 0x00},//wait(0.05)
	{0x4205, 0x26, 0x00},
	{0x4215, 0x26, 0x00},
	{0x4225, 0x26, 0x00},
	{0x4235, 0x26, 0x00},
	{0x0800, 0x17, 0x00},
	{0x1800, 0x17, 0x00},
	{0x2800, 0x17, 0x00},
	{0x3800, 0x17, 0x00},
#endif
	{0x4340, 0x65, 0x00},
	{0x4204, 0x02, 0x00},
	{0x4214, 0x02, 0x00},
	{0x4224, 0x02, 0x00},
	{0x4234, 0x02, 0x00},
	{0x4080, 0x07, 0x00},
	{0x4119, 0x01, 0x00},
	{0x0501, 0x84, 0x00},
#if USE_YUVGAIN
	{0x010e, Y_GAIN, 0x00},
	{0x010f, U_GAIN, 0x00},
	{0x0110, V_GAIN, 0x00},
#endif
	{0x0111, 0x40, 0x00},
	{0x1501, 0x81, 0x00},
#if USE_YUVGAIN
	{0x110e, Y_GAIN, 0x00},
	{0x110f, U_GAIN, 0x00},
	{0x1110, V_GAIN, 0x00},
#endif
	{0x1111, 0x40, 0x00},
	{0x2501, 0x8e, 0x00},
#if USE_YUVGAIN
	{0x210e, Y_GAIN, 0x00},
	{0x210f, U_GAIN, 0x00},
	{0x2110, V_GAIN, 0x00},
#endif
	{0x2111, 0x40, 0x00},
	{0x3501, 0x8b, 0x00},
#if USE_YUVGAIN
	{0x310e, Y_GAIN, 0x00},
	{0x310f, U_GAIN, 0x00},
	{0x3110, V_GAIN, 0x00},
#endif
	{0x3111, 0x40, 0x00},
	{0x4141, 0x22, 0x00},
	{0x4140, 0x22, 0x00},
	{0x413f, 0x22, 0x00},
	{0x413e, 0x22, 0x00},
	{0x030c, 0x03, 0x00},
	{0x0300, 0x3f, 0x00},
	{0x0333, 0x09, 0x00},
	{0x0305, 0xe0, 0x00},
	{0x011c, 0x32, 0x00},
	{0x0105, 0xe1, 0x00},
	//{0x0106, 0x80}, // contrast
	//{0x0107, 0x00}, // brightness
	//{0x0108, 0x80}, // staturation
	//{0x0109, 0x00}, // hue
	{0x01bf, 0x4e, 0x00},
	{0x0b7c, 0x02, 0x00},
	{0x0b55, 0x80, 0x00},
	{0x0b56, 0x00, 0x00},
	{0x0b59, 0x04, 0x00},
	{0x0b5a, 0x01, 0x00},
	{0x0b5c, 0x07, 0x00},
	{0x0b5e, 0x05, 0x00},
	{0x0b31, 0x18, 0x00},
	{0x0b36, 0x40, 0x00},
	{0x0b37, 0x1f, 0x00},
	{0x0b4b, 0x10, 0x00},
	{0x0b4e, 0x05, 0x00},
	{0x0b51, 0x21, 0x00},
	{0x0b15, 0x03, 0x00},
	{0x0b16, 0x03, 0x00},
	{0x0b17, 0x03, 0x00},
	{0x0b07, 0x03, 0x00},
	{0x0b08, 0x05, 0x00},
	{0x0b1a, 0x10, 0x00},
	{0x0b1e, 0xb8, 0x00},
	{0x0b1f, 0x08, 0x00},
	{0x0b5f, 0x64, 0x00},
	{0x130c, 0x03, 0x00},
	{0x1300, 0x3f, 0x00},
	{0x1333, 0x09, 0x00},
	{0x111c, 0x32, 0x00},
	{0x11bf, 0x4e, 0x00},
	{0x1b7c, 0x02, 0x00},
	{0x1b55, 0x80, 0x00},
	{0x1b56, 0x00, 0x00},
	{0x1b59, 0x04, 0x00},
	{0x1b5a, 0x01, 0x00},
	{0x1b5c, 0x07, 0x00},
	{0x1b5e, 0x05, 0x00},
	{0x1b31, 0x18, 0x00},
	{0x1b36, 0x40, 0x00},
	{0x1b37, 0x1f, 0x00},
	{0x1b4b, 0x10, 0x00},
	{0x1b4e, 0x05, 0x00},
	{0x1b51, 0x21, 0x00},
	{0x1b15, 0x03, 0x00},
	{0x1b16, 0x03, 0x00},
	{0x1b17, 0x03, 0x00},
	{0x1b07, 0x03, 0x00},
	{0x1b08, 0x05, 0x00},
	{0x1b1a, 0x10, 0x00},
	{0x1b1e, 0xb8, 0x00},
	{0x1b1f, 0x08, 0x00},
	{0x1b5f, 0x64, 0x00},
	{0x1105, 0xe1, 0x00},
	//{0x1106, 0x80},
	//{0x1107, 0x00},
	//{0x1108, 0x80},
	//{0x1109, 0x00},
	{0x230c, 0x03, 0x00},
	{0x2300, 0x3f, 0x00},
	{0x2333, 0x09, 0x00},
	{0x211c, 0x32, 0x00},
	{0x21bf, 0x4e, 0x00},
	{0x2b7c, 0x02, 0x00},
	{0x2b55, 0x80, 0x00},
	{0x2b56, 0x00, 0x00},
	{0x2b59, 0x04, 0x00},
	{0x2b5a, 0x01, 0x00},
	{0x2b5c, 0x07, 0x00},
	{0x2b5e, 0x05, 0x00},
	{0x2b31, 0x18, 0x00},
	{0x2b36, 0x40, 0x00},
	{0x2b37, 0x1f, 0x00},
	{0x2b4b, 0x10, 0x00},
	{0x2b4e, 0x05, 0x00},
	{0x2b51, 0x21, 0x00},
	{0x2b15, 0x03, 0x00},
	{0x2b16, 0x03, 0x00},
	{0x2b17, 0x03, 0x00},
	{0x2b07, 0x03, 0x00},
	{0x2b08, 0x05, 0x00},
	{0x2b1a, 0x10, 0x00},
	{0x2b1e, 0xb8, 0x00},
	{0x2b1f, 0x08, 0x00},
	{0x2b5f, 0x64, 0x00},
	{0x2105, 0xe1, 0x00},
	//{0x2106, 0x80},
	//{0x2107, 0x00},
	//{0x2108, 0x80},
	//{0x2109, 0x00},
	{0x330c, 0x03, 0x00},
	{0x3300, 0x3f, 0x00},
	{0x3333, 0x09, 0x00},
	{0x311c, 0x32, 0x00},
	{0x31bf, 0x4e, 0x00},
	{0x3b7c, 0x02, 0x00},
	{0x3b55, 0x80, 0x00},
	{0x3b56, 0x00, 0x00},
	{0x3b59, 0x04, 0x00},
	{0x3b5a, 0x01, 0x00},
	{0x3b5c, 0x07, 0x00},
	{0x3b5e, 0x05, 0x00},
	{0x3b31, 0x18, 0x00},
	{0x3b36, 0x40, 0x00},
	{0x3b37, 0x1f, 0x00},
	{0x3b4b, 0x10, 0x00},
	{0x3b4e, 0x05, 0x00},
	{0x3b51, 0x21, 0x00},
	{0x3b15, 0x03, 0x00},
	{0x3b16, 0x03, 0x00},
	{0x3b17, 0x03, 0x00},
	{0x3b07, 0x03, 0x00},
	{0x3b08, 0x05, 0x00},
	{0x3b1a, 0x10, 0x00},
	{0x3b1e, 0xb8, 0x00},
	{0x3b1f, 0x08, 0x00},
	{0x3b5f, 0x64, 0x00},
	{0x3105, 0xe1, 0x00},
	//{0x3106, 0x80},
	//{0x3107, 0x00},
	//{0x3108, 0x80},
	//{0x3109, 0x00},
	{0x4800, 0x81, 0x00},
	{0x4802, 0x01, 0x00},
	{0x4030, 0x15, 0x00},
	{0x50fc, 0x00, 0x00},
	{0x50fd, 0x00, 0x00},
	{0x50fe, 0x0d, 0x00},
	{0x50ff, 0x59, 0x00},
	{0x50e4, 0x00, 0x00},//32位寄存器，0x50e4高字节，0x50e7低8位字节
	{0x50e5, 0x00, 0x00},
	{0x50e6, 0x2f, 0x00},
	{0x50e7, 0x03, 0x00},//0=1lane,1=2lane,2=3lane,3=4lane
	{0x50f0, 0x00, 0x00},
	{0x50f1, 0x00, 0x00},
	{0x50f2, 0x00, 0x00},
	{0x50f3, 0x32, 0x00},
	{0x50e0, 0x00, 0x00},
	{0x50e1, 0x00, 0x00},
	{0x50e2, 0x00, 0x00},
	{0x50e3, 0x00, 0x00},
	{0x5114, 0x00, 0x00},
	{0x5115, 0x00, 0x00},
	{0x5116, 0x00, 0x00},
	{0x5117, 0x03, 0x00},
	{0x5114, 0x00, 0x00},
	{0x5115, 0x00, 0x00},
	{0x5116, 0x00, 0x00},
	{0x5117, 0x02, 0x00},
	{0x5118, 0x00, 0x00},
	{0x5119, 0x01, 0x00},
	{0x511a, 0x00, 0x00},
	{0x511b, 0x44, 0x00},
	{0x5114, 0x00, 0x00},
	{0x5115, 0x00, 0x00},
	{0x5116, 0x00, 0x00},
	{0x5117, 0x00, 0x00},
	{0x5118, 0x00, 0x00},
	{0x5119, 0x00, 0x00},
	{0x511a, 0x00, 0x00},
	{0x511b, 0x78, 0x00},//0x78=1.5G,0x36=1.2G,0x34=1G,0x32=800M,0x30=700M,0x2E=600M
	{0x5114, 0x00, 0x00},
	{0x5115, 0x00, 0x00},
	{0x5116, 0x00, 0x00},
	{0x5117, 0x02, 0x00},
	{0x5118, 0x00, 0x00},
	{0x5119, 0x01, 0x00},
	{0x511a, 0x00, 0x00},
	{0x511b, 0x30, 0x00},
	{0x5114, 0x00, 0x00},
	{0x5115, 0x00, 0x00},
	{0x5116, 0x00, 0x00},
	{0x5117, 0x00, 0x00},
	{0x5118, 0x00, 0x00},
	{0x5119, 0x00, 0x00},
	{0x511a, 0x00, 0x00},
	{0x511b, 0x03, 0x00},
	{0x5114, 0x00, 0x00},
	{0x5115, 0x00, 0x00},
	{0x5116, 0x00, 0x00},
	{0x5117, 0x02, 0x00},
	{0x5118, 0x00, 0x00},
	{0x5119, 0x01, 0x00},
	{0x511a, 0x00, 0x00},
	{0x511b, 0x40, 0x00},
	{0x5114, 0x00, 0x00},
	{0x5115, 0x00, 0x00},
	{0x5116, 0x00, 0x00},
	{0x5117, 0x00, 0x00},
	{0x5118, 0x00, 0x00},
	{0x5119, 0x00, 0x00},
	{0x511a, 0x00, 0x00},
	{0x511b, 0x03, 0x00},
	{0x5114, 0x00, 0x00},
	{0x5115, 0x00, 0x00},
	{0x5116, 0x00, 0x00},
	{0x5117, 0x02, 0x00},
	{0x5118, 0x00, 0x00},
	{0x5119, 0x01, 0x00},
	{0x511a, 0x00, 0x00},
	{0x511b, 0x50, 0x00},
	{0x5114, 0x00, 0x00},
	{0x5115, 0x00, 0x00},
	{0x5116, 0x00, 0x00},
	{0x5117, 0x00, 0x00},
	{0x5118, 0x00, 0x00},
	{0x5119, 0x00, 0x00},
	{0x511a, 0x00, 0x00},
	{0x511b, 0x03, 0x00},
	{0x5114, 0x00, 0x00},
	{0x5115, 0x00, 0x00},
	{0x5116, 0x00, 0x00},
	{0x5117, 0x02, 0x00},
	{0x5118, 0x00, 0x00},
	{0x5119, 0x01, 0x00},
	{0x511a, 0x00, 0x00},
	{0x511b, 0x80, 0x00},
	{0x5114, 0x00, 0x00},
	{0x5115, 0x00, 0x00},
	{0x5116, 0x00, 0x00},
	{0x5117, 0x00, 0x00},
	{0x5118, 0x00, 0x00},
	{0x5119, 0x00, 0x00},
	{0x511a, 0x00, 0x00},
	{0x511b, 0x03, 0x00},
	{0x5114, 0x00, 0x00},
	{0x5115, 0x00, 0x00},
	{0x5116, 0x00, 0x00},
	{0x5117, 0x02, 0x00},
	{0x5118, 0x00, 0x00},
	{0x5119, 0x01, 0x00},
	{0x511a, 0x00, 0x00},
	{0x511b, 0x90, 0x00},
	{0x5114, 0x00, 0x00},
	{0x5115, 0x00, 0x00},
	{0x5116, 0x00, 0x00},
	{0x5117, 0x00, 0x00},
	{0x5118, 0x00, 0x00},
	{0x5119, 0x00, 0x00},
	{0x511a, 0x00, 0x00},
	{0x511b, 0x03, 0x00},
	{0x5114, 0x00, 0x00},
	{0x5115, 0x00, 0x00},
	{0x5116, 0x00, 0x00},
	{0x5117, 0x02, 0x00},
	{0x5118, 0x00, 0x00},
	{0x5119, 0x01, 0x00},
	{0x511a, 0x00, 0x00},
	{0x511b, 0x20, 0x00},
	{0x5114, 0x00, 0x00},
	{0x5115, 0x00, 0x00},
	{0x5116, 0x00, 0x00},
	{0x5117, 0x00, 0x00},
	{0x5118, 0x00, 0x00},
	{0x5119, 0x00, 0x00},
	{0x511a, 0x00, 0x00},
	{0x511b, 0x45, 0x00},
	{0x5114, 0x00, 0x00},
	{0x5115, 0x00, 0x00},
	{0x5116, 0x00, 0x00},
	{0x5117, 0x02, 0x00},
#if MIPILANESWAP
	{0x5114, 0x00, 0x00},
	{0x5115, 0x00, 0x00},
	{0x5116, 0x00, 0x00},
	{0x5117, 0x02, 0x00},
	{0x5118, 0x00, 0x00},
	{0x5119, 0x01, 0x00},
	{0x511a, 0x00, 0x00},
	{0x511b, 0x55, 0x00},
	{0x5114, 0x00, 0x00},
	{0x5115, 0x00, 0x00},
	{0x5116, 0x00, 0x00},
	{0x5117, 0x00, 0x00},
	{0x5118, 0x00, 0x00},
	{0x5119, 0x00, 0x00},
	{0x511a, 0x00, 0x00},
	{0x511b, 0x01, 0x00},
	{0x5114, 0x00, 0x00},
	{0x5115, 0x00, 0x00},
	{0x5116, 0x00, 0x00},
	{0x5117, 0x02, 0x00},

	{0x5114, 0x00, 0x00},
	{0x5115, 0x00, 0x00},
	{0x5116, 0x00, 0x00},
	{0x5117, 0x02, 0x00},
	{0x5118, 0x00, 0x00},
	{0x5119, 0x01, 0x00},
	{0x511a, 0x00, 0x00},
	{0x511b, 0x85, 0x00},
	{0x5114, 0x00, 0x00},
	{0x5115, 0x00, 0x00},
	{0x5116, 0x00, 0x00},
	{0x5117, 0x00, 0x00},
	{0x5118, 0x00, 0x00},
	{0x5119, 0x00, 0x00},
	{0x511a, 0x00, 0x00},
	{0x511b, 0x01, 0x00},
	{0x5114, 0x00, 0x00},
	{0x5115, 0x00, 0x00},
	{0x5116, 0x00, 0x00},
	{0x5117, 0x02, 0x00},
#endif
	{0x50e0, 0x00, 0x00},
	{0x50e1, 0x00, 0x00},
	{0x50e2, 0x00, 0x00},
	{0x50e3, 0x04, 0x00},
	{0x50e0, 0x00, 0x00},
	{0x50e1, 0x00, 0x00},
	{0x50e2, 0x00, 0x00},
	{0x50e3, 0x05, 0x00},
	{0x50e0, 0x00, 0x00},
	{0x50e1, 0x00, 0x00},
	{0x50e2, 0x00, 0x00},
	{0x50e3, 0x07, 0x00},
	{0x50e8, 0x00, 0x00},
	{0x50e9, 0x00, 0x00},
	{0x50ea, 0x00, 0x00},
	{0x50eb, 0x01, 0x00},  //1：连续模式
	{0x4801, 0x81, 0x00},
	{0x4031, 0x01, 0x00},
	{0x4138, 0x1e, 0x00},
	{0x413b, 0x1e, 0x00},
	{0x4135, 0x1e, 0x00},
	{0x412f, 0x1e, 0x00},
	{0x412c, 0x1e, 0x00},
	{0x4126, 0x1e, 0x00},
	{0x4129, 0x1e, 0x00},
	{0x4123, 0x1e, 0x00},
	{0x4f00, 0x01, 0x00},
	{REG_NULL, 0x0, 0x00},
};


static const struct regval xs9922_720p_4lanes_25fps_1500M[] = {
	{0x0803, 0x03, 0x00}, // cam0
	{0x080e, 0x0f, 0x00},
	{0x0803, 0x0f, 0x00},
	{0x080e, 0x3f, 0x00},
	{0x080e, 0x3f, 0x00},
	{0x012d, 0x3f, 0x00},
	{0x012f, 0xcc, 0x00},
	{0x01e2, 0x03, 0x00},
	{0x0158, 0x01, 0x00},
	{0x0130, 0x10, 0x00},
	{0x010c, 0x01, 0x00},
	{0x010d, 0x40, 0x00}, // timing
	{0x0805, 0x05, 0x00},
	{0x0e11,  XS9922B_FREERUN_COLOR_VAL, 0x00},
	{0x0e12, 0x01, 0x00},
	{0x060b, 0x00, 0x00},
	{0x0627, 0x14, 0x00},
	{0x061c, 0x00, 0x00},
	{0x061d, 0x5a, 0x00},
	{0x061e, 0xa5, 0x00},
	{0x0640, 0x04, 0x00},
	{0x0616, 0x24, 0x00},
	{0x0617, 0x00, 0x00},
	{0x0618, 0x04, 0x00},
	{0x060a, 0x07, 0x00},
	{0x010a, 0x05, 0x00},
	{0x0100, 0x30, 0x00},
	{0x0104, 0x48, 0x00}, //1500M
	{0x0802, 0x21, 0x00},
	{0x0502, 0x0c, 0x00},
	{0x050e, 0x1c, 0x00},
	{0x1803, 0x03, 0x00}, // cam1
	{0x180e, 0x0f, 0x00},
	{0x1803, 0x0f, 0x00},
	{0x180e, 0x3f, 0x00},
	{0x180e, 0x3f, 0x00},
	{0x112d, 0x3f, 0x00},
	{0x112f, 0xcc, 0x00},
	{0x11e2, 0x03, 0x00},
	{0x1158, 0x01, 0x00},
	{0x1130, 0x10, 0x00},
	{0x110c, 0x01, 0x00},
	{0x110d, 0x40, 0x00}, // timing
	{0x1805, 0x05, 0x00},
	{0x1e11, 0x06, 0x00},
	{0x1e12, 0x01, 0x00},
	{0x160b, 0x00, 0x00},
	{0x1627, 0x14, 0x00},
	{0x161c, 0x00, 0x00},
	{0x161d, 0x5a, 0x00},
	{0x161e, 0xa5, 0x00},
	{0x1640, 0x04, 0x00},
	{0x1616, 0x24, 0x00},
	{0x1617, 0x00, 0x00},
	{0x1618, 0x04, 0x00},
	{0x160a, 0x07, 0x00},
	{0x110a, 0x05, 0x00},
	{0x1100, 0x30, 0x00},
	{0x1104, 0x48, 0x00},
	{0x1802, 0x21, 0x00},
	{0x1502, 0x0d, 0x00},
	{0x150e, 0x1d, 0x00},
	{0x2803, 0x03, 0x00}, // cam2
	{0x280e, 0x0f, 0x00},
	{0x2803, 0x0f, 0x00},
	{0x280e, 0x3f, 0x00},
	{0x280e, 0x3f, 0x00},
	{0x212d, 0x3f, 0x00},
	{0x212f, 0xcc, 0x00},
	{0x21e2, 0x03, 0x00},
	{0x2158, 0x01, 0x00},
	{0x2130, 0x10, 0x00},
	{0x210c, 0x01, 0x00},
	{0x210d, 0x40, 0x00}, // timing
	{0x2805, 0x05, 0x00},
	{0x2e11, 0x06, 0x00},
	{0x2e12, 0x01, 0x00},
	{0x260b, 0x00, 0x00},
	{0x2627, 0x14, 0x00},
	{0x261c, 0x00, 0x00},
	{0x261d, 0x5a, 0x00},
	{0x261e, 0xa5, 0x00},
	{0x2640, 0x04, 0x00},
	{0x2616, 0x24, 0x00},
	{0x2617, 0x00, 0x00},
	{0x2618, 0x04, 0x00},
	{0x260a, 0x07, 0x00},
	{0x210a, 0x05, 0x00},
	{0x2100, 0x30, 0x00},
	{0x2104, 0x48, 0x00},
	{0x2802, 0x21, 0x00},
	{0x2502, 0x0e, 0x00},
	{0x250e, 0x1e, 0x00},
	{0x3803, 0x03, 0x00}, // cam3
	{0x380e, 0x0f, 0x00},
	{0x3803, 0x0f, 0x00},
	{0x380e, 0x3f, 0x00},
	{0x380e, 0x3f, 0x00},
	{0x312d, 0x3f, 0x00},
	{0x312f, 0xcc, 0x00},
	{0x31e2, 0x03, 0x00},
	{0x3158, 0x01, 0x00},
	{0x3130, 0x10, 0x00},
	{0x310c, 0x01, 0x00},
	{0x310d, 0x40, 0x00}, // timing
	{0x3805, 0x05, 0x00},
	{0x3e11, 0x06, 0x00},
	{0x3e12, 0x01, 0x00},
	{0x360b, 0x00, 0x00},
	{0x3627, 0x14, 0x00},
	{0x361c, 0x00, 0x00},
	{0x361d, 0x5a, 0x00},
	{0x361e, 0xa5, 0x00},
	{0x3640, 0x04, 0x00},
	{0x3616, 0x24, 0x00},
	{0x3617, 0x00, 0x00},
	{0x3618, 0x04, 0x00},
	{0x360a, 0x07, 0x00},
	{0x310a, 0x05, 0x00},
	{0x3100, 0x38, 0x00},
	{0x3104, 0x48, 0x00},
	{0x3802, 0x21, 0x00},
	{0x3502, 0x0f, 0x00},
	{0x350e, 0x1f, 0x00},
	{0x0e08, 0x01, 0x00},
	{0x1e08, 0x01, 0x00},
	{0x2e08, 0x01, 0x00},
	{0x3e08, 0x01, 0x00},
	{0x5004, 0x00, 0x00},
	{0x5005, 0x00, 0x00},
	{0x5006, 0x00, 0x00},
	{0x5007, 0x00, 0x00},
	{0x5004, 0x00, 0x00},
	{0x5005, 0x00, 0x00},
	{0x5006, 0x00, 0x00},
	{0x5007, 0x01, 0x00},
	{REG_NULL, 0x0, 0x00},
};

static const struct regval xs9922_mipi_quick_streams[] = {
	{0x0e08, 0x01, 0x00}, //enable chn0
	{0x1e08, 0x01, 0x00},
	{0x2e08, 0x01, 0x00},
	{0x3e08, 0x01, 0x00},
	{0x5004, 0x00, 0x00},
	{0x5005, 0x00, 0x00},
	{0x5006, 0x00, 0x00},
	{0x5007, 0x00, 0x00},
	{0x5004, 0x00, 0x00},
	{0x5005, 0x00, 0x00},
	{0x5006, 0x00, 0x00},
	{0x5007, 0x01, 0x00},
	{REG_NULL, 0x0, 0x00},
};

static const struct regval xs9922_mipi_dphy_set[] = {
	{0x5114, 0x0, 0x00},
	{0x5115, 0x0, 0x00},
	{0x5116, 0x0, 0x00},
	{0x5117, 0x2, 0x00},
	{0x5118, 0x0, 0x00},
	{0x5119, 0x1, 0x00},
	{0x511a, 0x0, 0x00},
	/* T-Trail time reg: 0x73 */
	{0x511b, 0x73, 0x00}, /* The val is means the reg add of d-phy */
	{0x5114, 0x0, 0x00},
	{0x5115, 0x0, 0x00},
	{0x5116, 0x0, 0x00},
	{0x5117, 0x0, 0x00},
	{0x5118, 0x0, 0x00},
	{0x5119, 0x0, 0x00},
	{0x511a, 0x0, 0x00},
	{0x511b, 0x9a, 0x00}, /* The val is means the reg val of d-phy */
	{0x5114, 0x0, 0x00},
	{0x5115, 0x0, 0x00},
	{0x5116, 0x0, 0x00},
	{0x5117, 0x2, 0x00},
	{REG_NULL, 0x0, 0x00},
};


/****************************************************************************
 * external variables and functions                                         *
 ****************************************************************************/

extern ISP_SNS_STATE_S *g_pastXS9922b[VI_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U g_aunXS9922b_BusInfo[];
extern const CVI_U8 xs9922b_i2c_addr;
extern const CVI_U32 xs9922b_addr_byte;
extern const CVI_U32 xs9922b_data_byte;
extern void xs9922b_init(VI_PIPE ViPipe);
extern void xs9922b_exit(VI_PIPE ViPipe);
extern void xs9922b_standby(VI_PIPE ViPipe);
extern void xs9922b_restart(VI_PIPE ViPipe);
extern int  xs9922b_write_register(VI_PIPE ViPipe, int addr, int data);
extern int  xs9922b_read_register(VI_PIPE ViPipe, int addr);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __XS9922B_CMOS_EX_H_ */