/**
 ******************************************************************************
 *
 *  @file bl_mod_params.h
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


#ifndef _BL_MOD_PARAM_H_
#define _BL_MOD_PARAM_H_

struct bl_mod_params {
    bool ht_on;
    bool vht_on;
    int mcs_map;
    bool ldpc_on;
    bool vht_stbc;
    int phy_cfg;
    int uapsd_timeout;
    bool ap_uapsd_on;
    bool sgi;
    bool sgi80;
    bool use_2040;
    bool use_80;
    bool custregd;
    int nss;
    bool bfmee;
    bool bfmer;
    bool mesh;
    bool murx;
    bool mutx;
    bool mutx_on;
    unsigned int roc_dur_max;
    int listen_itv;
    bool listen_bcmc;
    int lp_clk_ppm;
    bool ps_on;
    int tx_lft;
    int amsdu_maxnb;
    int uapsd_queues;
    bool tdls;
    int opmode;
    bool pn_check;
    char * wifi_mac;
    char * wifi_mac_cfg;
    bool mp_mode;	
    char * cal_data_cfg;
    bool tcp_ack_filter;
    int  tcp_ack_max;
    
    char *dump_dir;
};

extern struct bl_mod_params bl_mod_params;

struct bl_hw;
int bl_handle_dynparams(struct bl_hw *bl_hw, struct wiphy *wiphy);
void bl_enable_wapi(struct bl_hw *bl_hw);
void bl_enable_mfp(struct bl_hw *bl_hw);

#endif /* _BL_MOD_PARAM_H_ */
