#ifndef __GC2385_1L_CMOS_EX_H_
#define __GC2385_1L_CMOS_EX_H_

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

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

enum gc2385_1l_linear_regs_e {
	LINEAR_EXP_PAGE = 0,
	LINEAR_EXP_H,
	LINEAR_EXP_L,
	LINEAR_VB_PAGE,
	LINEAR_VB_H,
	LINEAR_VB_L,
	LINEAR_FLIP_MIRROR_PAGE,
	LINEAR_FLIP_MIRROR,
	LINEAR_CROP_START_X,
	LINEAR_CROP_START_Y,
	LINEAR_BLK_Select1_H,
	LINEAR_BLK_Select1_L,
	LINEAR_GAIN_PAGE,
	LINEAR_T_INIT_SET,
	LINEAR_T_CLK_HS,
	LINEAR_COL_CODE,
	LINEAR_AUTO_PREGAIN_SYNC,
	LINEAR_AUTO_PREGAIN,
	LINEAR_REGS_NUM
};


typedef enum _GC2385_1L_MODE_E {
	GC2385_1L_MODE_1600X1200P30 = 0,
	GC2385_1L_MODE_NUM
} GC2385_1L_MODE_E;

typedef struct _GC2385_1L_STATE_S {
	CVI_U32		u32Sexp_MAX;
} GC2385_1L_STATE_S;

typedef struct _GC2385_1L_MODE_S {
	ISP_WDR_SIZE_S stImg;
	CVI_FLOAT f32MaxFps;
	CVI_FLOAT f32MinFps;
	CVI_U32 u32HtsDef;
	CVI_U32 u32VtsDef;
	SNS_ATTR_S stExp;
	SNS_ATTR_LARGE_S stAgain;
	SNS_ATTR_LARGE_S stDgain;
	char name[64];
} GC2385_1L_MODE_S;

/****************************************************************************
 * external variables and functions                                         *
 ****************************************************************************/

extern ISP_SNS_STATE_S *g_pastGc2385_1L[VI_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U g_aunGc2385_1L_BusInfo[];
extern ISP_SNS_MIRRORFLIP_TYPE_E g_aeGc2385_1L_MirrorFip[VI_MAX_PIPE_NUM];
extern const CVI_U8 gc2385_1l_i2c_addr;
extern const CVI_U32 gc2385_1l_addr_byte;
extern const CVI_U32 gc2385_1l_data_byte;
extern void gc2385_1l_init(VI_PIPE ViPipe);
extern void gc2385_1l_exit(VI_PIPE ViPipe);
extern void gc2385_1l_standby(VI_PIPE ViPipe);
extern void gc2385_1l_restart(VI_PIPE ViPipe);
extern int  gc2385_1l_write_register(VI_PIPE ViPipe, int addr, int data);
extern int  gc2385_1l_read_register(VI_PIPE ViPipe, int addr);
extern int  gc2385_1l_probe(VI_PIPE ViPipe);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __GC2385_1L_CMOS_EX_H_ */

