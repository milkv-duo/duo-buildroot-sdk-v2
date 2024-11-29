#ifndef __SC132GS_CMOS_EX_H_
#define __SC132GS_CMOS_EX_H_

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
#define SC132GS_I2C_ADDR_1 0x30
// #define SC132GS_I2C_ADDR_2 0x30
#define syslog(level, fmt, ...)	printf(fmt, ##__VA_ARGS__)

enum sc132gs_linear_regs_e {
	LINEAR_SHS1_0_ADDR,
	LINEAR_SHS1_1_ADDR,
	LINEAR_SHS1_2_ADDR,
	LINEAR_AGAIN_0_ADDR,
	LINEAR_AGAIN_1_ADDR,
	LINEAR_DGAIN_0_ADDR,
	LINEAR_DGAIN_1_ADDR,
	LINEAR_VMAX_0_ADDR,
	LINEAR_VMAX_1_ADDR,
	LINEAR_LOGIC_0_ADDR,
	LINEAR_LOGIC_1_ADDR,

	LINEAR_REGS_NUM
};

typedef enum _SC132GS_MODE_E {
	SC132GS_MODE_1080X1280P60 = 0,
	SC132GS_MODE_NUM
} SC132GS_MODE_E;

typedef struct _SC132GS_STATE_S {
	CVI_U32		u32Sexp_MAX;
} SC132GS_STATE_S;

typedef struct _SC132GS_MODE_S {
	ISP_WDR_SIZE_S astImg[2];
	CVI_FLOAT f32MaxFps;
	CVI_FLOAT f32MinFps;
	CVI_U32 u32HtsDef;
	CVI_U32 u32VtsDef;
	SNS_ATTR_S stExp[2];
	SNS_ATTR_LARGE_S stAgain[2];
	SNS_ATTR_LARGE_S stDgain[2];
	char name[64];
} SC132GS_MODE_S;

/****************************************************************************
 * external variables and functions                                         *
 ****************************************************************************/

extern ISP_SNS_STATE_S *g_pastSc132gs[VI_MAX_PIPE_NUM];
extern ISP_SNS_COMMBUS_U g_aunSc132gs_BusInfo[];
extern ISP_SNS_MIRRORFLIP_TYPE_E g_aeSc132gs_MirrorFip[VI_MAX_PIPE_NUM];
extern CVI_U8 sc132gs_i2c_addr;
extern CVI_U8 sc132gs_i2c_addr_list[];
extern CVI_S8 sc132gs_i2c_dev_list[];
extern CVI_U8 sc132gs_cur_idx;
extern const CVI_U32 sc132gs_addr_byte;
extern const CVI_U32 sc132gs_data_byte;
extern void sc132gs_init(VI_PIPE ViPipe);
extern void sc132gs_exit(VI_PIPE ViPipe);
extern void sc132gs_standby(VI_PIPE ViPipe);
extern void sc132gs_restart(VI_PIPE ViPipe);
extern void sc132gs_mirror_flip(VI_PIPE ViPipe, ISP_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip);
extern int  sc132gs_write_register(VI_PIPE ViPipe, int addr, int data);
extern int  sc132gs_read_register(VI_PIPE ViPipe, int addr);
extern int  sc132gs_probe(VI_PIPE ViPipe);
extern int  sc132gs_set_i2c_cfg(VI_PIPE ViPipe, CVI_U8 i2c_addr, CVI_S8 i2c_dev);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __SC132GS_CMOS_EX_H_ */

