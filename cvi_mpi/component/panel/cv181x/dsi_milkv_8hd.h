#ifndef _MIPI_TX_PARAM_MILKV_8HD_H_
#define _MIPI_TX_PARAM_MILKV_8HD_H_

#include <linux/cvi_comm_mipi_tx.h>

struct combo_dev_cfg_s dev_cfg_milkv_8hd_800x1280 = {
	.devno = 0,
	.lane_id = {MIPI_TX_LANE_0, MIPI_TX_LANE_1, MIPI_TX_LANE_CLK, MIPI_TX_LANE_2, MIPI_TX_LANE_3},
	.lane_pn_swap = {false, false, false, false, false},
	.output_mode = OUTPUT_MODE_DSI_VIDEO,
	.video_mode = BURST_MODE,
	.output_format = OUT_FORMAT_RGB_24_BIT,
	.sync_info = {
		.vid_hsa_pixels = 18,
		.vid_hbp_pixels = 20,
		.vid_hfp_pixels = 40,
		.vid_hline_pixels = 800,
		.vid_vsa_lines = 4,
		.vid_vbp_lines = 20,
		.vid_vfp_lines = 20,
		.vid_active_lines = 1280,
		.vid_vsa_pos_polarity = false,
		.vid_hsa_pos_polarity = false,
	},
	.pixel_clk = 70000,
};

const struct hs_settle_s hs_timing_cfg_milkv_8hd_800x1280 = { .prepare = 6, .zero = 32, .trail = 1 };

