/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 */

#include <common.h>
#include <clk.h>
#include <asm/gpio.h>
//#include <asm/hardware.h>
#include <asm/io.h>
#include <linux/delay.h>

#include "reg.h"
#include "vip_common.h"
#include "scaler.h"
#include "cvi_i80_hw.h"
#include "dsi_phy.h"

#include "mmio.h"
#include "../../board/cvitek/cv181x/cv181x_reg.h"
#include "../../board/cvitek/cv181x/cv181x_reg_fmux_gpio.h"
#include "../../board/cvitek/cv181x/cv181x_pinlist_swconfig.h"

#define PINMUX_CONFIG(PIN_NAME, FUNC_NAME) \
		mmio_clrsetbits_32(PINMUX_BASE + FMUX_GPIO_FUNCSEL_##PIN_NAME, \
			FMUX_GPIO_FUNCSEL_##PIN_NAME##_MASK << FMUX_GPIO_FUNCSEL_##PIN_NAME##_OFFSET, \
			PIN_NAME##__##FUNC_NAME)

void _disp_sel_remux(const struct VO_D_REMAP *pins, unsigned int pin_num)
{
	int i = 0;

	for (i = 0; i < pin_num; ++i) {
		switch (pins[i].sel) {
		case VO_MIPI_TXP2:
			PINMUX_CONFIG(PAD_MIPI_TXP2, VO_CLK0);
		break;
		case VO_VIVO_CLK:
			PINMUX_CONFIG(VIVO_CLK, VO_CLK1);
		break;
		case VO_MIPI_TXM2:
			PINMUX_CONFIG(PAD_MIPI_TXM2, VO_D_0);
		break;
		case VO_MIPI_TXP1:
			PINMUX_CONFIG(PAD_MIPI_TXP1, VO_D_1);
		break;
		case VO_MIPI_TXM1:
			PINMUX_CONFIG(PAD_MIPI_TXM1, VO_D_2);
		break;
		case VO_MIPI_TXP0:
			PINMUX_CONFIG(PAD_MIPI_TXP0, VO_D_3);
		break;
		case VO_MIPI_TXM0:
			PINMUX_CONFIG(PAD_MIPI_TXM0, VO_D_4);
		break;
		case VO_MIPI_RXP0:
			PINMUX_CONFIG(PAD_MIPIRX0P, VO_D_5);
		break;
		case VO_MIPI_RXN0:
			PINMUX_CONFIG(PAD_MIPIRX0N, VO_D_6);
		break;
		case VO_MIPI_RXP1:
			PINMUX_CONFIG(PAD_MIPIRX1P, VO_D_7);
		break;
		case VO_MIPI_RXN1:
			PINMUX_CONFIG(PAD_MIPIRX1N, VO_D_8);
		break;
		case VO_MIPI_RXP2:
			PINMUX_CONFIG(PAD_MIPIRX2P, VO_D_9);
		break;
		case VO_MIPI_RXN2:
			PINMUX_CONFIG(PAD_MIPIRX2N, VO_D_10);
		break;
		case VO_MIPI_RXP5:
			PINMUX_CONFIG(PAD_MIPIRX5P, VO_D_11);
		break;
		case VO_MIPI_RXN5:
			PINMUX_CONFIG(PAD_MIPIRX5N, VO_D_12);
		break;
		case VO_VIVO_D0:
			PINMUX_CONFIG(VIVO_D0, VO_D_13);
		break;
		case VO_VIVO_D1:
			PINMUX_CONFIG(VIVO_D1, VO_D_14);
		break;
		case VO_VIVO_D2:
			PINMUX_CONFIG(VIVO_D2, VO_D_15);
		break;
		case VO_VIVO_D3:
			PINMUX_CONFIG(VIVO_D3, VO_D_16);
		break;
		case VO_VIVO_D4:
			PINMUX_CONFIG(VIVO_D4, VO_D_17);
		break;
		case VO_VIVO_D5:
			PINMUX_CONFIG(VIVO_D5, VO_D_18);
		break;
		case VO_VIVO_D6:
			PINMUX_CONFIG(VIVO_D6, VO_D_19);
		break;
		case VO_VIVO_D7:
			PINMUX_CONFIG(VIVO_D7, VO_D_20);
		break;
		case VO_VIVO_D8:
			PINMUX_CONFIG(VIVO_D8, VO_D_21);
		break;
		case VO_VIVO_D9:
			PINMUX_CONFIG(VIVO_D9, VO_D_22);
		break;
		case VO_VIVO_D10:
			PINMUX_CONFIG(VIVO_D10, VO_D_23);
		break;
		case VO_MIPI_TXM4:
			PINMUX_CONFIG(PAD_MIPI_TXM4, VO_D_24);
		break;
		case VO_MIPI_TXP4:
			PINMUX_CONFIG(PAD_MIPI_TXP4, VO_D_25);
		break;
		case VO_MIPI_TXM3:
			PINMUX_CONFIG(PAD_MIPI_TXM3, VO_D_26);
		break;
		case VO_MIPI_TXP3:
			PINMUX_CONFIG(PAD_MIPI_TXP3, VO_D_27);
		break;
		default:
		break;
		}
	}

	for (i = 0; i < pin_num; ++i) {
		sclr_top_vo_mux_sel(pins[i].sel, pins[i].mux);
	}
}

static void _fill_disp_timing(struct sclr_disp_timing *timing, const struct sync_info_s *sync_info)
{
	timing->vtotal = sync_info->vid_vsa_lines + sync_info->vid_vbp_lines
			+ sync_info->vid_active_lines + sync_info->vid_vfp_lines - 1;
	timing->htotal = sync_info->vid_hsa_pixels + sync_info->vid_hbp_pixels
			+ sync_info->vid_hline_pixels + sync_info->vid_hfp_pixels - 1;
	timing->vsync_start = 1;
	timing->vsync_end = timing->vsync_start + sync_info->vid_vsa_lines - 1;
	timing->vfde_start = timing->vsync_start + sync_info->vid_vsa_lines + sync_info->vid_vbp_lines;
	timing->vfde_end = timing->vfde_start + sync_info->vid_active_lines - 1;
	timing->hsync_start = 1;
	timing->hsync_end = timing->hsync_start + sync_info->vid_hsa_pixels - 1;
	timing->hfde_start = timing->hsync_start + sync_info->vid_hsa_pixels + sync_info->vid_hbp_pixels;
	timing->hfde_end = timing->hfde_start + sync_info->vid_hline_pixels - 1;
	timing->vsync_pol = sync_info->vid_vsa_pos_polarity;
	timing->hsync_pol = sync_info->vid_hsa_pos_polarity;

	timing->vmde_start = timing->vfde_start;
	timing->vmde_end = timing->vfde_end;
	timing->hmde_start = timing->hfde_start;
	timing->hmde_end = timing->hfde_end;
}

int hw_mcu_cmd_send(const struct VO_I80_HW_INSTR_S *instr, int size)
{
	int i = 0;
	unsigned int sw_cmd0, sw_cmd1, sw_cmd2, sw_cmd3;
	int cmd_cnt = 0;

	for (i = 0; i < size; i = i + 4) {
		cmd_cnt = 0;
		if (i < size) {
			cmd_cnt++;
			sw_cmd0 = (instr[i].data_type << 8) | instr[i].data;
			i80_set_cmd0(sw_cmd0);
		}
		if ((i + 1) < size) {
			cmd_cnt++;
			sw_cmd1 = (instr[i + 1].data_type << 8) | instr[i + 1].data;
			i80_set_cmd1(sw_cmd1);
		}
		if ((i + 2) < size) {
			cmd_cnt++;
			sw_cmd2 = (instr[i + 2].data_type << 8) | instr[i + 2].data;
			i80_set_cmd2(sw_cmd2);
		}
		if ((i + 3) < size) {
			cmd_cnt++;
			sw_cmd3 = (instr[i + 3].data_type << 8) | instr[i + 3].data;
			i80_set_cmd3(sw_cmd3);
		}
		// printf("set cmd cnt [%d]\n",cmd_cnt);
		i80_set_cmd_cnt(cmd_cnt);
		i80_trig();
	}
	return 0;
}

int i80_hw_init(const struct VO_I80_HW_CFG_S *i80_hw_cfg)
{
	struct sclr_disp_timing timing;
	struct disp_ctrl_gpios ctrl_gpios;
	int ret;

	get_disp_ctrl_gpios(&ctrl_gpios);

	ret = dm_gpio_set_value(&ctrl_gpios.disp_power_ct_gpio,
				ctrl_gpios.disp_power_ct_gpio.flags & GPIOD_ACTIVE_LOW ? 0 : 1);
	if (ret < 0) {
		printf("dm_gpio_set_value(disp_power_ct_gpio, deassert) failed: %d", ret);
		return ret;
	}
	ret = dm_gpio_set_value(&ctrl_gpios.disp_pwm_gpio,
				ctrl_gpios.disp_pwm_gpio.flags & GPIOD_ACTIVE_LOW ? 0 : 1);
	if (ret < 0) {
		printf("dm_gpio_set_value(disp_pwm_gpio, deassert) failed: %d", ret);
		return ret;
	}
	ret = dm_gpio_set_value(&ctrl_gpios.disp_reset_gpio,
				ctrl_gpios.disp_reset_gpio.flags & GPIOD_ACTIVE_LOW ? 0 : 1);
	if (ret < 0) {
		printf("dm_gpio_set_value(disp_reset_gpio, deassert) failed: %d", ret);
		return ret;
	}
	mdelay(10);
	ret = dm_gpio_set_value(&ctrl_gpios.disp_reset_gpio,
				ctrl_gpios.disp_reset_gpio.flags & GPIOD_ACTIVE_LOW ? 1 : 0);
	if (ret < 0) {
		printf("dm_gpio_set_value(disp_reset_gpio, deassert) failed: %d", ret);
		return ret;
	}
	mdelay(10);
	ret = dm_gpio_set_value(&ctrl_gpios.disp_reset_gpio,
				ctrl_gpios.disp_reset_gpio.flags & GPIOD_ACTIVE_LOW ? 0 : 1);
	if (ret < 0) {
		printf("dm_gpio_set_value(disp_reset_gpio, deassert) failed: %d", ret);
		return ret;
	}
	mdelay(100);

	sclr_disp_mux_sel(SCLR_VO_SEL_I80_HW);

	_disp_sel_remux(i80_hw_cfg->pins.d_pins, i80_hw_cfg->pins.pin_num);

	sclr_disp_set_intf(SCLR_VO_INTF_I80_HW);

	u32 pixelclock = 60 * (i80_hw_cfg->sync_info.vid_vsa_lines + i80_hw_cfg->sync_info.vid_vbp_lines
					   + i80_hw_cfg->sync_info.vid_active_lines + i80_hw_cfg->sync_info.vid_vfp_lines)
					* (i80_hw_cfg->sync_info.vid_hsa_pixels + i80_hw_cfg->sync_info.vid_hbp_pixels
					   + i80_hw_cfg->sync_info.vid_hline_pixels + i80_hw_cfg->sync_info.vid_hfp_pixels) / 1000;

	if (i80_hw_cfg->mode == VO_I80_HW_MODE_RGB565) {
		dphy_dsi_set_pll(pixelclock * 4, 4, 24);
		vip_sys_clk_setting(0x10080);
	} else if (i80_hw_cfg->mode == VO_I80_HW_MODE_RGB888) {
		dphy_dsi_set_pll(pixelclock * 6, 4, 24);
		vip_sys_clk_setting(0x100c0);
	}
	//pinmux
	hw_mcu_cmd_send(i80_hw_cfg->instrs.instr_cmd, i80_hw_cfg->instrs.instr_num);
	sclr_disp_set_mcu_en(i80_hw_cfg->mode);

	_fill_disp_timing(&timing, &i80_hw_cfg->sync_info);
	sclr_disp_set_timing(&timing);

	sclr_disp_tgen_enable(true);

	return 0;
}
