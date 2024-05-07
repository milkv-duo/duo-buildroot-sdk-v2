#ifndef _CVI_I80_HW_H_
#define _CVI_I80_HW_H_

#include "cvi_mipi.h"

#define MAX_VO_PINS 32
#define MAX_MCU_INSTR 256

#define COMMAND 0
#define DATA	1

struct VO_I80_HW_INSTR_S {
	unsigned char delay;
	unsigned char data_type;
	unsigned char data;
};

enum VO_I80_HW_MODE {
	VO_I80_HW_MODE_RGB565 = 0,
	VO_I80_HW_MODE_RGB888,
	VO_I80_HW_MODE_MAX,
};

enum VO_TOP_MUX {
	VO_MUX_MCU_CS = 0,
	VO_MUX_MCU_RS,
	VO_MUX_MCU_WR,
	VO_MUX_MCU_RD,
	VO_MUX_MCU_DATA0,
	VO_MUX_MCU_DATA1,
	VO_MUX_MCU_DATA2,
	VO_MUX_MCU_DATA3,
	VO_MUX_MCU_DATA4,
	VO_MUX_MCU_DATA5,
	VO_MUX_MCU_DATA6,
	VO_MUX_MCU_DATA7,
	VO_MUX_MAX,
};

enum VO_TOP_SEL {
	VO_CLK_0 = 0,
	VO_CLK_1,
	VO_D0,
	VO_D1,
	VO_D2,
	VO_D3,
	VO_D4,
	VO_D5,
	VO_D6,
	VO_D7,
	VO_D8,
	VO_D9,
	VO_D10,
	VO_D11,
	VO_D12,
	VO_D13,
	VO_D14,
	VO_D15,
	VO_D16,
	VO_D17,
	VO_D18,
	VO_D19,
	VO_D20,
	VO_D21,
	VO_D22,
	VO_D23,
	VO_D24,
	VO_D25,
	VO_D26,
	VO_D27,
	VO_D_MAX,
};

enum VO_TOP_D_SEL {
	VO_VIVO_D0 = VO_D13,
	VO_VIVO_D1 = VO_D14,
	VO_VIVO_D2 = VO_D15,
	VO_VIVO_D3 = VO_D16,
	VO_VIVO_D4 = VO_D17,
	VO_VIVO_D5 = VO_D18,
	VO_VIVO_D6 = VO_D19,
	VO_VIVO_D7 = VO_D20,
	VO_VIVO_D8 = VO_D21,
	VO_VIVO_D9 = VO_D22,
	VO_VIVO_D10 = VO_D23,
	VO_VIVO_CLK = VO_CLK_1,
	VO_MIPI_TXM4 = VO_D24,
	VO_MIPI_TXP4 = VO_D25,
	VO_MIPI_TXM3 = VO_D26,
	VO_MIPI_TXP3 = VO_D27,
	VO_MIPI_TXM2 = VO_D0,
	VO_MIPI_TXP2 = VO_CLK_0,
	VO_MIPI_TXM1 = VO_D2,
	VO_MIPI_TXP1 = VO_D1,
	VO_MIPI_TXM0 = VO_D4,
	VO_MIPI_TXP0 = VO_D3,
	VO_MIPI_RXN5 = VO_D12,
	VO_MIPI_RXP5 = VO_D11,
	VO_MIPI_RXN2 = VO_D10,
	VO_MIPI_RXP2 = VO_D9,
	VO_MIPI_RXN1 = VO_D8,
	VO_MIPI_RXP1 = VO_D7,
	VO_MIPI_RXN0 = VO_D6,
	VO_MIPI_RXP0 = VO_D5,
	VO_PAD_MAX = VO_D_MAX
};

struct VO_D_REMAP {
	enum VO_TOP_D_SEL sel;
	enum VO_TOP_MUX mux;
};

struct VO_I80_HW_PINS {
	unsigned char pin_num;
	struct VO_D_REMAP d_pins[MAX_VO_PINS];
};

struct VO_I80_HW_INSTRS {
	unsigned char instr_num;
	struct VO_I80_HW_INSTR_S instr_cmd[MAX_MCU_INSTR];
};

struct VO_I80_HW_CFG_S {
	enum VO_I80_HW_MODE mode;
	struct VO_I80_HW_PINS pins;
	struct VO_I80_HW_INSTRS instrs;
	struct sync_info_s sync_info;
};

int i80_hw_init(const struct VO_I80_HW_CFG_S *i80_hw_cfg);

#endif // _CVI_I80_HW_H_