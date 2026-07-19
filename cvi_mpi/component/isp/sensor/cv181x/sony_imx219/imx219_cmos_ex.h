#ifndef __IMX219_CMOS_EX_H_
#define __IMX219_CMOS_EX_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#include <linux/cvi_type.h>
#include "cvi_sns_ctrl.h"


enum imx219_linear_regs_e {
	LINEAR_EXP_0,
	LINEAR_EXP_1,
	LINEAR_AGAIN_0,
	LINEAR_DGAIN_0,
	LINEAR_DGAIN_1,
	LINEAR_VTS_0,
	LINEAR_VTS_1,
	LINEAR_REGS_NUM
};

typedef enum _IMX219_MODE_E {
	IMX219_MODE_1920X1080P30 = 0,
	IMX219_MODE_2560X1920P27,
	IMX219_MODE_LINEAR_NUM,
	IMX219_MODE_NUM = IMX219_MODE_LINEAR_NUM
} IMX219_MODE_E;

typedef struct _IMX219_STATE_S {
	CVI_U32		u32Sexp_MAX;
} IMX219_STATE_S;

typedef struct _IMX219_MODE_S {
	ISP_WDR_SIZE_S astImg[2];
	CVI_FLOAT f32MaxFps;
	CVI_FLOAT f32MinFps;
	CVI_U32 u32HtsDef;
	CVI_U32 u32VtsDef;
	CVI_U16 u16L2sOffset;
	CVI_U16 u16TopBoundary;
	CVI_U16 u16BotBoundary;
	SNS_ATTR_S stExp[2];
	SNS_ATTR_LARGE_S stAgain[2];
	SNS_ATTR_LARGE_S stDgain[2];
	CVI_U32 u32L2S_offset;
	CVI_U32 u32IspResTime;
	CVI_U32 u32HdrMargin;
	char name[64];
} IMX219_MODE_S;

/****************************************************************************
 * external variables and functions                                         *
 ****************************************************************************/

extern ISP_SNS_STATE_S *g_pastImx219[VI_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U g_aunImx219_BusInfo[];
extern CVI_U16 g_au16Imx219_GainMode[];
extern CVI_U16 g_au16Imx219_UseHwSync[VI_MAX_PIPE_NUM];
extern CVI_U8 imx219_i2c_addr;
extern const CVI_U32 imx219_addr_byte;
extern const CVI_U32 imx219_data_byte;
extern void imx219_init(VI_PIPE ViPipe);
extern void imx219_exit(VI_PIPE ViPipe);
extern void imx219_standby(VI_PIPE ViPipe);
extern void imx219_restart(VI_PIPE ViPipe);
extern int  imx219_write_register(VI_PIPE ViPipe, int addr, int data);
extern int  imx219_read_register(VI_PIPE ViPipe, int addr);
extern void imx219_mirror_flip(VI_PIPE ViPipe, ISP_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip);
extern int imx219_probe(VI_PIPE ViPipe);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __IMX219_CMOS_EX_H_ */

