#ifndef __XS9922B_CMOS_PARAM_H_
#define __XS9922B_CMOS_PARAM_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include <linux/cvi_type.h>
#include "cvi_sns_ctrl.h"
#include "xs9922b_cmos_ex.h"

static const XS9922B_MODE_S g_astXS9922b_mode[XS9922B_MODE_NUM] = {
	[XS9922B_MODE_720P_1CH] = {
		.name = "720p25_1ch",
		.astImg[0] = {
			.stSnsSize = {
				.u32Width = 1280,
				.u32Height = 720,
			},
			.stWndRect = {
				.s32X = 0,
				.s32Y = 0,
				.u32Width = 1280,
				.u32Height = 720,
			},
			.stMaxSize = {
				.u32Width = 1280,
				.u32Height = 720,
			},
		},
	},

	[XS9922B_MODE_720P_2CH] = {
		.name = "720p25_2ch",
		.astImg[0] = {
			.stSnsSize = {
				.u32Width = 1280,
				.u32Height = 720,
			},
			.stWndRect = {
				.s32X = 0,
				.s32Y = 0,
				.u32Width = 1280,
			},
			.stMaxSize = {
				.u32Width = 1280,
				.u32Height = 720,
			},
		},
	},

	[XS9922B_MODE_720P_3CH] = {
		.name = "720p25_3ch",
		.astImg[0] = {
			.stSnsSize = {
				.u32Width = 1280,
				.u32Height = 720,
			},
			.stWndRect = {
				.s32X = 0,
				.s32Y = 0,
				.u32Width = 1280,
				.u32Height = 720,
			},
			.stMaxSize = {
				.u32Width = 1280,
				.u32Height = 720,
			},
		},
	},

	[XS9922B_MODE_720P_4CH] = {
		.name = "720p25_4ch",
		.astImg[0] = {
			.stSnsSize = {
				.u32Width = 1280,
				.u32Height = 720,
			},
			.stWndRect = {
				.s32X = 0,
				.s32Y = 0,
				.u32Width = 1280,
				.u32Height = 720,
			},
			.stMaxSize = {
				.u32Width = 1280,
				.u32Height = 720,
			},
		},
	},
};

struct combo_dev_attr_s xs9922b_multi_rx_attr = {
	.input_mode = INPUT_MODE_MIPI,
	.mac_clk = RX_MAC_CLK_600M,
	.mipi_attr = {
		.raw_data_type = YUV422_8BIT,
		.lane_id = {2, 0, 1, 3, 4},
		.pn_swap = {0, 0, 0, 0, 0},
		.wdr_mode = CVI_MIPI_WDR_MODE_VC,
		.demux = {
			.demux_en = 1,
			.vc_mapping = {0, 1, 2, 3},
		},
		.dphy = {
			.enable = 1,
			.hs_settle = 15,
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


#endif /* __XS9922B_CMOS_PARAM_H_ */
