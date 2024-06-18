/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_dis.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_DIS_H_
#define _ISP_ALGO_DIS_H_

#include "cvi_type.h"
#include "isp_defines.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define DIS_SENSOR_NUM (2)
// Configurable

#define HIST_RDY_NUM 2
#define DIS_RDY_NUM  6

#define DIS_STB_Q_S	(4)

// By chip
#define MAX_XHIST_BITS			16
#define MAX_YHIST_BITS			17
#define MAX_MVX_JITTER_RANGE	120		// +- 120 pixels motion
#define MAX_MVY_JITTER_RANGE	120		// +- 120 pixels motion

#define QUEBUF_LENGTH			20

#define LIMIT_RANGE_DIS(v, min, max)                                                                                   \
	({                                                                                                             \
		typeof(v) _v = (v);                                                                                    \
		typeof(min) _min = (min);                                                                              \
		typeof(max) _max = (max);                                                                              \
		_v = (_v < _min) ? _min : _v;                                                                          \
		_v = (_v > _max) ? _max : _v;                                                                          \
	})

#ifndef MIN2
#define MIN2(a, b)                                                                                                     \
	({                                                                                                             \
		__typeof__(a) _a = (a);                                                                                \
		__typeof__(b) _b = (b);                                                                                \
		_a < _b ? _a : _b;                                                                                     \
	})
#endif

#ifndef MAX2
#define MAX2(a, b)                                                                                                     \
	({                                                                                                             \
		__typeof__(a) _a = (a);                                                                                \
		__typeof__(b) _b = (b);                                                                                \
		_a > _b ? _a : _b;                                                                                     \
	})
#endif

typedef struct COORDINATE2D_INT {
	CVI_S16 xcor;
	CVI_S16 ycor;
} COORDINATE2D_INT;

typedef struct REG {
	CVI_S16 reg_gms_x_gap;
	CVI_S16 reg_gms_y_gap;
	CVI_S16 reg_gms_start_x;
	CVI_S16 reg_gms_start_y;
	CVI_S16 reg_gms_x_section_size;
	CVI_S16 reg_gms_y_section_size;
} REG;

typedef struct REG_DIS {

	CVI_U16 reg_sw_DIS_en;
	CVI_U16 reg_MARGIN_INI_U;
	CVI_U16 reg_MARGIN_INI_D;
	CVI_U16 reg_MARGIN_INI_L;
	CVI_U16 reg_MARGIN_INI_R;

	CVI_U16 reg_HIST_NORMX;
	CVI_U16 reg_HIST_NORMY;
	CVI_U16 reg_XHIST_LENGTH;
	CVI_U16 reg_YHIST_LENGTH;

	CVI_U16 reg_MAX_MVX_JITTER_RANGE;
	CVI_U16 reg_MAX_MVY_JITTER_RANGE;

	CVI_S16 reg_SAD_CONFRM_RANGEX;
	CVI_S16 reg_SAD_CONFRM_RANGEY;

	CVI_U16 reg_BLOCK_LENGTH_X;
	CVI_U16 reg_BLOCK_LENGTH_Y;

	CVI_U16 reg_cfrmsad_norm;
	CVI_U16 reg_cfrmsad_max;
	CVI_U16 reg_cfrmgain_xcoring;
	CVI_U16 reg_cfrmgain_ycoring;
	CVI_U16 reg_cfrmsad_xcoring;
	CVI_U16 reg_cfrmsad_ycoring;
	CVI_U16 reg_cfrmflag_xthd;
	CVI_U16 reg_cfrmflag_ythd;
	CVI_U16 reg_tmp_maxval_diff_thd;
	CVI_U16 reg_tmp_maxval_gain;
	CVI_U16 reg_xhist_cplx_thd;
	CVI_U16 reg_yhist_cplx_thd;

	CVI_S16 reg_force_offset_en;
	CVI_S16 reg_force_xoffset;
	CVI_S16 reg_force_yoffset;
} REG_DIS;

typedef struct _DIS_STATS_INFO {
	CVI_U32 frameCnt;
	CVI_U16 histX[DIS_MAX_WINDOW_X_NUM][DIS_MAX_WINDOW_Y_NUM][XHIST_LENGTH];
	CVI_U16 histY[DIS_MAX_WINDOW_X_NUM][DIS_MAX_WINDOW_Y_NUM][YHIST_LENGTH];
} DIS_STATS_INFO;

