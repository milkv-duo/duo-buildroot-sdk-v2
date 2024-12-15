#ifndef __tp2825_CMOS_PARAM_H_
#define __tp2825_CMOS_PARAM_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#include <linux/cvi_type.h>
#include "cvi_sns_ctrl.h"
#include "tp2825_cmos_ex.h"

static const tp2825_MODE_S g_asttp2825_mode[TP2825_MODE_NUM] = {
	[TP2825_MODE_1440P_30P] = {
		.name = "1440p30",
		.astImg[0] = {
			.stSnsSize = {
				.u32Width = 2560,
				.u32Height = 1440,
			},
			.stWndRect = {
				.s32X = 0,
				.s32Y = 0,
				.u32Width = 2560,
				.u32Height = 1440,
			},
			.stMaxSize = {
				.u32Width = 2560,
				.u32Height = 1440,
			},
		},
	},
	[TP2825_MODE_1440P_25P] = {
		.name = "1440p30",
		.astImg[0] = {
			.stSnsSize = {
				.u32Width = 2560,
				.u32Height = 1440,
			},
			.stWndRect = {
				.s32X = 0,
				.s32Y = 0,
				.u32Width = 2560,
				.u32Height = 1440,
			},
			.stMaxSize = {
				.u32Width = 2560,
				.u32Height = 1440,
			},
		},
	},
};

struct combo_dev_attr_s tp2825_rx_attr = {
	.input_mode = INPUT_MODE_BT1120,
	.mac_clk = RX_MAC_CLK_400M,
	.mclk = {
		.cam = 0,
		.freq = CAMPLL_FREQ_NONE,
	},
	.ttl_attr = {
		.vi = TTL_VI_SRC_VI1,
		.func = {
			-1, -1, -1, -1,
			0, 1, 2, 3, 4, 5, 6, 7,
			12, 11, 14, 13,
			8, 15, 10, 9,
		},
	},
	.devno = 0,
};

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __tp2825_CMOS_PARAM_H_ */
