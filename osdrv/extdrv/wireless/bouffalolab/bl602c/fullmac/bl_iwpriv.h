/**
 ******************************************************************************
 *
 *  @file bl_iwpriv.h
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

#ifndef _BL_IWPRIV_H_
#define _BL_IWPRIV_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <linux/wireless.h>
#include <net/iw_handler.h>


#include "bl_defs.h"

/*
 # Enum
 ****************************************************************************************
 */
/* Device private IOCTL */
#define BL_DEV_PRIV_IOCTL_DEFAULT    (SIOCDEVPRIVATE)

/* odd number is get command, even number is set command */
#define BL_IOCTL_VERSION        (SIOCIWFIRSTPRIV + 1)
#define BL_IOCTL_WMMCFG         (SIOCIWFIRSTPRIV + 2)
#define BL_IOCTL_DBG            (SIOCIWFIRSTPRIV + 6)
#define BL_IOCTL_SETRATE        (SIOCIWFIRSTPRIV + 8)
#define BL_IOCTL_TEMP           (SIOCIWFIRSTPRIV + 11)
#define BL_IOCTL_RW_MEM         (SIOCIWFIRSTPRIV + 15)

#define IWPRIV_OUT_BUF_LEN 2048

#ifdef CONFIG_BL_MP
#define BL_IOCTL_MP_CFG         (SIOCIWFIRSTPRIV + 3)
#define BL_IOCTL_MP_MS          (SIOCIWFIRSTPRIV + 4)
#define BL_IOCTL_MP_HELP        (SIOCIWFIRSTPRIV + 5)
#define BL_IOCTL_MP_MG          (SIOCIWFIRSTPRIV + 7)
#define BL_IOCTL_MP_IND         (SIOCIWFIRSTPRIV + 9)

#define BL_IOCTL_SET_CMD_TYPE_INT        (SIOCIWFIRSTPRIV + 18)
#define BL_IOCTL_GET_CMD_TYPE_INT        (SIOCIWFIRSTPRIV + 19)

#define IS_VAR_LEN 0xffff0000U
#define SHORT_WAIT_MS 50
#define LONG_WAIT_MS 200
#define NONE 0
#define DEFAULT_TX_DUTY 50

enum {
    SDIO_MSG_HELLO,
    SDIO_MSG_UART_DATA,
};

//ms sub cmds
enum {
    BL_IOCTL_MP_UNICAST_TX,
    BL_IOCTL_MP_11n_RATE,
    BL_IOCTL_MP_11g_RATE,
    BL_IOCTL_MP_11b_RATE,
    BL_IOCTL_MP_TX,
    BL_IOCTL_MP_SET_PKT_FREQ,
    BL_IOCTL_MP_SET_PKT_LEN,
    BL_IOCTL_MP_SET_CHANNEL,
    BL_IOCTL_MP_SET_CWMODE,
    BL_IOCTL_MP_SET_PWR,
    BL_IOCTL_MP_SET_TXDUTY,
    BL_IOCTL_MP_SET_PWR_OFT_EN,
    BL_IOCTL_MP_SET_XTAL_CAP,
    BL_IOCTL_MP_SET_PRIV_PARAM,
    BL_IOCTL_MP_WR_MEM,
    BL_IOCTL_MP_RD_MEM,
};


// normal sub set cmd (type-int mode)
enum {
    PLACE_HOLDER,
};

// normal sub get cmd (type-int mode)
enum {
    //dbg purpose, need firmware to support
    BL_IOCIL_SUB_READ_SCRATCH,
};

//mg sub cmds
enum {
    BL_IOCTL_MP_HELLO,
    BL_IOCTL_MP_UNICAST_RX,
    BL_IOCTL_MP_RX,
    // BL_IOCTL_MP_PM,  //MCU power control like hibernate, sleep, wakeup
    // BL_IOCTL_MP_RESET,
    BL_IOCTL_MP_GET_FW_VER,
    BL_IOCTL_MP_GET_BUILD_INFO,
    BL_IOCTL_MP_GET_POWER,
    BL_IOCTL_MP_GET_FREQ,
    BL_IOCTL_MP_GET_TX_STATUS,
    BL_IOCTL_MP_GET_PKT_FREQ,
    BL_IOCTL_MP_GET_XTAL_CAP,
    BL_IOCTL_MP_GET_CWMODE,
    BL_IOCTL_MP_GET_TX_DUTY,
    BL_IOCTL_MP_EFUSE_RD,
    BL_IOCTL_MP_EFUSE_WR,
    BL_IOCTL_MP_EFUSE_CAP_RD,
    BL_IOCTL_MP_EFUSE_CAP_WR,
    BL_IOCTL_MP_EFUSE_PWR_RD,
    BL_IOCTL_MP_EFUSE_PWR_WR,
    BL_IOCTL_MP_EFUSE_MAC_ADR_RD,
    BL_IOCTL_MP_EFUSE_MAC_ADR_WR,
    BL_IOCTL_MP_GET_TEMPERATURE,
};

typedef int (*parse_func_t)(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *out_len);

struct iwpriv_sub_cmd {
    union {
        char    *cname;
        int32_t iname;
    } sc_name;    
    uint32_t    sc_len;
    uint32_t    sc_var_len;
    char        *sc_helper;
    parse_func_t    sc_var_parser;

    char        *mfg_cmd_name;
    uint32_t    mfg_cmd_len;

    uint32_t    ind_wait_ms;
    char        *ind_exp;
};

struct iwpriv_cmd {
    char     *name;
    struct   iwpriv_sub_cmd *sub_cmd;
    uint32_t sub_cmd_num;
};

struct __attribute__((packed)) sdio_msg {
    uint8_t  len_lsb;
    uint8_t  len_msb;
    uint16_t type;
    uint8_t  payload[];
};

int bl_iwpriv_mp_ms_hdl(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
int bl_iwpriv_mp_mg_hdl(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
int bl_iwpriv_help_hdl(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
int bl_iwpriv_mp_ind_hdl(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
int bl_iwpriv_mp_load_caldata_hdl(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);

#endif

enum {
    CFG_CAPCODE_ID,
    CFG_PWR_OFT_ID,
    CFG_MAC_ID,
    CFG_EN_TCAL_ID,
    CFG_TCHANNEL_OS_ID,
    CFG_TCHANNEL_OS_LOW_ID,
    CFG_PWR_MODE_ID,
};

struct cfg_kv_t {
    char    *key;
    bool    valid;
    u8_l    sv_cnt;
    u8_l    sv[32];
};

struct bl_dev_priv_cmd_node {
    char * name;
    int (*hdl) (struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
};

int bl_iwpriv_dbg_hdl(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);

int bl_iwpriv_wmmcfg_hdl(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
int bl_iwpriv_ver_hdl(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
int bl_iwpriv_temp_read_hdl(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
int bl_caldata_cfg_file_handle(struct bl_hw *bl_hw, char *file_path);
int bl_iwpriv_set_rate_hdl(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
int bl_iwpriv_rw_mem_hdl(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra);
int bl_iwpriv_cmd_set_type_int(struct net_device *dev, struct iw_request_info *info, 
                                        union iwreq_data *wrqu, char *extra);
int bl_iwpriv_cmd_get_type_int(struct net_device *dev, struct iw_request_info *info, 
                                        union iwreq_data *wrqu, char *extra);
#endif