typedef struct _DIS_MAIN_INPUT_S {
	//DIS_STATS_INFO *disStt;
	ISP_VOID_PTR disStt;

	REG greg;
	REG_DIS greg_dis;
} DIS_MAIN_INPUT_S;

typedef struct _DIS_MAIN_OUTPUT_S {
	COORDINATE2D_INT coordiate;
} DIS_MAIN_OUTPUT_S;

typedef struct MV_STRUCT {
	CVI_FLOAT x;
	CVI_FLOAT y;
} MV_STRUCT;

typedef struct DIS_HIST {
	CVI_U16 xhist_d[XHIST_LENGTH];
	CVI_U16 yhist_d[YHIST_LENGTH];
} DIS_HIST;

typedef struct DIS_RGN_RAWMV {

	CVI_S32 x;
	CVI_S32 y;
	CVI_BOOL scenechange;

	CVI_S32 xavgSAD;
	CVI_S32 yavgSAD;

	CVI_S32 xbestSAD;
	CVI_S32 ybestSAD;
	CVI_S32 xMaxSAD;
	CVI_S32 yMaxSAD;
	CVI_S32 xCfrmval;
	CVI_S32 yCfrmval;

	CVI_BOOL xcfrmflag;
	CVI_BOOL ycfrmflag;
	CVI_BOOL xflatness;
	CVI_BOOL yflatness;

} DIS_RGN_RAWMV;

typedef struct _WINXY_DIS_HIST {
	DIS_HIST wHist[DIS_MAX_WINDOW_Y_NUM][DIS_MAX_WINDOW_X_NUM];
} WINXY_DIS_HIST;

typedef struct _DIS_RUNTIME {
	WINXY_DIS_HIST *curhist;
	WINXY_DIS_HIST *prvhist;

	WINXY_DIS_HIST _cur;
	WINXY_DIS_HIST _prv;

	MV_STRUCT MV_BUF[QUEBUF_LENGTH];
	MV_STRUCT accsum[QUEBUF_LENGTH];
	MV_STRUCT smoothAcc[QUEBUF_LENGTH];
	DIS_RGN_RAWMV rgn_prvMV[DIS_MAX_WINDOW_Y_NUM][DIS_MAX_WINDOW_X_NUM];
	DIS_RGN_RAWMV rgn_curMV[DIS_MAX_WINDOW_Y_NUM][DIS_MAX_WINDOW_X_NUM];
	CVI_FLOAT accsumX;
	CVI_FLOAT accsumY;

	MV_STRUCT OFFSET[QUEBUF_LENGTH];
	MV_STRUCT GMV[QUEBUF_LENGTH];
	// flow control parameters
	CVI_S32 hist_ready_cnt;
	CVI_S32 dis_ready_cnt;

	// MV r/w idx
	CVI_U32 curpos_exe;			// cur-display position	of ring buffer.
	CVI_U32 curpos_wrt;			// cur-ME updating position of ring buffer
	CVI_S32 drop_end_cnt;			// if drop_end_cnt != 0 -> DIS drop.
} DIS_RUNTIME;

typedef enum _DIS_DIR_TYPE_E {
	DIS_DIR_X = 0x0,
	DIS_DIR_Y
} DIS_DIR_TYPE_E;

#define DIS_FLK_NUM_PER_Y (4)
#define DIS_FLK_NUM_FRM (6)
#define DIS_FLK_Y_SHT (6)//div YHIST_LENGTH to DIS_FLK_NUM_PER_Y part

struct S_DIS_FLICKER {
	CVI_BOOL ena;
	CVI_U8 flickSts;
	CVI_U8 curInx;
	CVI_S8 lastQ;
	CVI_U32 flicker[DIS_FLK_NUM_FRM][DIS_MAX_WINDOW_Y_NUM * DIS_FLK_NUM_PER_Y];
};

CVI_S32 disMainInit(VI_PIPE ViPipe);
CVI_S32 disMainUnInit(VI_PIPE ViPipe);
CVI_S32 disMain(VI_PIPE ViPipe, DIS_MAIN_INPUT_S *inPtr, DIS_MAIN_OUTPUT_S *outPtr);
CVI_S32 disAlgoCreate(VI_PIPE ViPipe);
CVI_S32 disAlgoDestroy(VI_PIPE ViPipe);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_ALGO_DIS_H_
