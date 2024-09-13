/**
 ******************************************************************************
 *
 *  @file bl_cfgfile.h
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


#ifndef _BL_CFGFILE_H_
#define _BL_CFGFILE_H_

/*
 * Definitions for RF caldata processing
 */
#define RFTLV_SIZE_TYPE                (2)
#define RFTLV_SIZE_LENGTH              (2)
#define RFTLV_SIZE_TL                  (RFTLV_SIZE_TYPE + RFTLV_SIZE_LENGTH)

#define RFTLF_TYPE_MAGIC_FW_HEAD1      (0x4152415046524C42)
#define RFTLF_TYPE_MAGIC_FW_HEAD2      (0x6B3162586B44364F)
#define RFTLV_TYPE_INVALID             0X0000
#define RFTLV_TYPE_XTAL_MODE           0X0001
#define RFTLV_TYPE_XTAL                0X0002
#define RFTLV_TYPE_PWR_MODE            0X0003
#define RFTLV_TYPE_PWR_TABLE           0X0004
#define RFTLV_TYPE_PWR_TABLE_11B       0X0005
#define RFTLV_TYPE_PWR_TABLE_11G       0X0006
#define RFTLV_TYPE_PWR_TABLE_11N       0X0007
#define RFTLV_TYPE_PWR_OFFSET          0X0008
#define RFTLV_TYPE_CHAN_DIV_TAB        0X0009
#define RFTLV_TYPE_CHAN_CNT_TAB        0X000A
#define RFTLV_TYPE_LO_FCAL_DIV         0X000B
#define RFTLV_TYPE_EN_TCAL             0X0020
#define RFTLV_TYPE_LINEAR_OR_FOLLOW    0X0021
#define RFTLV_TYPE_TCHANNELS           0X0022
#define RFTLV_TYPE_TCHANNEL_OS         0X0023
#define RFTLV_TYPE_TCHANNEL_OS_LOW     0X0024
#define RFTLV_TYPE_TROOM_OS            0X0025
#define RFTLV_TYPE_PWR_TABLE_BLE       0X0030

#define RFTLV_MAXLEN_XTAL_MODE          (2)
#define RFTLV_MAXLEN_XTAL               (20)
#define RFTLV_MAXLEN_PWR_MODE           (2)
#define RFTLV_MAXLEN_PWR_TABLE_11B      (4)
#define RFTLV_MAXLEN_PWR_TABLE_11G      (8)
#define RFTLV_MAXLEN_PWR_TABLE_11N      (8)
#define RFTLV_MAXLEN_PWR_OFFSET         (14)
#define RFTLV_MAXLEN_EN_TCAL            (1)
#define RFTLV_MAXLEN_LINEAR_OR_FOLLOW   (1)
#define RFTLV_MAXLEN_TCHANNELS          (10)
#define RFTLV_MAXLEN_TCHANNEL_OS        (10)
#define RFTLV_MAXLEN_TCHANNEL_OS_LOW    (10)
#define RFTLV_MAXLEN_TROOM_OS           (2)
#define RFTLV_MAXLEN_PWR_TABLE_BLE      (4)

/*
 * Structure used to retrieve information from the Config file used at Initialization time
 */
struct bl_conf_file {
    u8 mac_addr[ETH_ALEN];
};

/*
 * Structure used to retrieve information from the PHY Config file used at Initialization time
 */
struct bl_phy_conf_file {
    struct phy_trd_cfg_tag trd;
    struct phy_karst_cfg_tag karst;
};

int bl_parse_configfile(struct bl_hw *bl_hw, const char *filename,
                          struct bl_conf_file *config);

int bl_parse_phy_configfile(struct bl_hw *bl_hw, const char *filename,
                              struct bl_phy_conf_file *config);

int bl_find_rftlv(u8_l * buff_addr, u16_l buff_len, u16_l type, u16_l len, u8_l *value);

int bl_parse_caldata(u8_l * buff_addr, u16_l buff_len, struct mm_set_rftlv_req * req);

int bl_parse_mac_config(char *file_path, struct bl_conf_file *config); 

#endif /* _BL_CFGFILE_H_ */
