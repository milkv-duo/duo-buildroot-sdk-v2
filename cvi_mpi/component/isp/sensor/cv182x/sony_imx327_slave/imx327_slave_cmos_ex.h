#ifndef __IMX327_SLAVE_CMOS_EX_H_
#define __IMX327_SLAVE_CMOS_EX_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#include <linux/cvi_type.h>
#include "cvi_sns_ctrl.h"

enum imx327_slave_linear_regs_e {
	LINEAR_HOLD = 0,
	LINEAR_SHS1_0,
	LINEAR_SHS1_1,
	LINEAR_SHS1_2,
	LINEAR_GAIN,
	LINEAR_HCG,
	LINEAR_VMAX_0,
	LINEAR_VMAX_1,
	LINEAR_VMAX_2,
	LINEAR_REL,
	LINEAR_REGS_NUM
};

enum imx327_slave_dol2_regs_e {
	DOL2_HOLD = 0,
	DOL2_SHS1_0,
	DOL2_SHS1_1,
	DOL2_SHS1_2,
	DOL2_GAIN,
	DOL2_HCG,
	DOL2_GAIN1,
	DOL2_RHS1_0,
	DOL2_RHS1_1,
	DOL2_RHS1_2,
	DOL2_SHS2_0,
	DOL2_SHS2_1,
	DOL2_SHS2_2,
	DOL2_VMAX_0,
	DOL2_VMAX_1,
	DOL2_VMAX_2,
	DOL2_YOUT_SIZE_0,
	DOL2_YOUT_SIZE_1,
	DOL2_REL,
	DOL2_REGS_NUM
};

typedef enum _IMX327_SLAVE_MODE_E {
	IMX327_SLAVE_MODE_1080P30 = 0,
	IMX327_SLAVE_MODE_LINEAR_NUM,
	IMX327_SLAVE_MODE_1080P30_WDR = IMX327_SLAVE_MODE_LINEAR_NUM,
	IMX327_SLAVE_MODE_NUM
} IMX327_SLAVE_MODE_E;

typedef struct _IMX327_SLAVE_STATE_S {
	CVI_U8       u8Hcg;
	CVI_U32      u32BRL;
	CVI_U32      u32RHS1;
	CVI_U32      u32RHS1_MAX;
} IMX327_SLAVE_STATE_S;

typedef struct _IMX327_SLAVE_MODE_S {
	ISP_WDR_SIZE_S astImg[2];
	CVI_FLOAT f32MaxFps;
	CVI_FLOAT f32MinFps;
	CVI_U32 u32HtsDef;
	CVI_U32 u32VtsDef;
	SNS_ATTR_S stExp[2];
	SNS_ATTR_S stAgain[2];
	SNS_ATTR_S stDgain[2];
	CVI_U16 u16RHS1;
	CVI_U16 u16BRL;
	CVI_U16 u16OpbSize;
	CVI_U16 u16MarginVtop;
	CVI_U16 u16MarginVbot;
	char name[64];
} IMX327_SLAVE_MODE_S;

/****************************************************************************
 * external variables and functions                                         *
 ****************************************************************************/

extern ISP_SNS_STATE_S *g_pastImx327_Slave[VI_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U g_aunImx327_Slave_BusInfo[];
extern CVI_U16 g_au16Imx327_Slave_GainMode[];
extern CVI_U16 g_au16Imx327_Slave_UseHwSync[];
extern const CVI_U8 imx327_slave_i2c_addr;
extern const CVI_U32 imx327_slave_addr_byte;
extern const CVI_U32 imx327_slave_data_byte;
extern void imx327_slave_init(VI_PIPE ViPipe);
extern void imx327_slave_exit(VI_PIPE ViPipe);
extern void imx327_slave_standby(VI_PIPE ViPipe);
extern void imx327_slave_restart(VI_PIPE ViPipe);
extern int  imx327_slave_write_register(VI_PIPE ViPipe, int addr, int data);
extern int  imx327_slave_read_register(VI_PIPE ViPipe, int addr);
extern void imx327_slave_mirror_flip(VI_PIPE ViPipe, ISP_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip);
extern int  imx327_slave_probe(VI_PIPE ViPipe);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __IMX327_SLAVE_CMOS_EX_H_ */
