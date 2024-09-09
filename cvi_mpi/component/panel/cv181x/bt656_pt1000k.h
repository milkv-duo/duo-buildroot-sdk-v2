#ifndef _BT656_PT1000K_H_
#define _BT656_PT1000K_H_

#include <linux/cvi_comm_vo.h>

const VO_BT_ATTR_S stpt1000kbt656cfg = {
	.pins = {
		.pin_num = 9,
		.d_pins = {
			{VO_SD1_CMD, VO_MUX_BT_DATA0},
			{VO_SD1_D0, VO_MUX_BT_DATA1},
			{VO_SD1_D1, VO_MUX_BT_DATA2},
			{VO_SD1_D2, VO_MUX_BT_DATA3},
			{VO_SD1_D3, VO_MUX_BT_DATA4},
			{VO_JTAG_CPU_TMS, VO_MUX_BT_DATA7},
			{VO_JTAG_CPU_TCK, VO_MUX_BT_DATA6},
			{VO_SD1_CLK, VO_MUX_BT_DATA5},
			{VO_MIPI_TXP2, VO_MUX_BT_CLK}
		}
	},
};

#endif // _BT656_PT1000K_H_