static CVI_U8 data_milkv_8hd_0[] = { 0xE0, 0x00 };
static CVI_U8 data_milkv_8hd_1[] = { 0xE1, 0x93 };
static CVI_U8 data_milkv_8hd_2[] = { 0xE2, 0x65 };
static CVI_U8 data_milkv_8hd_3[] = { 0xE3, 0xF8 };
static CVI_U8 data_milkv_8hd_4[] = { 0x80, 0x03 };
static CVI_U8 data_milkv_8hd_5[] = { 0xE0, 0x01 };
static CVI_U8 data_milkv_8hd_6[] = { 0x00, 0x00 };
static CVI_U8 data_milkv_8hd_7[] = { 0x01, 0x7E };
static CVI_U8 data_milkv_8hd_8[] = { 0x03, 0x00 };
static CVI_U8 data_milkv_8hd_9[] = { 0x04, 0x65 };
static CVI_U8 data_milkv_8hd_10[] = { 0x0C, 0x74 };
static CVI_U8 data_milkv_8hd_11[] = { 0x17, 0x00 };
static CVI_U8 data_milkv_8hd_12[] = { 0x18, 0xB7 };
static CVI_U8 data_milkv_8hd_13[] = { 0x19, 0x00 };
static CVI_U8 data_milkv_8hd_14[] = { 0x1A, 0x00 };
static CVI_U8 data_milkv_8hd_15[] = { 0x1B, 0xB7 };
static CVI_U8 data_milkv_8hd_16[] = { 0x1C, 0x00 };
static CVI_U8 data_milkv_8hd_17[] = { 0x24, 0xFE };
static CVI_U8 data_milkv_8hd_18[] = { 0x37, 0x19 };
static CVI_U8 data_milkv_8hd_19[] = { 0x38, 0x05 };
static CVI_U8 data_milkv_8hd_20[] = { 0x39, 0x00 };
static CVI_U8 data_milkv_8hd_21[] = { 0x3A, 0x01 };
static CVI_U8 data_milkv_8hd_22[] = { 0x3B, 0x01 };
static CVI_U8 data_milkv_8hd_23[] = { 0x3C, 0x70 };
static CVI_U8 data_milkv_8hd_24[] = { 0x3D, 0xFF };
static CVI_U8 data_milkv_8hd_25[] = { 0x3E, 0xFF };
static CVI_U8 data_milkv_8hd_26[] = { 0x3F, 0xFF };
static CVI_U8 data_milkv_8hd_27[] = { 0x40, 0x06 };
static CVI_U8 data_milkv_8hd_28[] = { 0x41, 0xA0 };
static CVI_U8 data_milkv_8hd_29[] = { 0x43, 0x1E };
static CVI_U8 data_milkv_8hd_30[] = { 0x44, 0x0F };
static CVI_U8 data_milkv_8hd_31[] = { 0x45, 0x28 };
static CVI_U8 data_milkv_8hd_32[] = { 0x4B, 0x04 };
static CVI_U8 data_milkv_8hd_33[] = { 0x55, 0x02 };
static CVI_U8 data_milkv_8hd_34[] = { 0x56, 0x01 };
static CVI_U8 data_milkv_8hd_35[] = { 0x57, 0xA9 };
static CVI_U8 data_milkv_8hd_36[] = { 0x58, 0x0A };
static CVI_U8 data_milkv_8hd_37[] = { 0x59, 0x0A };
static CVI_U8 data_milkv_8hd_38[] = { 0x5A, 0x37 };
static CVI_U8 data_milkv_8hd_39[] = { 0x5B, 0x19 };
static CVI_U8 data_milkv_8hd_40[] = { 0x5D, 0x78 };
static CVI_U8 data_milkv_8hd_41[] = { 0x5E, 0x63 };
static CVI_U8 data_milkv_8hd_42[] = { 0x5F, 0x54 };
static CVI_U8 data_milkv_8hd_43[] = { 0x60, 0x49 };
static CVI_U8 data_milkv_8hd_44[] = { 0x61, 0x45 };
static CVI_U8 data_milkv_8hd_45[] = { 0x62, 0x38 };
static CVI_U8 data_milkv_8hd_46[] = { 0x63, 0x3D };
static CVI_U8 data_milkv_8hd_47[] = { 0x64, 0x28 };
static CVI_U8 data_milkv_8hd_48[] = { 0x65, 0x43 };
static CVI_U8 data_milkv_8hd_49[] = { 0x66, 0x41 };
static CVI_U8 data_milkv_8hd_50[] = { 0x67, 0x43 };
static CVI_U8 data_milkv_8hd_51[] = { 0x68, 0x62 };
static CVI_U8 data_milkv_8hd_52[] = { 0x69, 0x50 };
static CVI_U8 data_milkv_8hd_53[] = { 0x6A, 0x57 };
static CVI_U8 data_milkv_8hd_54[] = { 0x6B, 0x49 };
static CVI_U8 data_milkv_8hd_55[] = { 0x6C, 0x44 };
static CVI_U8 data_milkv_8hd_56[] = { 0x6D, 0x37 };
static CVI_U8 data_milkv_8hd_57[] = { 0x6E, 0x23 };
static CVI_U8 data_milkv_8hd_58[] = { 0x6F, 0x10 };
static CVI_U8 data_milkv_8hd_59[] = { 0x70, 0x78 };
static CVI_U8 data_milkv_8hd_60[] = { 0x71, 0x63 };
static CVI_U8 data_milkv_8hd_61[] = { 0x72, 0x54 };
static CVI_U8 data_milkv_8hd_62[] = { 0x73, 0x49 };
static CVI_U8 data_milkv_8hd_63[] = { 0x74, 0x45 };
static CVI_U8 data_milkv_8hd_64[] = { 0x75, 0x38 };
static CVI_U8 data_milkv_8hd_65[] = { 0x76, 0x3D };
static CVI_U8 data_milkv_8hd_66[] = { 0x77, 0x28 };
static CVI_U8 data_milkv_8hd_67[] = { 0x78, 0x43 };
static CVI_U8 data_milkv_8hd_68[] = { 0x79, 0x41 };
static CVI_U8 data_milkv_8hd_69[] = { 0x7A, 0x43 };
static CVI_U8 data_milkv_8hd_70[] = { 0x7B, 0x62 };
static CVI_U8 data_milkv_8hd_71[] = { 0x7C, 0x50 };
static CVI_U8 data_milkv_8hd_72[] = { 0x7D, 0x57 };
static CVI_U8 data_milkv_8hd_73[] = { 0x7E, 0x49 };
static CVI_U8 data_milkv_8hd_74[] = { 0x7F, 0x44 };
static CVI_U8 data_milkv_8hd_75[] = { 0x80, 0x37 };
static CVI_U8 data_milkv_8hd_76[] = { 0x81, 0x23 };
static CVI_U8 data_milkv_8hd_77[] = { 0x82, 0x10 };
static CVI_U8 data_milkv_8hd_78[] = { 0xE0, 0x02 };
static CVI_U8 data_milkv_8hd_79[] = { 0x00, 0x47 };
static CVI_U8 data_milkv_8hd_80[] = { 0x01, 0x47 };
static CVI_U8 data_milkv_8hd_81[] = { 0x02, 0x45 };
static CVI_U8 data_milkv_8hd_82[] = { 0x03, 0x45 };
static CVI_U8 data_milkv_8hd_83[] = { 0x04, 0x4B };
static CVI_U8 data_milkv_8hd_84[] = { 0x05, 0x4B };
static CVI_U8 data_milkv_8hd_85[] = { 0x06, 0x49 };
static CVI_U8 data_milkv_8hd_86[] = { 0x07, 0x49 };
static CVI_U8 data_milkv_8hd_87[] = { 0x08, 0x41 };
static CVI_U8 data_milkv_8hd_88[] = { 0x09, 0x1F };
static CVI_U8 data_milkv_8hd_89[] = { 0x0A, 0x1F };
static CVI_U8 data_milkv_8hd_90[] = { 0x0B, 0x1F };
static CVI_U8 data_milkv_8hd_91[] = { 0x0C, 0x1F };
static CVI_U8 data_milkv_8hd_92[] = { 0x0D, 0x1F };
static CVI_U8 data_milkv_8hd_93[] = { 0x0E, 0x1F };
static CVI_U8 data_milkv_8hd_94[] = { 0x0F, 0x5F };
static CVI_U8 data_milkv_8hd_95[] = { 0x10, 0x5F };
static CVI_U8 data_milkv_8hd_96[] = { 0x11, 0x57 };
static CVI_U8 data_milkv_8hd_97[] = { 0x12, 0x77 };
static CVI_U8 data_milkv_8hd_98[] = { 0x13, 0x35 };
static CVI_U8 data_milkv_8hd_99[] = { 0x14, 0x1F };
static CVI_U8 data_milkv_8hd_100[] = { 0x15, 0x1F };
static CVI_U8 data_milkv_8hd_101[] = { 0x16, 0x46 };
static CVI_U8 data_milkv_8hd_102[] = { 0x17, 0x46 };
static CVI_U8 data_milkv_8hd_103[] = { 0x18, 0x44 };
static CVI_U8 data_milkv_8hd_104[] = { 0x19, 0x44 };
static CVI_U8 data_milkv_8hd_105[] = { 0x1A, 0x4A };
static CVI_U8 data_milkv_8hd_106[] = { 0x1B, 0x4A };
static CVI_U8 data_milkv_8hd_107[] = { 0x1C, 0x48 };
static CVI_U8 data_milkv_8hd_108[] = { 0x1D, 0x48 };
static CVI_U8 data_milkv_8hd_109[] = { 0x1E, 0x40 };
static CVI_U8 data_milkv_8hd_110[] = { 0x1F, 0x1F };
static CVI_U8 data_milkv_8hd_111[] = { 0x20, 0x1F };
static CVI_U8 data_milkv_8hd_112[] = { 0x21, 0x1F };
static CVI_U8 data_milkv_8hd_113[] = { 0x22, 0x1F };
static CVI_U8 data_milkv_8hd_114[] = { 0x23, 0x1F };
static CVI_U8 data_milkv_8hd_115[] = { 0x24, 0x1F };
static CVI_U8 data_milkv_8hd_116[] = { 0x25, 0x5F };
static CVI_U8 data_milkv_8hd_117[] = { 0x26, 0x5F };
static CVI_U8 data_milkv_8hd_118[] = { 0x27, 0x57 };
static CVI_U8 data_milkv_8hd_119[] = { 0x28, 0x77 };
static CVI_U8 data_milkv_8hd_120[] = { 0x29, 0x35 };
static CVI_U8 data_milkv_8hd_121[] = { 0x2A, 0x1F };
static CVI_U8 data_milkv_8hd_122[] = { 0x2B, 0x1F };
static CVI_U8 data_milkv_8hd_123[] = { 0x58, 0x40 };
static CVI_U8 data_milkv_8hd_124[] = { 0x59, 0x00 };
static CVI_U8 data_milkv_8hd_125[] = { 0x5A, 0x00 };
static CVI_U8 data_milkv_8hd_126[] = { 0x5B, 0x10 };
static CVI_U8 data_milkv_8hd_127[] = { 0x5C, 0x06 };
static CVI_U8 data_milkv_8hd_128[] = { 0x5D, 0x40 };
static CVI_U8 data_milkv_8hd_129[] = { 0x5E, 0x01 };
static CVI_U8 data_milkv_8hd_130[] = { 0x5F, 0x02 };
static CVI_U8 data_milkv_8hd_131[] = { 0x60, 0x30 };
static CVI_U8 data_milkv_8hd_132[] = { 0x61, 0x01 };
static CVI_U8 data_milkv_8hd_133[] = { 0x62, 0x02 };
static CVI_U8 data_milkv_8hd_134[] = { 0x63, 0x03 };
static CVI_U8 data_milkv_8hd_135[] = { 0x64, 0x6B };
static CVI_U8 data_milkv_8hd_136[] = { 0x65, 0x05 };
static CVI_U8 data_milkv_8hd_137[] = { 0x66, 0x0C };
static CVI_U8 data_milkv_8hd_138[] = { 0x67, 0x73 };
static CVI_U8 data_milkv_8hd_139[] = { 0x68, 0x09 };
static CVI_U8 data_milkv_8hd_140[] = { 0x69, 0x03 };
static CVI_U8 data_milkv_8hd_141[] = { 0x6A, 0x56 };
static CVI_U8 data_milkv_8hd_142[] = { 0x6B, 0x08 };
static CVI_U8 data_milkv_8hd_143[] = { 0x6C, 0x00 };
static CVI_U8 data_milkv_8hd_144[] = { 0x6D, 0x04 };
static CVI_U8 data_milkv_8hd_145[] = { 0x6E, 0x04 };
static CVI_U8 data_milkv_8hd_146[] = { 0x6F, 0x88 };
static CVI_U8 data_milkv_8hd_147[] = { 0x70, 0x00 };
static CVI_U8 data_milkv_8hd_148[] = { 0x71, 0x00 };
static CVI_U8 data_milkv_8hd_149[] = { 0x72, 0x06 };
static CVI_U8 data_milkv_8hd_150[] = { 0x73, 0x7B };
static CVI_U8 data_milkv_8hd_151[] = { 0x74, 0x00 };
static CVI_U8 data_milkv_8hd_152[] = { 0x75, 0xF8 };
static CVI_U8 data_milkv_8hd_153[] = { 0x76, 0x00 };
static CVI_U8 data_milkv_8hd_154[] = { 0x77, 0xD5 };
static CVI_U8 data_milkv_8hd_155[] = { 0x78, 0x2E };
static CVI_U8 data_milkv_8hd_156[] = { 0x79, 0x12 };
static CVI_U8 data_milkv_8hd_157[] = { 0x7A, 0x03 };
static CVI_U8 data_milkv_8hd_158[] = { 0x7B, 0x00 };
static CVI_U8 data_milkv_8hd_159[] = { 0x7C, 0x00 };
static CVI_U8 data_milkv_8hd_160[] = { 0x7D, 0x03 };
static CVI_U8 data_milkv_8hd_161[] = { 0x7E, 0x7B };
static CVI_U8 data_milkv_8hd_162[] = { 0xE0, 0x04 };
static CVI_U8 data_milkv_8hd_163[] = { 0x00, 0x0E };
static CVI_U8 data_milkv_8hd_164[] = { 0x02, 0xB3 };
static CVI_U8 data_milkv_8hd_165[] = { 0x09, 0x60 };
static CVI_U8 data_milkv_8hd_166[] = { 0x0E, 0x2A };
static CVI_U8 data_milkv_8hd_167[] = { 0x36, 0x59 };
static CVI_U8 data_milkv_8hd_168[] = { 0xE0, 0x00 };

