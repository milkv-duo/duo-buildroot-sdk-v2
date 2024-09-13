/**
 ******************************************************************************
 *
 *  @file bl_main.h
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


#ifndef _BL_MAIN_H_
#define _BL_MAIN_H_

#include "bl_defs.h"


int bl_cfg80211_init(struct bl_plat *bl_plat, void **platform_data);
void bl_cfg80211_deinit(struct bl_hw *bl_hw);
int bl_ioctl(struct net_device *dev, struct ifreq *ifreq, int cmd);
void bl_nl_broadcast_event(struct bl_hw *bl_hw, u32 event_id, u8* payload, u16 len);

#endif /* _BL_MAIN_H_ */
