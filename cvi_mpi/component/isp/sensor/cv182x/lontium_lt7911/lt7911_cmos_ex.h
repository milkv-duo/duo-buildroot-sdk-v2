#ifndef __LT7911_CMOS_EX_H_
#define __LT7911_CMOS_EX_H_

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


enum lt7911_linear_regs_e {
	LINEAR_REGS_NUM
};


typedef enum _LT7911_MODE_E {
	LT7911_MODE_NONE,
	LT7911_MODE_NORMAL,
	LT7911_MODE_NUM
} LT7911_MODE_E;


typedef struct _LT7911_MODE_S {
	ISP_WDR_SIZE_S astImg[2];
	char name[64];
} LT7911_MODE_S;


extern CVI_U8 lt7911_i2c_addr;
extern const CVI_U32 lt7911_addr_byte;
extern const CVI_U32 lt7911_data_byte;
extern void lt7911_init(VI_PIPE ViPipe);
extern void lt7911_exit(VI_PIPE ViPipe);
extern void lt7911_standby(VI_PIPE ViPipe);
extern void lt7911_restart(VI_PIPE ViPipe);
extern int  lt7911_write(VI_PIPE ViPipe, int addr, int data);
extern int  lt7911_read(VI_PIPE ViPipe, int addr);
extern void lt7911_mirror_flip(VI_PIPE ViPipe, ISP_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip);
extern int  lt7911_probe(VI_PIPE ViPipe);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __LT7911_CMOS_EX_H_ */