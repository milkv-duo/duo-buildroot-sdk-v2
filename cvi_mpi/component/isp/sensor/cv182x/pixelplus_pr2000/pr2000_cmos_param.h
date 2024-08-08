#ifndef __PR2000_CMOS_PARAM_H_
#define __PR2000_CMOS_PARAM_H_

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
#include "pr2000_cmos_ex.h"

static const PR2000_MODE_S g_astPr2000_mode[PR2000_MODE_NUM] = {
	[PR2000_MODE_720H_PAL] = {
		.name = "720hpal",
		.astImg[0] = {
			.stSnsSize = {
				.u32Width = 720,
				.u32Height = 288,
			},
			.stWndRect = {
				.s32X = 0,
				.s32Y = 0,
				.u32Width = 720,
				.u32Height = 288,
			},
			.stMaxSize = {
				.u32Width = 720,
				.u32Height = 288,
			},
		},
	},
	[PR2000_MODE_720P_25] = {
		.name = "720p25",
		.astImg[0] = {
			.stSnsSize = {
				.u32Width = 720,
				.u32Height = 288,
			},
			.stWndRect = {
				.s32X = 0,
				.s32Y = 0,
				.u32Width = 720,
				.u32Height = 288,
			},
			.stMaxSize = {
				.u32Width = 720,
				.u32Height = 288,
			},
		},
	},
	[PR2000_MODE_720P_30] = {
		.name = "720p30",
		.astImg[0] = {
			.stSnsSize = {
				.u32Width = 720,
				.u32Height = 288,
			},
			.stWndRect = {
				.s32X = 0,
				.s32Y = 0,
				.u32Width = 720,
				.u32Height = 288,
			},
			.stMaxSize = {
				.u32Width = 720,
				.u32Height = 288,
			},
		},
	},
	[PR2000_MODE_1080P_25] = {
		.name = "1080p25",
		.astImg[0] = {
			.stSnsSize = {
				.u32Width = 1920,
				.u32Height = 1080,
			},
			.stWndRect = {
				.s32X = 0,
				.s32Y = 0,
				.u32Width = 1920,
				.u32Height = 1080,
			},
			.stMaxSize = {
				.u32Width = 1920,
				.u32Height = 1080,
			},
		},
	},
	[PR2000_MODE_1080P_30] = {
		.name = "1080p30",
		.astImg[0] = {
			.stSnsSize = {
				.u32Width = 1920,
				.u32Height = 1080,
			},
			.stWndRect = {
				.s32X = 0,
				.s32Y = 0,
				.u32Width = 1920,
				.u32Height = 1080,
			},
			.stMaxSize = {
				.u32Width = 1920,
				.u32Height = 1080,
			},
		},
	},
};

struct combo_dev_attr_s pr2000_rx_attr = {
	.input_mode = INPUT_MODE_MIPI,
	.mac_clk = RX_MAC_CLK_200M,//
	.mipi_attr = {
		.raw_data_type = YUV422_8BIT,
		.lane_id = {0, 4, 3, -1, -1},
		.wdr_mode = CVI_MIPI_WDR_MODE_NONE,
		.dphy = {
			.enable = 1,
			.hs_settle = 8,//
		},
	},
	.mclk = {
		.cam = 0,
		.freq = CAMPLL_FREQ_NONE,
	},
	.devno = 0,
};

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __PR2000_CMOS_PARAM_H_ */
