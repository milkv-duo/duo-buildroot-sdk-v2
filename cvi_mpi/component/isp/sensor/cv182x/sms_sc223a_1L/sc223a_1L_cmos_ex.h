#ifndef __SC223A_1L_CMOS_EX_H_
#define __SC223A_1L_CMOS_EX_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#include <linux/cvi_type.h>
#include "cvi_sns_ctrl.h"


enum sc223a_1l_linear_regs_e {
	LINEAR_SHS1_0_ADDR,
	LINEAR_SHS1_1_ADDR,
	LINEAR_SHS1_2_ADDR,
	LINEAR_AGAIN_0_ADDR,
	LINEAR_DGAIN_0_ADDR,
	LINEAR_DGAIN_1_ADDR,
	LINEAR_VMAX_0_ADDR,
	LINEAR_VMAX_1_ADDR,
	LINEAR_REGS_NUM
};


typedef enum _SC223A_1L_MODE_E {
	SC223A_1L_MODE_1080P30 = 0,
	SC223A_1L_MODE_LINEAR_NUM,
	SC223A_1L_MODE_NUM
} SC223A_1L_MODE_E;

typedef struct _SC223A_1L_STATE_S {
	CVI_U32 u32Sexp_MAX;
} SC223A_1L_STATE_S;

typedef struct _SC223A_1L_MODE_S {
	ISP_WDR_SIZE_S astImg[2];
	CVI_FLOAT f32MaxFps;
	CVI_FLOAT f32MinFps;
	CVI_U32 u32HtsDef;
	CVI_U32 u32VtsDef;
	SNS_ATTR_S stExp[2];
	SNS_ATTR_S stAgain[2];
	SNS_ATTR_S stDgain[2];
	CVI_U16 u16SexpMaxReg;
	char name[64];
} SC223A_1L_MODE_S;

/****************************************************************************
 * external variables and functions                                         *
 ****************************************************************************/

extern ISP_SNS_STATE_S *g_pastSC223A_1L[VI_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U g_aunSC223A_1L_BusInfo[];
extern CVI_U16 g_au16SC223A_1L_GainMode[];
extern CVI_U16 g_au16SC223A_1L_L2SMode[];
extern CVI_U8 sc223a_1l_i2c_addr;
extern const CVI_U32 sc223a_1l_addr_byte;
extern const CVI_U32 sc223a_1l_data_byte;
extern void sc223a_1l_init(VI_PIPE ViPipe);
extern void sc223a_1l_exit(VI_PIPE ViPipe);
extern void sc223a_1l_standby(VI_PIPE ViPipe);
extern void sc223a_1l_restart(VI_PIPE ViPipe);
extern int  sc223a_1l_write_register(VI_PIPE ViPipe, int addr, int data);
extern int  sc223a_1l_read_register(VI_PIPE ViPipe, int addr);
extern void sc223a_1l_mirror_flip(VI_PIPE ViPipe, ISP_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip);
extern int  sc223a_1l_probe(VI_PIPE ViPipe);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __SC223A_1L_CMOS_EX_H_ */
