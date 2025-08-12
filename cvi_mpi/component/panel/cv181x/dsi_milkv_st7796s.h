#ifndef _MIPI_TX_PARAM_MILKV_ST7796S_H_
#define _MIPI_TX_PARAM_MILKV_ST7796S_H_

#include <linux/cvi_comm_mipi_tx.h>

#define MILKV_ST7796S_HACT	320
#define MILKV_ST7796S_HSA		20
#define MILKV_ST7796S_HBP		40
#define MILKV_ST7796S_HFP		10

#define MILKV_ST7796S_VACT	480
#define MILKV_ST7796S_VSA		2
#define MILKV_ST7796S_VBP		2
#define MILKV_ST7796S_VFP		2

#define PIXEL_CLK(x) ((x##_VACT + x##_VSA + x##_VBP + x##_VFP) \
	* (x##_HACT + x##_HSA + x##_HBP + x##_HFP) * 60 / 1000)

struct combo_dev_cfg_s dev_cfg_milkv_st7796s_320x480 = {
	.devno = 0,
	.lane_id = {MIPI_TX_LANE_0, -1, MIPI_TX_LANE_CLK, -1, -1},
	.lane_pn_swap = {false, false, false, false, false},
	.output_mode = OUTPUT_MODE_DSI_VIDEO,
	.video_mode = BURST_MODE,
	.output_format = OUT_FORMAT_RGB_24_BIT,
	.sync_info = {
		.vid_hsa_pixels = MILKV_ST7796S_HSA,
		.vid_hbp_pixels = MILKV_ST7796S_HBP,
		.vid_hfp_pixels = MILKV_ST7796S_HFP,
		.vid_hline_pixels = MILKV_ST7796S_HACT,
		.vid_vsa_lines = MILKV_ST7796S_VSA,
		.vid_vbp_lines = MILKV_ST7796S_VBP,
		.vid_vfp_lines = MILKV_ST7796S_VFP,
		.vid_active_lines = MILKV_ST7796S_VACT,
		.vid_vsa_pos_polarity = true,
		.vid_hsa_pos_polarity = true,
	},
	.pixel_clk = PIXEL_CLK(MILKV_ST7796S),
};

const struct hs_settle_s hs_timing_cfg_milkv_st7796s_320x480 = { .prepare = 6, .zero = 32, .trail = 1 };

static CVI_U8 data_milkv_st7796s_0[]  = { 0x11 };   // Sleep Out
static CVI_U8 data_milkv_st7796s_1[]  = { 0x36, 0x48 };    // Memory Data Access Control
static CVI_U8 data_milkv_st7796s_2[]  = { 0x35, 0x00 };
static CVI_U8 data_milkv_st7796s_3[]  = { 0x3A, 0x77 };    // Interface Pixel Format
static CVI_U8 data_milkv_st7796s_4[]  = { 0xF0, 0xC3 };    // Command Set Control
static CVI_U8 data_milkv_st7796s_5[]  = { 0xF0, 0x96 };
static CVI_U8 data_milkv_st7796s_6[]  = { 0xB4, 0x01 };    // , 1-Dot INV
static CVI_U8 data_milkv_st7796s_7[]  = { 0xB7, 0xC6 };
static CVI_U8 data_milkv_st7796s_8[]  = { 0xB9, 0x02, 0xE0 };
static CVI_U8 data_milkv_st7796s_9[]  = { 0xC0, 0xF0, 0x54 };  // , VGH = ?V, VGL = -?V
static CVI_U8 data_milkv_st7796s_10[] = { 0xC1, 0x15 };    // ?V
static CVI_U8 data_milkv_st7796s_11[] = { 0xC2, 0xAF };
static CVI_U8 data_milkv_st7796s_12[] = { 0xC5, 0x06 };   // VCOM Control, ?V
static CVI_U8 data_milkv_st7796s_13[] = { 0xC6, 0x00 };
static CVI_U8 data_milkv_st7796s_14[] = { 0xB6, 0x20, 0x02, 0x3B };
static CVI_U8 data_milkv_st7796s_15[] = { 0xE7, 0x27, 0x02, 0x42, 0xB5, 0x05 };
static CVI_U8 data_milkv_st7796s_16[] = { 0xE8, 0x40, 0x8A, 0x00, 0x00, 0x29, 0x19, 0xA5, 0x33 };
static CVI_U8 data_milkv_st7796s_17[] = { 0xE0, 0xD0, 0x0A, 0x00, 0x1B,
                                          0x15, 0x27, 0x33, 0x44, 0x48,
                                          0x17, 0x14, 0x15, 0x2C, 0x31 };
static CVI_U8 data_milkv_st7796s_18[] = { 0xE1, 0xD0, 0x14, 0x00, 0x1F,
                                          0x13, 0x0B, 0x32, 0x43, 0x47,
                                          0x38, 0x12, 0x12, 0x2A, 0x32 };

static CVI_U8 data_milkv_st7796s_19[] = { 0xF0, 0x3C };
static CVI_U8 data_milkv_st7796s_20[] = { 0xF0, 0x69 };
static CVI_U8 data_milkv_st7796s_21[] = { 0x21 }; // Display Inversion On

static CVI_U8 data_milkv_st7796s_22[] = { 0x11 };
static CVI_U8 data_milkv_st7796s_23[] = { 0x29 }; // Display ON

const struct dsc_instr dsi_init_cmds_milkv_st7796s_320x480[] = {
    {.delay = 120, .data_type = 0x05, .size = 1,  .data = data_milkv_st7796s_0 },

    {.delay = 0,   .data_type = 0x15, .size = 2,  .data = data_milkv_st7796s_1 },
    {.delay = 0,   .data_type = 0x15, .size = 2,  .data = data_milkv_st7796s_2 },
    {.delay = 0,   .data_type = 0x15, .size = 2,  .data = data_milkv_st7796s_3 },
    {.delay = 0,   .data_type = 0x15, .size = 2,  .data = data_milkv_st7796s_4 },
    {.delay = 0,   .data_type = 0x15, .size = 2,  .data = data_milkv_st7796s_5 },
    {.delay = 0,   .data_type = 0x15, .size = 2,  .data = data_milkv_st7796s_6 },
    {.delay = 0,   .data_type = 0x15, .size = 2,  .data = data_milkv_st7796s_7 },

    {.delay = 0,   .data_type = 0x29, .size = 3,  .data = data_milkv_st7796s_8 },
    {.delay = 0,   .data_type = 0x29, .size = 3,  .data = data_milkv_st7796s_9 },

    {.delay = 0,   .data_type = 0x15, .size = 2,  .data = data_milkv_st7796s_10 },
    {.delay = 0,   .data_type = 0x15, .size = 2,  .data = data_milkv_st7796s_11 },
    {.delay = 0,   .data_type = 0x15, .size = 2,  .data = data_milkv_st7796s_12 },
    {.delay = 0,   .data_type = 0x15, .size = 2,  .data = data_milkv_st7796s_13 },

    {.delay = 0,   .data_type = 0x29, .size = 4,  .data = data_milkv_st7796s_14 },
    {.delay = 0,   .data_type = 0x29, .size = 6,  .data = data_milkv_st7796s_15 },
    {.delay = 0,   .data_type = 0x29, .size = 9,  .data = data_milkv_st7796s_16 },

    {.delay = 0,   .data_type = 0x29, .size = 15, .data = data_milkv_st7796s_17 },
    {.delay = 0,   .data_type = 0x29, .size = 15, .data = data_milkv_st7796s_18 },

    {.delay = 0,   .data_type = 0x15, .size = 2,  .data = data_milkv_st7796s_19 },
    {.delay = 0,   .data_type = 0x15, .size = 2,  .data = data_milkv_st7796s_20 },

    {.delay = 120, .data_type = 0x05, .size = 1,  .data = data_milkv_st7796s_21 },
    {.delay = 120, .data_type = 0x05, .size = 1,  .data = data_milkv_st7796s_22 },
    {.delay = 120, .data_type = 0x05, .size = 1,  .data = data_milkv_st7796s_23 }
};

#else
#error "MIPI_TX_PARAM multi-delcaration!!"
#endif // _MIPI_TX_PARAM_MILKV_ST7796S_H_
