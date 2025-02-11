#ifndef __SC035GS_CMOS_EX_H_
#define __SC035GS_CMOS_EX_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#include <linux/cvi_type.h>
#include "cvi_sns_ctrl.h"


enum sc035gs_linear_regs_e {
	LINEAR_EXP_H_ADDR,
	LINEAR_EXP_L_ADDR,
	LINEAR_AGAIN_H_ADDR,
	LINEAR_AGAIN_L_ADDR,
	LINEAR_DGAIN_H_ADDR,
	LINEAR_DGAIN_L_ADDR,
	LINEAR_VMAX_H_ADDR,
	LINEAR_VMAX_L_ADDR,
	LINEAR_GAIN_MAGIC_0_ADDR,
	LINEAR_GAIN_MAGIC_1_ADDR,
	LINEAR_REGS_NUM
};

typedef enum _SC035GS_MODE_E {
	SC035GS_MODE_640X480P120 = 0,
	SC035GS_MODE_LINEAR_NUM,
	SC035GS_MODE_NUM
} SC035GS_MODE_E;

typedef struct _SC035GS_MODE_S {
	ISP_WDR_SIZE_S astImg[2];
	CVI_FLOAT f32MaxFps;
	CVI_FLOAT f32MinFps;
	CVI_U32 u32HtsDef;
	CVI_U32 u32VtsDef;
	SNS_ATTR_S stExp[2];
	SNS_ATTR_LARGE_S stAgain[2];
	SNS_ATTR_LARGE_S stDgain[2];
	char name[64];
} SC035GS_MODE_S;

/****************************************************************************
 * external variables and functions                                         *
 ****************************************************************************/

extern ISP_SNS_STATE_S *g_pastSC035GS[VI_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U g_aunSC035GS_BusInfo[];
extern CVI_U16 g_au16SC035GS_GainMode[];
extern CVI_U16 g_au16SC035GS_L2SMode[];
extern const CVI_U8 sc035gs_i2c_addr;
extern const CVI_U32 sc035gs_addr_byte;
extern const CVI_U32 sc035gs_data_byte;
extern void sc035gs_init(VI_PIPE ViPipe);
extern void sc035gs_exit(VI_PIPE ViPipe);
extern void sc035gs_standby(VI_PIPE ViPipe);
extern void sc035gs_restart(VI_PIPE ViPipe);
extern int  sc035gs_write_register(VI_PIPE ViPipe, int addr, int data);
extern int  sc035gs_read_register(VI_PIPE ViPipe, int addr);
extern void sc035gs_mirror_flip(VI_PIPE ViPipe, ISP_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __SC035GS_CMOS_EX_H_ */
