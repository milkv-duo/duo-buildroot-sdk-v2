#ifndef __IMX675_CMOS_PARAM_H_
#define __IMX675_CMOS_PARAM_H_

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
#include "imx675_cmos_ex.h"

static const IMX675_MODE_S g_astImx675_mode[IMX675_MODE_NUM] = {
	[IMX675_MODE_5M30] = {
		.name = "5M30",
		.astImg[0] = {
			.stSnsSize = {
				.u32Width = 2608,
				.u32Height = 1964,
			},
			.stWndRect = {
				.s32X = 0,
				.s32Y = 0,
				.u32Width = 2560,
				.u32Height = 1944,
			},
			.stMaxSize = {
				.u32Width = 2608,
				.u32Height = 1964,
			},
		},
		.f32MaxFps = 30,
		.f32MinFps = 0.0066,
		.u32HtsDef = 0x1130,
		.u32VtsDef = 3666,
		.stExp[0] = {
			.u16Min = 1,
			.u16Max = 1123,
			.u16Def = 400,
			.u16Step = 1,
		},
		.stAgain[0] = {
			.u16Min = 1024,
			.u16Max = 32381,
			.u16Def = 1024,
			.u16Step = 1,
		},
		.stDgain[0] = {
			.u16Min = 1024,
			.u16Max = 65535,
			.u16Def = 1024,
			.u16Step = 1,
		},
		.u16RHS1 = 7,
		.u16BRL = 1984,
		.u16OpbSize = 10,
		.u16MarginVtop = 8,
		.u16MarginVbot = 9,
	},
	[IMX675_MODE_5M25_WDR] = {
		.name = "5M25wdr",
		/* sef */
		.astImg[0] = {
			.stSnsSize = {
				.u32Width = 2608,
				.u32Height = 1964,

			},
			.stWndRect = {
				.s32X = 0,
				.s32Y = 0,
				.u32Width = 2560,
				.u32Height = 1944,
			},
			.stMaxSize = {
				.u32Width = 2608,
				.u32Height = 1964,
			},
		},
		/* lef */
		.astImg[1] = {
			.stSnsSize = {
				.u32Width = 2608,
				.u32Height = 1964,
			},
			.stWndRect = {
				.s32X = 0,
				.s32Y = 0,
				.u32Width = 2560,
				.u32Height = 1944,

			},
			.stMaxSize = {
				.u32Width = 2608,
				.u32Height = 1964,
			},
		},
		.f32MaxFps = 25,
		.f32MinFps = 0.0033, /* 2200 * 25 / 0xFFFFFF */
		.u32HtsDef = 0x0898,
		.u32VtsDef = 2200,
		.stExp[0] = {
			.u16Min = 1,
			.u16Max = 8,
			.u16Def = 8,
			.u16Step = 1,
		},
		.stExp[1] = {
			.u16Min = 1,
			.u16Max = 2236,
			.u16Def = 828,
			.u16Step = 1,
		},
		.stAgain[0] = {
			.u16Min = 1024,
			.u16Max = 62416,
			.u16Def = 1024,
			.u16Step = 1,
		},
		.stAgain[1] = {
			.u16Min = 1024,
			.u16Max = 62416,
			.u16Def = 1024,
			.u16Step = 1,
		},
		.stDgain[0] = {
			.u16Min = 1024,
			.u16Max = 38485,
			.u16Def = 1024,
			.u16Step = 1,
		},
		.stDgain[1] = {
			.u16Min = 1024,
			.u16Max = 38485,
			.u16Def = 1024,
			.u16Step = 1,
		},
		.u16RHS1 = 7,
		.u16BRL = 1984,
		.u16OpbSize = 7,
		.u16MarginVtop = 8,
		.u16MarginVbot = 9,
	},
};

static ISP_CMOS_BLACK_LEVEL_S g_stIspBlcCalibratio = {
	.bUpdate = CVI_TRUE,
	.blcAttr = {
		.Enable = 1,
		.enOpType = OP_TYPE_AUTO,
		.stManual = {50, 50, 50, 50, 0, 0, 0, 0
#ifdef ARCH_CV182X
			, 1088, 1088, 1088, 1088
#endif
		},
		.stAuto = {
			{50, 50, 50, 50, 50, 50, 50, 50, /*8*/50, 50, 50, 50, 50, 50, 50, 50},
			{50, 50, 50, 50, 50, 50, 50, 50, /*8*/50, 50, 50, 50, 50, 50, 50, 50},
			{50, 50, 50, 50, 50, 50, 50, 50, /*8*/50, 50, 50, 50, 50, 50, 50, 50},
			{50, 50, 50, 50, 50, 50, 50, 50, /*8*/50, 50, 50, 50, 50, 50, 50, 50},
			{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
			{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
#ifdef ARCH_CV182X
			{1089, 1089, 1089, 1089, 1088, 1088, 1088, 1088,
				/*8*/1085, 1084, 1098, 1122, 1204, 1365, 1365, 1365},
			{1089, 1089, 1089, 1089, 1088, 1088, 1088, 1088,
				/*8*/1085, 1084, 1098, 1122, 1204, 1365, 1365, 1365},
			{1089, 1089, 1089, 1089, 1088, 1088, 1088, 1088,
				/*8*/1085, 1084, 1098, 1122, 1204, 1365, 1365, 1365},
			{1089, 1089, 1089, 1089, 1088, 1088, 1088, 1088,
				/*8*/1085, 1084, 1098, 1122, 1204, 1365, 1365, 1365},
#endif
		},
	},
};

struct combo_dev_attr_s imx675_rx_attr = {
	.input_mode = INPUT_MODE_MIPI,
	.mac_clk = RX_MAC_CLK_400M,   //400 for linear  500 for wdr25
	.mipi_attr = {
		.raw_data_type = RAW_DATA_12BIT,
		.lane_id = {0, 1, 2, 3, 4},
		.pn_swap = {1, 1, 1, 1, 1},
		.wdr_mode = CVI_MIPI_WDR_MODE_NONE,
		.dphy = {
			.enable = 1,
			.hs_settle = 14,
		},
	},
	.mclk = {
		.cam = 0,
		.freq = CAMPLL_FREQ_27M,
	},
	.devno = 0,
};

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __IMX675_CMOS_PARAM_H_ */