static CVI_U8 data_milkv_8hd_169[] = { 0x11 };
static CVI_U8 data_milkv_8hd_170[] = { 0x29 };

const struct dsc_instr dsi_init_cmds_milkv_8hd_800x1280[] = {
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_0 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_1 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_2 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_3 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_4 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_5 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_6 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_7 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_8 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_9 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_10 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_11 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_12 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_13 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_14 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_15 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_16 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_17 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_18 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_19 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_20 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_21 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_22 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_23 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_24 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_25 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_26 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_27 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_28 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_29 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_30 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_31 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_32 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_33 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_34 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_35 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_36 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_37 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_38 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_39 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_40 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_41 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_42 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_43 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_44 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_45 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_46 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_47 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_48 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_49 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_50 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_51 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_52 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_53 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_54 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_55 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_56 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_57 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_58 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_59 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_60 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_61 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_62 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_63 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_64 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_65 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_66 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_67 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_68 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_69 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_70 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_71 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_72 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_73 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_74 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_75 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_76 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_77 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_78 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_79 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_80 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_81 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_82 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_83 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_84 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_85 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_86 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_87 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_88 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_89 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_90 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_91 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_92 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_93 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_94 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_95 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_96 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_97 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_98 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_99 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_100 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_101 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_102 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_103 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_104 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_105 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_106 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_107 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_108 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_109 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_110 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_111 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_112 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_113 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_114 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_115 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_116 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_117 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_118 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_119 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_120 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_121 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_122 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_123 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_124 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_125 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_126 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_127 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_128 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_129 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_130 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_131 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_132 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_133 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_134 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_135 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_136 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_137 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_138 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_139 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_140 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_141 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_142 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_143 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_144 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_145 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_146 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_147 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_148 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_149 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_150 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_151 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_152 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_153 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_154 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_155 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_156 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_157 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_158 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_159 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_160 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_161 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_162 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_163 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_164 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_165 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_166 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_167 },
	{.delay = 0, .data_type = 0x15, .size = 2, .data = data_milkv_8hd_168 },

	{.delay = 120, .data_type = 0x05, .size = 1, .data = data_milkv_8hd_169 },
	{.delay = 20, .data_type = 0x05, .size = 1, .data = data_milkv_8hd_170 }
};

#else
#error "MIPI_TX_PARAM_MILKV_8HD multi-delcaration!!"
#endif // _MIPI_TX_PARAM_MILKV_8HD_H_
