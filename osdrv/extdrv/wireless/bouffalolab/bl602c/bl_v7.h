/**
 ******************************************************************************
 *
 *  @file bl_v7.h
 *
 *  Copyright (C) BouffaloLab 2017-2024
 *
 *  Licensed under the Apache License, Version 2.0 (the License);
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an ASIS BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************
 */


#ifndef _BL_V7_H_
#define _BL_V7_H_

#include "bl_platform.h"

struct bl_device {
	const char *firmware;
	const struct bl_sdio_card_reg *reg;
	u8 *mp_regs;
	u8 max_ports;
	u8 mp_agg_pkt_limit;
	bool supports_sdio_new_mode;
	bool has_control_mask;
	bool supports_fw_dump;
	u16 tx_buf_size;
	u32 mp_tx_agg_buf_size;
	u32 mp_rx_agg_buf_size;
	u8 auto_tdls;
};

int bl_device_init(struct sdio_func *func, struct bl_plat **bl_plat);
void bl_device_deinit(struct bl_plat *bl_plat);

#endif /* _BL_V7_H_ */
