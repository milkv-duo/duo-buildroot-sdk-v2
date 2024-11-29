#ifndef __PR2000_CMOS_EX_H_
#define __PR2000_CMOS_EX_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#include <linux/cvi_type.h>
#include "cvi_sns_ctrl.h"

typedef enum _PR2000_MODE_E {
	PR2000_MODE_NONE,
	PR2000_MODE_720H_NTSC,
	PR2000_MODE_720H_PAL,
	PR2000_MODE_720P_25,
	PR2000_MODE_720P_30,
	PR2000_MODE_720P_50,
	PR2000_MODE_720P_60,
	PR2000_MODE_1080P_25,
	PR2000_MODE_1080P_30,
	PR2000_MODE_NUM
} PR2000_MODE_E;

typedef struct _PR2000_MODE_S {
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
} PR2000_MODE_S;

/****************************************************************************
 * external variables and functions                                         *
 ****************************************************************************/

extern ISP_SNS_STATE_S *g_pastPr2000[VI_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U g_aunPr2000_BusInfo[];
extern const CVI_U8 pr2000_i2c_addr;
extern const CVI_U32 pr2000_addr_byte;
extern const CVI_U32 pr2000_data_byte;
extern void pr2000_init(VI_PIPE ViPipe);
extern void pr2000_exit(VI_PIPE ViPipe);
extern void pr2000_standby(VI_PIPE ViPipe);
extern void pr2000_restart(VI_PIPE ViPipe);
extern int  pr2000_write_register(VI_PIPE ViPipe, int addr, int data);
extern int  pr2000_read_register(VI_PIPE ViPipe, int addr);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __PR2000_CMOS_EX_H_ */
