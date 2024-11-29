#ifndef __OS02N10_1L_CMOS_EX_H_
#define __OS02N10_1L_CMOS_EX_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include <linux/cvi_type.h>
#include "cvi_sns_ctrl.h"

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

enum os02n10_1l_linear_regs_e {
	LINEAR_PAGE_SWITCH,
	LINEAR_EXP_0,
	LINEAR_EXP_1,
	LINEAR_EXP_TRIGGER,
	LINEAR_AGAIN,
	LINEAR_DGAIN_0,
	LINEAR_DGAIN_1,
	LINEAR_GAIN_TRIGGER,
	LINEAR_VB_0,
	LINEAR_VB_1,
	LINEAR_VB_TRIGGER,
	LINEAR_FLIP_MIRROR,
	LINEAR_FLIP_PAGE_SWITCH,
	LINEAR_FLIP_MIRROR_0,
	LINEAR_FLIP_MIRROR_1,
	LINEAR_FLIP_MIRROR_2,
	LINEAR_COL_OFFSET,
	LINEAR_ROW_OFFSET_0,
	LINEAR_ROW_OFFSET_1,
	LINEAR_REGS_NUM
};

typedef enum _OS02N10_1L_MODE_E {
	OS02N10_1L_MODE_1080P15= 0,
	OS02N10_1L_MODE_LINEAR_NUM,
	OS02N10_1L_MODE_NUM
} OS02N10_1L_MODE_E;

typedef struct _OS02N10_1L_STATE_S {
	CVI_U32		u32Sexp_MAX;
} OS02N10_1L_STATE_S;

typedef struct _OS02N10_1L_MODE_S {
	ISP_WDR_SIZE_S astImg[2];
	CVI_FLOAT f32MaxFps;
	CVI_FLOAT f32MinFps;
	CVI_U32 u32HtsDef;
	CVI_U32 u32VtsDef;
	SNS_ATTR_S stExp[2];
	SNS_ATTR_LARGE_S stAgain[2];
	SNS_ATTR_LARGE_S stDgain[2];
	char name[64];
	CVI_U32 u32L2S_offset;
	CVI_U32 u32IspResTime;
	CVI_U32 u32VStart;
	CVI_U32 u32VEnd;
} OS02N10_1L_MODE_S;

/****************************************************************************
 * external variables and functions                                         *
 ****************************************************************************/

extern ISP_SNS_STATE_S *g_pastOs02n10_1l[VI_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U g_aunOs02n10_1l_BusInfo[];
extern CVI_U16 g_au16Os02n10_1l_GainMode[];
extern CVI_U16 g_au16Os02n10_1l_UseHwSync[VI_MAX_PIPE_NUM];
extern CVI_U8 os02n10_1l_i2c_addr;
extern const CVI_U32 os02n10_1l_addr_byte;
extern const CVI_U32 os02n10_1l_data_byte;
extern void os02n10_1l_init(VI_PIPE ViPipe);
extern void os02n10_1l_exit(VI_PIPE ViPipe);
extern void os02n10_1l_standby(VI_PIPE ViPipe);
extern void os02n10_1l_restart(VI_PIPE ViPipe);
extern int  os02n10_1l_write_register(VI_PIPE ViPipe, int addr, int data);
extern int  os02n10_1l_read_register(VI_PIPE ViPipe, int addr);
extern int  os02n10_1l_probe(VI_PIPE ViPipe);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __OS02N10_1L_CMOS_EX_H_ */
