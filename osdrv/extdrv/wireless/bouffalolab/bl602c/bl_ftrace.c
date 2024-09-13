/**
 ******************************************************************************
 *
 *  @file bl_ftrace.c
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

#include "lmac_types.h"
#include "linux/ieee80211.h"
#include "bl_tx.h"

#ifndef CONFIG_BL_TRACE
void trace_mgmt_tx(u16 freq, u8 vif_idx, u8 sta_idx, struct ieee80211_mgmt *mgmt){}
void trace_mgmt_rx(u16 freq, u8 vif_idx, u8 sta_idx, struct ieee80211_mgmt *mgmt){}
void trace_hwq_flowctrl_stop(u8 hwq_idx){}
void trace_hwq_flowctrl_start(u8 hwq_idx){}
void trace_txq_add_to_hw(struct bl_txq *txq){}
void trace_txq_del_from_hw(struct bl_txq *txq){}
void trace_txq_flowctrl_stop(struct bl_txq *txq){}
void trace_txq_flowctrl_restart(struct bl_txq *txq){}
void trace_txq_start(struct bl_txq *txq, u16 reason){}
void trace_txq_stop(struct bl_txq *txq, u16 reason){}
void trace_txq_vif_start(u16 idx){}
void trace_txq_vif_stop(u16 idx){}
void trace_txq_sta_start(u16 idx){}
void trace_txq_sta_stop(u16 idx){}
void trace_ps_queue(struct bl_sta *sta){}
void trace_ps_push(struct bl_sta *sta){}
void trace_ps_enable(struct bl_sta *sta){}
void trace_ps_disable(u16 idx){}
void trace_roc(u8 vif_idx, u16 freq, unsigned int duration){}
void trace_cancel_roc(u8 vif_idx){}
void trace_roc_exp(u8 vif_idx){}
void trace_switch_roc(u8 vif_idx){}
void trace_msg_send(u16 id){}
void trace_msg_recv(u16 id){}
void trace_txq_queue_skb(struct sk_buff *skb, struct bl_txq *txq, bool retry){}
void trace_process_hw_queue(struct bl_hwq *hwq){}
void trace_process_txq(struct bl_txq *txq){}
void trace_push_desc(struct sk_buff *skb, struct bl_sw_txhdr *sw_txhdr, int push_flags){}
void trace_mgmt_cfm(u8 vif_idx, u8 sta_idx, bool acked){}
void trace_credit_update(struct bl_txq *txq, s8_l cred_up){}
void trace_ps_traffic_update(u16 sta_idx, u8 traffic, bool uapsd){}
void trace_ps_traffic_req(struct bl_sta *sta, u16 pkt_req, u8 ps_id){}
#endif
