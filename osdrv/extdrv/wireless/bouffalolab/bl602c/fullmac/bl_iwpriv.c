/**
 ******************************************************************************
 *
 *  @file bl_iwpriv.c
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


#include <linux/version.h>
#include <linux/module.h>
#include <linux/inetdevice.h>
#include <net/cfg80211.h>
#include <net/mac80211.h>
#include <linux/etherdevice.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#include <linux/sched/signal.h>
#endif
#include <linux/mmc/sdio.h>
#include <linux/timer.h>
#include <linux/wireless.h>
#include <net/iw_handler.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "bl_compat.h"
#include "bl_defs.h"
#include "bl_msg_tx.h"
#include "bl_tx.h"
#include "hal_desc.h"
#include "bl_debugfs.h"
#include "bl_cfgfile.h"
#include "bl_irqs.h"
#include "bl_version.h"
#include "bl_ftrace.h"
#include "bl_sdio.h"
#include "bl_main.h"
#include "bl_iwpriv.h"
#include "bl_version.h"
#include "bl_cmds.h"

char fw_signed[60] = {'\0'};

uint32_t get_args_type(struct net_device *dev, uint16_t cmd) {
    const struct iw_priv_args *descr;
    int i;

    descr = NULL;
#ifdef CONFIG_WEXT_PRIV
    for (i = 0; i < dev->wireless_handlers->num_private_args; i++) {
        if (cmd == dev->wireless_handlers->private_args[i].cmd) {
            descr = &dev->wireless_handlers->private_args[i];
            break;
        }
    }
#endif

    if (descr == NULL) {
        printk("%s, unknow cmd, not in iw_priv_args.\r\n", __func__);
        return IW_PRIV_TYPE_CHAR;
    } else {
        if (IW_IS_SET(cmd))
            return (descr->set_args & IW_PRIV_TYPE_MASK);
        else
            return (descr->get_args & IW_PRIV_TYPE_MASK);
    }    
}

int read_file(uint8_t *file_name, uint8_t *buf, uint16_t buf_len) {
    int ret = 0;
    struct file *fp;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
    mm_segment_t fs;
#endif
    loff_t pos;

    fp = filp_open(file_name, O_RDONLY, 0644);
    if (IS_ERR(fp)){
        printk("%s, open file error: %s\n", __func__, file_name);
        return -EPIPE;
    }

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
    fs = get_fs();
    set_fs(KERNEL_DS);
#endif
    printk("%s, open file done: %s\n", __func__, file_name);

    pos =0;
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
    ret = kernel_read(fp, pos, buf, buf_len);
#else
    ret = kernel_read(fp, buf, buf_len, &pos);
#endif
    filp_close(fp, NULL);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
    set_fs(fs);
#endif

    return ret;
}

int load_cal_data(char *file_path, struct cfg_kv_t *cfg_kv, int cfg_num) {
    int ret = 0;
    int i = 0, j=0;
    uint8_t *cfg_buf;
    uint16_t cfg_buf_len = 400;

    cfg_buf = kzalloc(cfg_buf_len, GFP_KERNEL);
    if (cfg_buf == NULL) {
        printk("%s, fail to alloc cfg_buf\n", __func__);
        return -ENOMEM;
    }
    memset(cfg_buf, 0, cfg_buf_len);
    
    ret = read_file(file_path, cfg_buf, cfg_buf_len-1);
    if (ret < 0) {
        printk("%s, read file error, ret:%d\n", __func__, ret);
        kfree(cfg_buf);
        return ret;
    }
    //printk("%s, read, ret:%d", __func__, ret);
    cfg_buf[ret] = '\0';
    
    //debug
    //dump_buf_char(cfg_buf, ret);
    //dump_buf(cfg_buf, ret);
    
    for (i=0; i < cfg_num; i++) {
        uint8_t *start_pos = NULL;
        uint8_t *eq_pos = NULL;
        uint8_t *end_pos = NULL;

        if (cfg_kv[i].key == NULL)
            continue;
            
        start_pos = strstr(cfg_buf, cfg_kv[i].key);
        if (start_pos != NULL) {
            printk("%s, Find key in cfg_buf, key:%s\n", __func__, cfg_kv[i].key);
            eq_pos = strchr(start_pos, '=');
            end_pos = strchr(start_pos, '\n');
            if (eq_pos != NULL && end_pos != NULL) {
                BL_DBG("%s, Find = and end_pos, 0x%x, 0x%x\n", __func__, 
                        (uint32_t)eq_pos, (uint32_t)end_pos);
    
                if (i == CFG_CAPCODE_ID) {
                    int capcode = 0;
                    ret = sscanf(eq_pos+1, "%d", &capcode);
                    if (ret != 1) {
                        printk("%s, not valid capcode in decimal\n", __func__);
                        continue;
                    }
                    cfg_kv[i].sv[0] = (uint8_t)capcode;
                    printk("cap code %d\n", cfg_kv[i].sv[0]);
                } else if (i == CFG_MAC_ID) {
                    ret = sscanf(eq_pos+1, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", 
                                 &cfg_kv[i].sv[0], &cfg_kv[i].sv[1], &cfg_kv[i].sv[2],
                                 &cfg_kv[i].sv[3], &cfg_kv[i].sv[4], &cfg_kv[i].sv[5]);
                    if (ret != 6) {
                        printk("%s, not enough valid mac addr in hex like 11:22:33:44:55:66\n", __func__);
                        continue;
                    }
                    for(j=0; j<6; j++)
                        printk("0x%x ",cfg_kv[i].sv[j]);
                    printk("\n");
                } else if (i == CFG_PWR_OFT_ID) {
                    int p0,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13;
                    ret = sscanf(eq_pos+1, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", 
                                 &p0,&p1,&p2,&p3,&p4,&p5,&p6,&p7,&p8,&p9,&p10,&p11,&p12,&p13);
                    if (ret != 14) {
                        printk("%s, not enough valid power offset in decimal like 1,-1,3,5,8,10,1,0,9,0,1,2,3,14\n", __func__);
                        continue;
                    }
                    cfg_kv[i].sv[0] = (uint8_t)p0;
                    cfg_kv[i].sv[1] = (uint8_t)p1;
                    cfg_kv[i].sv[2] = (uint8_t)p2;
                    cfg_kv[i].sv[3] = (uint8_t)p3;
                    cfg_kv[i].sv[4] = (uint8_t)p4;
                    cfg_kv[i].sv[5] = (uint8_t)p5;
                    cfg_kv[i].sv[6] = (uint8_t)p6;
                    cfg_kv[i].sv[7] = (uint8_t)p7;
                    cfg_kv[i].sv[8] = (uint8_t)p8;
                    cfg_kv[i].sv[9] = (uint8_t)p9;
                    cfg_kv[i].sv[10] = (uint8_t)p10;
                    cfg_kv[i].sv[11] = (uint8_t)p11;
                    cfg_kv[i].sv[12] = (uint8_t)p12;
                    cfg_kv[i].sv[13] = (uint8_t)p13;
                    for(j=0; j<14; j++)
                        printk("%d ",cfg_kv[i].sv[j]);
                    printk("\n");
                } else if (i == CFG_PWR_MODE_ID) {
                    char b1, b2;
                    ret = sscanf(eq_pos+1, "%c%c", &b1, &b2);
                    if (ret != 2) {
                        printk("%s, not enough valid power mode like BF or Bf or bF or bf\n", __func__);
                        continue;
                    }
                    cfg_kv[i].sv[0] = b1;
                    cfg_kv[i].sv[1] = b2;
                    printk("%s, pwr mode:%c%c\n", __func__, b1, b2);
                } else if (i == CFG_EN_TCAL_ID) {
                    int en_tcal = 0;
                    ret = sscanf(eq_pos+1, "%d", &en_tcal);
                    if (ret != 1) {
                        printk("%s, not valid en_tcal in decimal\n", __func__);
                        continue;
                    }
                    cfg_kv[i].sv[0] = (uint8_t)en_tcal;
                    printk("en_tcal %d\n", cfg_kv[i].sv[0]);
                } else if (i == CFG_TCHANNEL_OS_ID) {
                    int t0, t1, t2, t3, t4;
                    ret = sscanf(eq_pos+1, "%d,%d,%d,%d,%d", &t0,&t1,&t2,&t3,&t4);
                    if (ret != 5) {
                        printk("%s, not enough valid Tchannel_os in decimal like 180,168,163,160,157\n", __func__);
                        continue;
                    }
                    cfg_kv[i].sv[0] = (uint8_t)(t0 & 0xff);
                    cfg_kv[i].sv[1] = (uint8_t)((t0 & 0xff)>>8);
                    cfg_kv[i].sv[2] = (uint8_t)(t1 & 0xff);
                    cfg_kv[i].sv[3] = (uint8_t)((t1 & 0xff)>>8);
                    cfg_kv[i].sv[4] = (uint8_t)(t2 & 0xff);
                    cfg_kv[i].sv[5] = (uint8_t)((t2 & 0xff)>>8);
                    cfg_kv[i].sv[6] = (uint8_t)(t3 & 0xff);
                    cfg_kv[i].sv[7] = (uint8_t)((t3 & 0xff)>>8);
                    cfg_kv[i].sv[8] = (uint8_t)(t4 & 0xff);
                    cfg_kv[i].sv[9] = (uint8_t)((t4 & 0xff)>>8);
                    for(j=0; j<10; j++)
                        printk("%d ",cfg_kv[i].sv[j]);
                    printk("\n");
                } 
				else if (i == CFG_TCHANNEL_OS_LOW_ID) {
                    int t0, t1, t2,t3,t4;
                    ret = sscanf(eq_pos+1, "%d,%d,%d,%d,%d", &t0,&t1,&t2,&t3,&t4);
                    if (ret != 5) {
                        printk("%s, not enough valid Tchannel_os_low in decimal like 180,168,163,160,157\n", __func__);
                        continue;
                    }
                    cfg_kv[i].sv[0] = (uint8_t)(t0 & 0xff);
                    cfg_kv[i].sv[1] = (uint8_t)((t0 & 0xff00)>>8);
                    cfg_kv[i].sv[2] = (uint8_t)(t1 & 0xff);
                    cfg_kv[i].sv[3] = (uint8_t)((t1 & 0xff00)>>8);
                    cfg_kv[i].sv[4] = (uint8_t)(t2 & 0xff);
                    cfg_kv[i].sv[5] = (uint8_t)((t2 & 0xff00)>>8);
                    cfg_kv[i].sv[6] = (uint8_t)(t3 & 0xff);
                    cfg_kv[i].sv[7] = (uint8_t)((t3 & 0xff00)>>8);
                    cfg_kv[i].sv[8] = (uint8_t)(t4 & 0xff);
                    cfg_kv[i].sv[9] = (uint8_t)((t4 & 0xff00)>>8);
                    for(j=0; j<10; j++)
                        printk("%d ",cfg_kv[i].sv[j]);
                    printk("\n");
                }
				else {
                    printk("%s, cfg id %d is not handled\n", __func__, i);
                    continue;
                }
                cfg_kv[i].valid = true;
            } else {
                printk("%s, Not find = and end_pos, 0x%x, 0x%x\n", 
                        __func__, (uint32_t)eq_pos, (uint32_t)end_pos);
            }
        } else {
            printk("%s, Not find key in cfg_buf, key:%s\n", __func__, cfg_kv[i].key);
        }
    }


    if (cfg_buf != NULL)
        kfree(cfg_buf);

    return 0;
}

#ifdef CONFIG_BL_MP
void dump_buf(const void *buf, const unsigned long buf_len)
{
    unsigned long i = 0;

    for (; i < buf_len; i++) {
        printk("%02X ", ((uint8_t *)buf)[i]);
    }
    if (!(i % 16 == 0)) {
        BL_DBG("\n");
    }
}

void dump_buf_char(const void *buf, const unsigned long buf_len)
{    
    unsigned long i = 0;

    for (; i < buf_len; i++) {
        printk("%c", ((uint8_t *)buf)[i]);
    }
    BL_DBG("\n");
}

static int bl_iwpriv_common_hdl(struct net_device *dev, uint16_t cmd, union iwreq_data *wrqu, 
                         char *extra, struct iwpriv_cmd *iwp_cmds, uint32_t sc_num, 
                         uint16_t sc_index, uint8_t *in_buf, uint16_t in_len);
static int parse_str_2_one_bool(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len);
static int parse_str_2_one_uint(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len);
static int parse_one_bool(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len);
static int parse_one_uint(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len);
static int parse_set_duty(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len);
static int parse_set_xtal_cap(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len);
static int parse_set_misc_param(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len);
static int parse_wr_mem(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len);
static int parse_rd_mem(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len);
static int parse_pkt_len(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len);
static int parse_unicast_tx_param(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len);
static int parse_11n_rate(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len);
static int parse_11g_rate(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len);
static int parse_11b_rate(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len);
static int parse_set_pkt_freq(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len);
static int parse_set_channel(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len);
static int parse_set_power(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len);
static int parse_sleep_dtim(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len);
static int parse_help_args(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len);

//single
struct iwpriv_sub_cmd iwp_mp_sc_ver[] = 
{
    {
        .sc_name.cname =    NULL,
        .sc_len =           0,
        .sc_var_len =       0,
        .sc_helper =        "iwpriv <interface> version\r\n\
                            example: sudo iwpriv wlan0 version\r\n",
        .mfg_cmd_name =     NULL,
        .mfg_cmd_len =      0,
        .ind_wait_ms =      0,
    },
};

struct iwpriv_sub_cmd iwp_mp_sc_help[] = 
{
    {
        .sc_name.cname =    NULL,
        .sc_len =           0,
        .sc_var_len =       IS_VAR_LEN|0,
        .sc_var_parser =    parse_help_args,
        .mfg_cmd_name =     NULL,
        .mfg_cmd_len =      0,
        .ind_wait_ms =      0,
    },
};


//ms
struct iwpriv_sub_cmd iwp_mp_sc_uc_tx[] = 
{
    {
        .sc_name.iname =    1,          //tx_on
        .sc_len =           4,
        .sc_var_len =       4,          //1 uint, as "<pkt number to be sent>"
        .sc_var_parser =    parse_unicast_tx_param,
        .sc_helper =        "iwpriv <interface> mp_uc_tx <on> <pkt number to be sent>\r\n \
                            on: 1\r\n \
                            pkt number to be sent: > 0\r\n \
                            example: sudo iwpriv wlan0 mp_uc_tx 1 100\r\n",
        .mfg_cmd_name =     "ut1",      //Or "UT1"
        .mfg_cmd_len =      3,
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "unicat tx on",
    },
    {
        .sc_name.iname =    0,          //tx_off
        .sc_len =           4,
        .sc_var_len =       0,
        .sc_var_parser =    NULL,
        .sc_helper =        "iwpriv <interface> mp_uc_tx <off>\r\n \
                            off: 0\r\n \
                            example: sudo iwpriv wlan0 mp_uc_tx 0\r\n",
        .mfg_cmd_name =     "ut0\n",    //Or "UT0\n"
        .mfg_cmd_len =      5,          //include \0
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "unicat tx off",
    },
};

struct iwpriv_sub_cmd iwp_mp_sc_11n_rate[] = 
{
    {
        .sc_name.iname =    NONE,
        .sc_len =           0,
        .sc_var_len =       12,         //3 uint32, 3*4 bytes
        .sc_var_parser =    parse_11n_rate,
        .sc_helper =        "iwpriv <interface> mp_11n_rate <short long GI> <mcs_index> <BW>\r\n \
                            short guard interval: 0, long guard interval: 1\r\n \
                            mcs_index: 0-7\r\n \
                            bandwidth: 2 for 20MHz, 4 for 40MHz, while 40MHz is not support on BL602\r\n \
                            example: sudo iwpriv wlan0 mp_11n_rate 0 7 2\r\n",
        .mfg_cmd_name =     NULL,
        .mfg_cmd_len =      0,
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "[TX] Use HT MC",
    },
};

struct iwpriv_sub_cmd iwp_mp_sc_11g_rate[] = 
{
    {
        .sc_name.iname =    0,          //short preamble
        .sc_len =           4,
        .sc_var_len =       4,          //1 uint32, 1*4 bytes, as "<rate idx>"
        .sc_var_parser =    parse_11g_rate,
        .sc_helper =        "iwpriv <interface> mp_11g_rate <short long preamble> <rate idx>\r\n \
                            short preamble: 0, long preamble: 1, \r\n \
                            rate idx: 2.4G 11g rate idx = 0 - 7, 0:6Mbps 1:9Mbps 2:12Mbps 3:18Mbps 4:24Mbps 5:36Mbps 6:48Mbps 7:54Mbps\r\n \
                            example: sudo iwpriv wlan0 mp_11g_rate 1 7\r\n",
        .mfg_cmd_name =     "g",
        .mfg_cmd_len =      1,
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "[TX] Use 11g basic rate",
    },
    {
        .sc_name.iname =    1,          //long preamble
        .sc_len =           4,
        .sc_var_len =       4,          //1 uint32, 1*4 bytes, as "<rate>"
        .sc_var_parser =    parse_11g_rate,
        .sc_helper =        NULL,
        .mfg_cmd_name =     "G",
        .mfg_cmd_len =      1,
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "[TX] Use 11g basic rate",
    },
};

struct iwpriv_sub_cmd iwp_mp_sc_11b_rate[] = 
{
    {
        .sc_name.iname =    0,          //short preamble
        .sc_len =           4,
        .sc_var_len =       4,          //1 uint32, 1*4 bytes, as "<rate idx>"
        .sc_var_parser =    parse_11b_rate,
        .sc_helper =        "iwpriv <interface> mp_11b_rate <short long preamble> <rate idx>\r\n \
                            short preamble: 0, long preamble: 1, \r\n \
                            rate idx: 2.4G 11b rate idx = 0 - 3, 0:1Mbps, 1:2Mbps, 2:5.5Mbps, 3:11Mbps\r\n \
                            example: sudo iwpriv wlan0 mp_11b_rate 1 2\r\n",
        .mfg_cmd_name =     "B",
        .mfg_cmd_len =      1,
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "[TX] Use 11b rate",
    },
    {
        .sc_name.iname =    1,          //long preamble
        .sc_len =           4,
        .sc_var_len =       4,          //1 uint32, 1*4 bytes, as "<rate>", 2.4G 11b rate idx = 0 - 3, 0:1Mbps, 1:2Mbps, 2:5.5Mbps, 3:11Mbps
        .sc_var_parser =    parse_11b_rate,
        .mfg_cmd_name =     "b",
        .mfg_cmd_len =      1,
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "[TX] Use 11b rate",
    },
};

struct iwpriv_sub_cmd iwp_mp_sc_tx[] = 
{
    {
        .sc_name.iname =    NONE,
        .sc_len =           0,
        .sc_var_len =       4,          //1 uint32, 1*4 bytes, as "<on/off>"
        .sc_var_parser =    parse_one_bool,
        .sc_helper =        "iwpriv <interface> mp_tx <on/off>\r\n\
                            off: 0, on: 1, \r\n\
                            example: sudo iwpriv wlan0 mp_tx 1\r\n\
                                     sudo iwpriv wlan0 mp_tx 0\r\n",
        .mfg_cmd_name =     "t",        //Or "T1\n"
        .mfg_cmd_len =      1,
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "[TX] Toggle",
    },
};

struct iwpriv_sub_cmd iwp_mp_sc_set_pkt_freq[] = 
{
    {
        .sc_name.iname =    NONE,
        .sc_len =           0,
        .sc_var_len =       4,          //1 uint32, 1*4 bytes, as "<pkt_freq>"
        .sc_var_parser =    parse_set_pkt_freq,
        .sc_helper =        "iwpriv <interface> mp_set_pkt_freq <pkt_freq>\r\n\
                            pkt_freq: pkt to be sent per seconds, {1-1000}\r\n\
                            example: sudo iwpriv wlan0 mp_set_pkt_freq 300\r\n",
        .mfg_cmd_name =     "f",        //Or 'F'
        .mfg_cmd_len =      1,
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "tx frenquecy:",
    },
};

struct iwpriv_sub_cmd iwp_mp_sc_set_pkt_len[] = 
{
    {
        .sc_name.iname =    NONE,
        .sc_len =           0,
        .sc_var_len =       4,          //1 uint32, 1*4 bytes, as "<pkt_len>"
        .sc_var_parser =    parse_pkt_len,
        .sc_helper =        "iwpriv <interface> mp_set_pkt_len <pkt_len>\r\n\
                            pkt_len: length of pkt to be sent, 1968 by default\r\n\
                            valid range is [1, 4096] and not 1000\r\n\
                            example: sudo iwpriv wlan0 mp_set_pkt_len 330\r\n",
        .mfg_cmd_name =     "l",        //Or 'l'
        .mfg_cmd_len =      1,
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "[TX] len=",
    },
};

struct iwpriv_sub_cmd iwp_mp_sc_set_ch[] = 
{
    {
        .sc_name.iname =    NONE,
        .sc_len =           0,
        .sc_var_len =       4,          //1 uint, 4 bytes, as "<channel idx>", which is in [1-13]
        .sc_var_parser =    parse_set_channel,
        .sc_helper =        "iwpriv <interface> mp_set_channel <channel idx>\r\n \
                            channel idx: which is in range {1-13}\r\n \
                            example: sudo iwpriv wlan0 mp_set_channel 13\r\n",
        .mfg_cmd_name =     "c",
        .mfg_cmd_len =      1,
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "[mfg fw]",
    },
};

struct iwpriv_sub_cmd iwp_mp_sc_set_cw[] = 
{
    {
        .sc_name.iname =    NONE,
        .sc_len =           0,
        .sc_var_len =       4,          //1 uint32, 1*4 bytes, as "<mode>"
        .sc_var_parser =    parse_one_bool,
        .sc_helper =        "iwpriv <interface> mp_set_cw_mode <mode>\r\n \
                            mode: 0 for none CW mode, 1 for CW mode\r\n \
                            example: sudo iwpriv wlan0 mp_set_cw_mode 0\r\n",
        .mfg_cmd_name =     "M",
        .mfg_cmd_len =      1,
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "[mfg fw]",
    },
};

struct iwpriv_sub_cmd iwp_mp_sc_set_pwr[] = 
{
    {
        .sc_name.iname =    NONE,
        .sc_len =           0,
        .sc_var_len =       4,          //1 uint32, 1*4 bytes, as "<power>", which is in 12dbm ~ 23dbm
        .sc_var_parser =    parse_set_power,
        .sc_helper =        "iwpriv <interface> mp_set_power <power>\r\n \
                            power: tx power value, which is in range {12-23}\r\n \
                            example: sudo iwpriv wlan0 mp_set_power 22\r\n",
        .mfg_cmd_name =     "p",        //Or "P"
        .mfg_cmd_len =      1,
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "[mfg fw]",
    },
};

struct iwpriv_sub_cmd iwp_mp_sc_set_tx_duty[] = 
{
    {
        .sc_name.iname =    NONE,
        .sc_len =           0,
        .sc_var_len =       4,          //1 uint32, 1*4 bytes, <duty value>, [0-100]
        .sc_var_parser =    parse_set_duty,
        .sc_helper =        "iwpriv <interface> mp_set_tx_duty <duty>\r\n \
                            tx duty value, {0-100}\r\n \
                            example: sudo iwpriv wlan0 mp_set_tx_duty 40\r\n",
        .mfg_cmd_name =     "d",
        .mfg_cmd_len =      1,
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "[mfg fw]",
    },
};

struct iwpriv_sub_cmd iwp_mp_sc_set_pwr_offset_en[] = 
{
    {
        .sc_name.iname =    NONE,
        .sc_len =           0,
        .sc_var_len =       4,           //1 uint32, 1*4 bytes, 
        .sc_var_parser =    parse_one_bool,
        .sc_helper =        "iwpriv <interface> mp_en_pwr_oft <1/0>\r\n \
                            reload power offset calibration value, or not reload\r\n \
                            example: sudo iwpriv wlan0 mp_en_pwr_oft 1\r\n",
        .mfg_cmd_name =     "V",         //include \0
        .mfg_cmd_len =      1,
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "offset",       //"[mfg fw]",
    },
};

struct iwpriv_sub_cmd iwp_mp_sc_set_xtal_cap[] = 
{
    {
        .sc_name.iname =    NONE,
        .sc_len =           0,
        .sc_var_len =       4,           //1 uint32, 1*4 bytes, "<N>",  > 0
        .sc_var_parser =    parse_set_xtal_cap,
        .sc_helper =        "iwpriv <interface> mp_set_xtal_cap <cap code>\r\n \
                            cap code: value which is bigger than 0\r\n \
                            example: sudo iwpriv wlan0 mp_set_xtal_cap 0\r\n",
        .mfg_cmd_name =     "X",         //Or "x"
        .mfg_cmd_len =      1,
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "[mfg fw]",
    },
};

struct iwpriv_sub_cmd iwp_mp_sc_set_misc_param[] = 
{
    {
        .sc_name.iname =    NONE,
        .sc_len =           0,
        .sc_var_len =       12,           //at least, 3 uint32, 3*4 bytes
        .sc_var_parser =    parse_set_misc_param,
        .sc_helper =        "iwpriv <interface> mp_set_misc_param <tcal_debug> <tcal_val_fix> <tcal_val_fix_value>\r\n \
                            example: sudo iwpriv wlan0 mp_set_misc_param 1 0 0\r\n",
        .mfg_cmd_name =     "i",
        .mfg_cmd_len =      1,
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "[mfg fw]",
    },
};

struct iwpriv_sub_cmd iwp_mp_sc_wr_mem[] = 
{
    {
        .sc_name.cname =    NULL,
        .sc_len =           0,
        .sc_var_len =       IS_VAR_LEN|8,   //At least 2 uint32
        .sc_var_parser =    parse_wr_mem,
        .sc_helper =        "iwpriv <interface> mp_wr_mem <addr in hex> <value in hex> [bits len] [lowest bit pos]\r\n \
                            addr should be hex value like 0x22008000, can be reg addr or mem addr, align 4 bytes\r\n \
                            if write memory, 2nd param is uint32 value\r\n \
                            if write register, 2nd param is reg value of some bits, 3rd param is bits len and 4th param is lowest bit pos\r\n \
                            sudo iwpriv wlan0 mp_wr_mem 0x22008400 7 4 16;sudo iwpriv wlan0 mp_ind\r\n \
                            sudo iwpriv wlan0 mp_wr_mem 0x22008400 0x12345678;sudo iwpriv wlan0 mp_ind\r\n \
                            sudo iwpriv wlan0 mp_wr_mem 0x40001064 11 4 15;sudo iwpriv wlan0 mp_ind",
        .mfg_cmd_name =     "w",
        .mfg_cmd_len =      1,
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "success, write addr", //"[mfg fw]",
    },
};

struct iwpriv_sub_cmd iwp_mp_sc_rd_mem[] = 
{
    {
        .sc_name.cname =    NULL,
        .sc_len =           0,
        .sc_var_len =       IS_VAR_LEN|4,   //At least 4 bytes, addr
        .sc_var_parser =    parse_rd_mem,
        .sc_helper =        "iwpriv <interface> mp_rd_mem <addr in hex> [number of uint32/bits] [bit pos]\r\n \
                            addr should be hex value like 0x22008000, can be reg addr or mem addr, align 4 bytes\r\n \
                            if read memory, 2nd param is number of uint32 to read, or omit 2nd param then use default value 4 in mfg fw. 3rd is omitted\r\n \
                            if read register, 2nd param should be 1 to read 32bits reg value, or 2nd param as bits len and 3rd param as bit pos to read register bits\r\n \
                            sudo iwpriv wlan0 mp_rd_mem 0x22008400 4 16;sudo iwpriv wlan0 mp_ind\r\n \
                            sudo iwpriv wlan0 mp_rd_mem 0x22008400 3;sudo iwpriv wlan0 mp_ind\r\n \
                            sudo iwpriv wlan0 mp_rd_mem 0x22008400;sudo iwpriv wlan0 mp_ind\r\n",
        .mfg_cmd_name =     "I",
        .mfg_cmd_len =      1,
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "[mfg fw]",
    },
};

//mg
struct iwpriv_sub_cmd iwp_mp_sc_hello[] = 
{
    {
        .sc_name.cname =    NULL,
        .sc_len =           0,
        .sc_var_len =       0,
        .sc_helper =        "iwpriv <interface> mp_hello\r\n\
                            first msg to check fw running up\r\n\
                            example: sudo iwpriv wlan0 mp_hello\r\n",
        .mfg_cmd_name =     "H\n",       //include \0
        .mfg_cmd_len =      3,
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "[mfg fw]",
    },
};

struct iwpriv_sub_cmd iwp_mp_sc_uc_rx[] = 
{
    {
        .sc_name.cname =    "1",         //rx_on
        .sc_len =           1,
        .sc_var_len =       0,
        .sc_helper =        "iwpriv <interface> mp_uc_rx <on/off>\r\n \
                            on: 1, off: 0\r\n \
                            example: sudo iwpriv wlan0 mp_uc_rx 1\r\n \
                                     sudo iwpriv wlan0 mp_uc_rx 0\r\n",
        .mfg_cmd_name =     "ur1\n",     //Or "UR1\n"
        .mfg_cmd_len =      5,           //include \0
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "[mfg fw]",
    },
    {
        .sc_name.cname =    "0",         //rx_off
        .sc_len =           1,
        .sc_var_len =       0,
        .mfg_cmd_name =     "ur0\n",     //Or "UR0\n"
        .mfg_cmd_len =      5,           //include \0
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "[mfg fw]",
    },
};

struct iwpriv_sub_cmd iwp_mp_sc_rx[] = 
{
    {
        .sc_name.cname =    "1",         //rx on
        .sc_len =           1,
        .sc_var_len =       0,
        .sc_helper =        "iwpriv <interface> mp_rx <on_off>\r\n \
                            on: 1, off: 0\r\n \
                            example: sudo iwpriv wlan0 mp_rx 1\r\n \
                                     sudo iwpriv wlan0 mp_rx 0\r\n",
        .mfg_cmd_name =     "r:s\n",
        .mfg_cmd_len =      5,
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "[mfg fw]",
    },
    {
        .sc_name.cname =    "0",         //rx off and get status of received pkts
        .sc_len =           1,
        .sc_var_len =       0,
        .mfg_cmd_name =     "r:g\n",
        .mfg_cmd_len =      5,           //include \0
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "[mfg fw]",
    },
};

struct iwpriv_sub_cmd iwp_mp_sc_pm[] = 
{
    {
        .sc_name.cname =    "hibernate",
        .sc_len =           9,
        .sc_var_len =       IS_VAR_LEN|1,//At least 1 bytes, as "<number of seconds to hibernate and then wake up by rtc>"
        .sc_var_parser =    parse_str_2_one_uint,
        .mfg_cmd_name =     "hr",
        .mfg_cmd_len =      2,
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "[mfg fw]",
    },
    {
        .sc_name.cname =    "sleep_forever",
        .sc_len =           13,
        .sc_var_len =       0,
        .mfg_cmd_name =     "sa\n",
        .mfg_cmd_len =      4,
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "[mfg fw]",
    },
    {
        .sc_name.cname =    "sleep_level",
        .sc_len =           11,
        .sc_var_len =       IS_VAR_LEN|1,   //At least 1 bytes, as "<PDS(power down sleep) level>"
        .sc_var_parser =    parse_str_2_one_uint,
        .mfg_cmd_name =     "sl",
        .mfg_cmd_len =      2,
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "[mfg fw]",
    },
    {
        .sc_name.cname =    "sleep_dtim",
        .sc_len =           10,
        .sc_var_len =       IS_VAR_LEN|3,   //At least 3 bytes, as "<dtim count>,<exit_count>"
        .sc_var_parser =    parse_sleep_dtim,
        .mfg_cmd_name =     "s:",
        .mfg_cmd_len =      2,
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "[mfg fw]",
    },
    {
        .sc_name.cname =    "awakekeep_dtim",
        .sc_len =           14,
        .sc_var_len =       IS_VAR_LEN|1,   //At least 1 bytes, as "<keep rx time>", the keep awake and rx time in ms
        .sc_var_parser =    parse_str_2_one_uint,
        .mfg_cmd_name =     "a:w",          //Or "A:w"
        .mfg_cmd_len =      3,
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "[mfg fw]",
    },
};

struct iwpriv_sub_cmd iwp_mp_sc_reset[] = 
{
    {
        .sc_name.cname =    NULL,
        .sc_len =           0,
        .sc_var_len =       0,
        .mfg_cmd_name =     "Reset\n",
        .mfg_cmd_len =      7,
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "[mfg fw]",
    },
};

struct iwpriv_sub_cmd iwp_mp_sc_get_fw_ver[] = 
{
    {
        .sc_name.cname =    NULL,
        .sc_len =           0,
        .sc_var_len =       0,
        .sc_helper =        "iwpriv <interface> mp_get_fw_ver\r\n \
                            get firmware version\r\n \
                            example: sudo iwpriv wlan0 mp_get_fw_ver\r\n",
        .mfg_cmd_name =     "y:v\n",      //Or "Y:v\n"
        .mfg_cmd_len =      5,
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "[mfg fw]",
    },
};

struct iwpriv_sub_cmd iwp_mp_sc_get_build_info[] = 
{
    {
        .sc_name.cname =    NULL,
        .sc_len =           0,
        .sc_var_len =       0,
        .sc_helper =        "iwpriv <interface> mp_get_buidinfo\r\n \
                            get firmware build info\r\n \
                            example: sudo iwpriv wlan0 mp_get_buidinfo\r\n",
        .mfg_cmd_name =     "y:d\n",
        .mfg_cmd_len =      5,
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "[mfg fw]",
    },
};

struct iwpriv_sub_cmd iwp_mp_sc_get_pwr[] = 
{
    {
        .sc_name.cname =    NULL,
        .sc_len =           0,
        .sc_var_len =       0,
        .sc_helper =        "iwpriv <interface> mp_get_power\r\n \
                            get tx power\r\n \
                            example: sudo iwpriv wlan0 mp_get_power\r\n",
        .mfg_cmd_name =     "y:p\n",
        .mfg_cmd_len =      5,
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "[mfg fw]",
    },
};

struct iwpriv_sub_cmd iwp_mp_sc_get_freq[] = 
{
    {
        .sc_name.cname =    NULL,
        .sc_len =           0,
        .sc_var_len =       0,
        .sc_helper =        "iwpriv <interface> mp_get_channel\r\n \
                            get frequency\r\n \
                            example: sudo iwpriv wlan0 mp_get_channel\r\n",
        .mfg_cmd_name =     "y:c\n",
        .mfg_cmd_len =      5,
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "[mfg fw]",
    }, 
};

struct iwpriv_sub_cmd iwp_mp_sc_get_tx_state[] = 
{   
    {
        .sc_name.cname =    NULL,
        .sc_len =           0,
        .sc_var_len =       0,
        .sc_helper =        "iwpriv <interface> mp_get_tx_state\r\n \
                            get tx onoff status\r\n \
                            example: sudo iwpriv wlan0 mp_get_tx_state\r\n",
        .mfg_cmd_name =     "y:t\n",
        .mfg_cmd_len =      5,
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "[mfg fw]",
    },
};

struct iwpriv_sub_cmd iwp_mp_sc_get_pkt_freq[] = 
{
    {
        .sc_name.cname =    NULL,
        .sc_len =           0,
        .sc_var_len =       0,
        .sc_helper =        "iwpriv <interface> mp_get_pkt_freq\r\n \
                            get number of pkts sent every second\r\n \
                            example: sudo iwpriv wlan0 mp_get_pkt_freq\r\n",
        .mfg_cmd_name =     "y:f\n",
        .mfg_cmd_len =      5,
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "[mfg fw]",
    },
};

struct iwpriv_sub_cmd iwp_mp_sc_get_xtal_cap[] = 
{
    {
        .sc_name.cname =    NULL,
        .sc_len =           0,
        .sc_var_len =       0,
        .sc_helper =        "iwpriv <interface> mp_get_xtal_cap\r\n \
                            get xtal cap code\r\n \
                            example: sudo iwpriv wlan0 mp_get_xtal_cap\r\n",
        .mfg_cmd_name =     "y:x\n",
        .mfg_cmd_len =      5,
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "[mfg fw]",
    },
};

struct iwpriv_sub_cmd iwp_mp_sc_get_cwmode[] = 
{
    {
        .sc_name.cname =    NULL,
        .sc_len =           0,
        .sc_var_len =       0,
        .sc_helper =        "iwpriv <interface> mp_get_cwmode\r\n \
                            get CW mode value\r\n \
                            example: sudo iwpriv wlan0 mp_get_cwmode\r\n",
        .mfg_cmd_name =     "y:M\n",
        .mfg_cmd_len =      5,
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "[mfg fw]",
    },
};

struct iwpriv_sub_cmd iwp_mp_sc_get_tx_duty[] = 
{
    {
        .sc_name.cname =    NULL,
        .sc_len =           0,
        .sc_var_len =       0,
        .sc_helper =        "iwpriv <interface> mp_get_tx_duty\r\n \
                            get tx_duty\r\n \
                            example: sudo iwpriv wlan0 mp_get_tx_duty\r\n",
        .mfg_cmd_name =     "y:i\n",
        .mfg_cmd_len =      5,
        .ind_wait_ms =      SHORT_WAIT_MS,
        .ind_exp =          "[mfg fw]",
    }, 
};

struct iwpriv_sub_cmd iwp_mp_sc_efuse_rd[] = 
{
    {
        .sc_name.cname =    NULL,
        .sc_len =           0,
        .sc_var_len =       IS_VAR_LEN|1,   //At least 1 byte
        .sc_helper =        "iwpriv <interface> mp_efuse_read <addr in hex>\r\n \
                            addr should be hex value\r\n \
                            read back the value at efuse's specific addr\r\n \
                            example: sudo iwpriv wlan0 mp_efuse_read 0x00000004\r\n",
        .mfg_cmd_name =     "REA",
        .mfg_cmd_len =      3,
        .ind_wait_ms =      LONG_WAIT_MS,
        .ind_exp =          "Rd ef",        //"[mfg fw] REA",
    },
};

struct iwpriv_sub_cmd iwp_mp_sc_efuse_wr[] = 
{
    {
        .sc_name.cname =    NULL,
        .sc_len =           0,
        .sc_var_len =       IS_VAR_LEN|3,   //At least 3 bytes
        .sc_helper =        "iwpriv <interface> mp_efuse_write <addr in hex>=<value in hex>\r\n \
                            addr and value should be hex value\r\n \
                            want to write value to efuse's specific addr, but only store in specific buffer first\r\n \
                            example: sudo iwpriv wlan0 mp_efuse_write 0x004=0x80000008\r\n",
        .mfg_cmd_name =     "WEA",
        .mfg_cmd_len =      3,
        .ind_wait_ms =      LONG_WAIT_MS,
        .ind_exp =          "Wr to ef to",
    },
};

struct iwpriv_sub_cmd iwp_mp_sc_efuse_cap_rd[] = 
{
    {
        .sc_name.cname =    NULL,
        .sc_len =           0,
        .sc_var_len =       0,
        .sc_helper =        "iwpriv <interface> mp_efuse_cap_rd\r\n \
                            read back the xtal cap code value at xtal cap code specific addr in efuse\r\n \
                            example: sudo iwpriv wlan0 mp_efuse_cap_rd\r\n",
        .mfg_cmd_name =     "REX\n",
        .mfg_cmd_len =      5,
        .ind_wait_ms =      LONG_WAIT_MS,
        .ind_exp =          "Cap code2:",   //"[mfg fw] REX OK",
    },
};

struct iwpriv_sub_cmd iwp_mp_sc_efuse_cap_wr[] = 
{
    {
        .sc_name.cname =    NULL,
        .sc_len =           0,
        .sc_var_len =       IS_VAR_LEN|1,   //At least 1 bytes, xtal cap code
        .sc_helper =        "iwpriv <interface> mp_efuse_cap_wr <xtal cap code>\r\n \
                            value should be decimal value\r\n \
                            want to write xtal cap code to xtal cap code specific addr in efuse, but only store in specific buffer first\r\n \
                            example: sudo iwpriv wlan0 mp_efuse_cap_wr 3\r\n",
        .mfg_cmd_name =     "WEX",
        .mfg_cmd_len =      3,
        .ind_wait_ms =      LONG_WAIT_MS,
        .ind_exp =          "[mfg fw] WEX", //"Cap code2 cmd", "Save cap code2 OK",
    },
};

struct iwpriv_sub_cmd iwp_mp_sc_efuse_pwr_rd[] = 
{
    {
        .sc_name.cname =    NULL,
        .sc_len =           0,
        .sc_var_len =       0,
        .sc_helper =        "iwpriv <interface> mp_efuse_pwr_rd\r\n \
                            read back the each channel power offset value at power offset specific addr in efuse\r\n \
                            example: sudo iwpriv wlan0 mp_efuse_pwr_rd\r\n",
        .mfg_cmd_name =     "REP\n",
        .mfg_cmd_len =      5,
        .ind_wait_ms =      LONG_WAIT_MS,
        .ind_exp =          "Power offset:", //"[mfg fw] REP OK",
    },
};

struct iwpriv_sub_cmd iwp_mp_sc_efuse_pwr_wr[] = 
{
    {
        .sc_name.cname =    NULL,
        .sc_len =           0,
        .sc_var_len =       IS_VAR_LEN|14,   //At least 14 bytes, 14 power offset values for 1-14 channels
        .sc_helper =        "iwpriv <interface> mp_efuse_pwr_wr <ch1 power offset>,<ch2 power offset>....<ch14 power offset>\r\n \
                            each channel's power offset value should be decimal value\r\n \
                            want to write each channel's power offset to power offset specific addr in efuse, but only store in specific buffer first\r\n \
                            example: sudo iwpriv wlan0 mp_efuse_pwr_wr 1,2,3,3,3,2,1,0,1,2,3,4,1,3\r\n",
        .mfg_cmd_name =     "WEP",
        .mfg_cmd_len =      3,
        .ind_wait_ms =      LONG_WAIT_MS,
        .ind_exp =          "[mfg fw] WEP", //"Pwr offset cmd:",
    },
};

struct iwpriv_sub_cmd iwp_mp_sc_efuse_mac_addr_rd[] = 
{
    {
        .sc_name.cname =    NULL,
        .sc_len =           0,
        .sc_var_len =       0,
        .sc_helper =        "iwpriv <interface> mp_efuse_mac_rd\r\n \
                            read back the mac addr in efuse\r\n \
                            example: sudo iwpriv wlan0 mp_efuse_mac_rd\r\n",
        .mfg_cmd_name =     "REM\n",
        .mfg_cmd_len =      5,
        .ind_wait_ms =      LONG_WAIT_MS,
        .ind_exp =          "[mfg fw] REM OK",
    },
};

struct iwpriv_sub_cmd iwp_mp_sc_efuse_mac_addr_wr[] = 
{
    {
        .sc_name.cname =    NULL,
        .sc_len =           0,
        .sc_var_len =       IS_VAR_LEN|12,   //At least 12 bytes, mac addr
        .sc_helper =        "iwpriv <interface> mp_efuse_mac_wr <mac addr>\r\n \
                            value should be hex value\r\n \
                            want to write mac addr specific addr in efuse, but only store in specific buffer first\r\n \
                            example: sudo iwpriv wlan0 mp_efuse_mac_wr 11:22:33:44:55:66\r\n",
        .mfg_cmd_name =     "WEM",
        .mfg_cmd_len =      3,
        .ind_wait_ms =      LONG_WAIT_MS,
        .ind_exp =          "[mfg fw] WEM",  //"MAC address cmd:",//"Write slot:",   
    },
};

struct iwpriv_sub_cmd iwp_mp_sc_get_temp[] = 
{
    {
        .sc_name.cname =    NULL,
        .sc_len =           0,
        .sc_var_len =       0,
        .sc_helper =        "iwpriv <interface> mp_get_temp\r\n",
        .mfg_cmd_name =     "e\n",
        .mfg_cmd_len =      3,
        .ind_wait_ms =      3000, //LONG_WAIT_MS,
        .ind_exp =          "average temperature", //"[mfg fw]",
    },
};
        
struct iwpriv_sub_cmd iwp_mp_sc_load_cal_data[] = 
{
    {
        .sc_name.cname =    NULL,
        .sc_len =           0,
        .sc_var_len =       0,
        .sc_var_parser =    NULL,
        .sc_helper =        "iwpriv <interface> mp_load_caldata <file path>\r\n\
                            example: sudo iwpriv wlan0 mp_load_caldata /home/rf_para.conf\r\n",
        .mfg_cmd_name =     "D",
        .mfg_cmd_len =      1,
        .ind_wait_ms =      LONG_WAIT_MS*3,
        .ind_exp =          "mp_load_caldata success", //"[mfg fw]",
    },
};

struct iwpriv_sub_cmd iwp_nmp_sc_wmm_cfg[] = 
{
    {
        .sc_name.iname =    0,
        .sc_len =           4,
        .sc_var_len =       0,
        .sc_helper =        "iwpriv <interface> wmmcfg <value>\r\n \
                            value should be decimal value, 0 or 1\r\n",
        .sc_var_parser =    NULL,
    },
    {
        .sc_name.iname =    1,
        .sc_len =           4,
        .sc_var_len =       0,
        .sc_helper =        NULL,
        .sc_var_parser =    NULL,
    },
};

//iwp cmd group
//for normal fw
struct iwpriv_cmd iwp_nmp_cmds[] = {
    [BL_IOCTL_WMMCFG - SIOCIWFIRSTPRIV] =   {
        .name =         "wmmcfg",
        .sub_cmd =      iwp_nmp_sc_wmm_cfg,
        .sub_cmd_num =  sizeof(iwp_nmp_sc_wmm_cfg)/sizeof(struct iwpriv_sub_cmd),
    },
};

//iwp cmd group
//mp cmd for mfg fw
struct iwpriv_cmd iwp_mp_single[] = {
    [BL_IOCTL_VERSION - SIOCIWFIRSTPRIV] =    {
        .name =         "version",
        .sub_cmd =      iwp_mp_sc_ver,
        .sub_cmd_num =  sizeof(iwp_mp_sc_ver)/sizeof(struct iwpriv_sub_cmd),
    },
    [BL_IOCTL_MP_CFG - SIOCIWFIRSTPRIV] = {
        .name =         "mp_load_caldata",
        .sub_cmd =      iwp_mp_sc_load_cal_data,
        .sub_cmd_num =  sizeof(iwp_mp_sc_load_cal_data)/sizeof(struct iwpriv_sub_cmd),
    },
    [BL_IOCTL_MP_HELP - SIOCIWFIRSTPRIV] = {
        .name =         "help",
        .sub_cmd =      iwp_mp_sc_help,
        .sub_cmd_num =  sizeof(iwp_mp_sc_help)/sizeof(struct iwpriv_sub_cmd),
    },
};

struct iwpriv_cmd iwp_mp_mg[] = {
    [BL_IOCTL_MP_HELLO] =   {
        .name =         "mp_hello",
        .sub_cmd =      iwp_mp_sc_hello,
        .sub_cmd_num =  sizeof(iwp_mp_sc_hello)/sizeof(struct iwpriv_sub_cmd),
    },
    [BL_IOCTL_MP_UNICAST_RX] = {
        .name =         "mp_uc_rx",
        .sub_cmd =      iwp_mp_sc_uc_rx,
        .sub_cmd_num =  sizeof(iwp_mp_sc_uc_rx)/sizeof(struct iwpriv_sub_cmd),
    },
    [BL_IOCTL_MP_RX] = {
        .name =         "mp_rx",
        .sub_cmd =      iwp_mp_sc_rx,
        .sub_cmd_num =  sizeof(iwp_mp_sc_rx)/sizeof(struct iwpriv_sub_cmd),
    },
    // [BL_IOCTL_MP_PM] = {
    //     .name =         "mp_pm",
    //     .sub_cmd =      iwp_mp_sc_pm,
    //     .sub_cmd_num =  sizeof(iwp_mp_sc_pm)/sizeof(struct iwpriv_sub_cmd),
    // },
    // [BL_IOCTL_MP_RESET] = {
    //     .name =         "mp_reset",
    //     .sub_cmd =      iwp_mp_sc_reset,
    //     .sub_cmd_num =  sizeof(iwp_mp_sc_reset)/sizeof(struct iwpriv_sub_cmd),
    // },
    [BL_IOCTL_MP_GET_FW_VER] = {
        .name =         "mp_get_fw_ver",
        .sub_cmd =      iwp_mp_sc_get_fw_ver,
        .sub_cmd_num =  sizeof(iwp_mp_sc_get_fw_ver)/sizeof(struct iwpriv_sub_cmd),
    },
    [BL_IOCTL_MP_GET_BUILD_INFO] = {
        .name =         "mp_get_buildinf",
        .sub_cmd =      iwp_mp_sc_get_build_info,
        .sub_cmd_num =  sizeof(iwp_mp_sc_get_build_info)/sizeof(struct iwpriv_sub_cmd),
    },
    [BL_IOCTL_MP_GET_POWER] = {
        .name =         "mp_get_power",
        .sub_cmd =      iwp_mp_sc_get_pwr,
        .sub_cmd_num =  sizeof(iwp_mp_sc_get_pwr)/sizeof(struct iwpriv_sub_cmd),
    },
    [BL_IOCTL_MP_GET_FREQ] = {
        .name =         "mp_get_channel",
        .sub_cmd =      iwp_mp_sc_get_freq,
        .sub_cmd_num =  sizeof(iwp_mp_sc_get_freq)/sizeof(struct iwpriv_sub_cmd),
    },
    [BL_IOCTL_MP_GET_TX_STATUS] = {
        .name =         "mp_get_tx_state",
        .sub_cmd =      iwp_mp_sc_get_tx_state,
        .sub_cmd_num =  sizeof(iwp_mp_sc_get_tx_state)/sizeof(struct iwpriv_sub_cmd),
    },
    [BL_IOCTL_MP_GET_PKT_FREQ] = {
        .name =         "mp_get_pkt_freq",
        .sub_cmd =      iwp_mp_sc_get_pkt_freq,
        .sub_cmd_num =  sizeof(iwp_mp_sc_get_pkt_freq)/sizeof(struct iwpriv_sub_cmd),
    },
    [BL_IOCTL_MP_GET_XTAL_CAP] = {
        .name =         "mp_get_xtal_cap",
        .sub_cmd =      iwp_mp_sc_get_xtal_cap,
        .sub_cmd_num =  sizeof(iwp_mp_sc_get_xtal_cap)/sizeof(struct iwpriv_sub_cmd),
    },
    [BL_IOCTL_MP_GET_CWMODE] = {
        .name =         "mp_get_cw_mode",
        .sub_cmd =      iwp_mp_sc_get_cwmode,
        .sub_cmd_num =  sizeof(iwp_mp_sc_get_cwmode)/sizeof(struct iwpriv_sub_cmd),
    },
    [BL_IOCTL_MP_GET_TX_DUTY] = {
        .name =         "mp_get_tx_duty",
        .sub_cmd =      iwp_mp_sc_get_tx_duty,
        .sub_cmd_num =  sizeof(iwp_mp_sc_get_tx_duty)/sizeof(struct iwpriv_sub_cmd),
    },
    [BL_IOCTL_MP_EFUSE_RD] = {
        .name =         "mp_efuse_read",
        .sub_cmd =      iwp_mp_sc_efuse_rd,
        .sub_cmd_num =  sizeof(iwp_mp_sc_efuse_rd)/sizeof(struct iwpriv_sub_cmd),
    },
    [BL_IOCTL_MP_EFUSE_WR] = {
        .name =         "mp_efuse_write",
        .sub_cmd =      iwp_mp_sc_efuse_wr,
        .sub_cmd_num =  sizeof(iwp_mp_sc_efuse_wr)/sizeof(struct iwpriv_sub_cmd),
    },
    [BL_IOCTL_MP_EFUSE_CAP_RD] = {
        .name =         "mp_efuse_cap_rd",
        .sub_cmd =      iwp_mp_sc_efuse_cap_rd,
        .sub_cmd_num =  sizeof(iwp_mp_sc_efuse_cap_rd)/sizeof(struct iwpriv_sub_cmd),
    },
    [BL_IOCTL_MP_EFUSE_CAP_WR] = {
        .name =         "mp_efuse_cap_wr",
        .sub_cmd =      iwp_mp_sc_efuse_cap_wr,
        .sub_cmd_num =  sizeof(iwp_mp_sc_efuse_cap_wr)/sizeof(struct iwpriv_sub_cmd),
    },
    [BL_IOCTL_MP_EFUSE_PWR_RD] = {
        .name =         "mp_efuse_pwr_rd",
        .sub_cmd =      iwp_mp_sc_efuse_pwr_rd,
        .sub_cmd_num =  sizeof(iwp_mp_sc_efuse_pwr_rd)/sizeof(struct iwpriv_sub_cmd),
    },
    [BL_IOCTL_MP_EFUSE_PWR_WR] = {
        .name =         "mp_efuse_pwr_wr",
        .sub_cmd =      iwp_mp_sc_efuse_pwr_wr,
        .sub_cmd_num =  sizeof(iwp_mp_sc_efuse_pwr_wr)/sizeof(struct iwpriv_sub_cmd),
    },
    [BL_IOCTL_MP_EFUSE_MAC_ADR_RD] = {
        .name =         "mp_efuse_mac_rd",
        .sub_cmd =      iwp_mp_sc_efuse_mac_addr_rd,
        .sub_cmd_num =  sizeof(iwp_mp_sc_efuse_mac_addr_rd)/sizeof(struct iwpriv_sub_cmd),
    },
    [BL_IOCTL_MP_EFUSE_MAC_ADR_WR] = {
        .name =         "mp_efuse_mac_wr",
        .sub_cmd =      iwp_mp_sc_efuse_mac_addr_wr,
        .sub_cmd_num =  sizeof(iwp_mp_sc_efuse_mac_addr_wr)/sizeof(struct iwpriv_sub_cmd),
    },
    [BL_IOCTL_MP_GET_TEMPERATURE] = {
        .name =         "mp_get_temp",
        .sub_cmd =      iwp_mp_sc_get_temp,
        .sub_cmd_num =  sizeof(iwp_mp_sc_get_temp)/sizeof(struct iwpriv_sub_cmd),
    },
};

struct iwpriv_cmd iwp_mp_ms[] = {
    [BL_IOCTL_MP_UNICAST_TX] = {
        .name =         "mp_uc_tx",
        .sub_cmd =      iwp_mp_sc_uc_tx,
        .sub_cmd_num =  sizeof(iwp_mp_sc_uc_tx)/sizeof(struct iwpriv_sub_cmd),
    },    
    [BL_IOCTL_MP_11n_RATE] = {
        .name =         "mp_11n_rate",
        .sub_cmd =      iwp_mp_sc_11n_rate,
        .sub_cmd_num =  sizeof(iwp_mp_sc_11n_rate)/sizeof(struct iwpriv_sub_cmd),
    },
    [BL_IOCTL_MP_11g_RATE] = {
        .name =         "mp_11g_rate",
        .sub_cmd =      iwp_mp_sc_11g_rate,
        .sub_cmd_num =  sizeof(iwp_mp_sc_11g_rate)/sizeof(struct iwpriv_sub_cmd),
    },
    [BL_IOCTL_MP_11b_RATE] = {
        .name =         "mp_11b_rate",
        .sub_cmd =      iwp_mp_sc_11b_rate,
        .sub_cmd_num =  sizeof(iwp_mp_sc_11b_rate)/sizeof(struct iwpriv_sub_cmd),
    },
    [BL_IOCTL_MP_TX] = {
        .name =         "mp_tx",
        .sub_cmd =      iwp_mp_sc_tx,
        .sub_cmd_num =  sizeof(iwp_mp_sc_tx)/sizeof(struct iwpriv_sub_cmd),
    },
    [BL_IOCTL_MP_SET_PKT_FREQ] = {
        .name =         "mp_set_pkt_freq",
        .sub_cmd =      iwp_mp_sc_set_pkt_freq,
        .sub_cmd_num =  sizeof(iwp_mp_sc_set_pkt_freq)/sizeof(struct iwpriv_sub_cmd),
    },
    [BL_IOCTL_MP_SET_PKT_LEN] = {
        .name =         "mp_set_pkt_len",
        .sub_cmd =      iwp_mp_sc_set_pkt_len,
        .sub_cmd_num =  sizeof(iwp_mp_sc_set_pkt_len)/sizeof(struct iwpriv_sub_cmd),
    },
    //Get more msg on console when the followings are in iwp_mp_mg group. only 'GET' iwpriv cmd id can forward msg to user console.
    //If not want the msg on console, we can put the followings to iwp_mp_ms
    [BL_IOCTL_MP_SET_CHANNEL] = {
        .name =         "mp_set_channel",
        .sub_cmd =      iwp_mp_sc_set_ch,
        .sub_cmd_num =  sizeof(iwp_mp_sc_set_ch)/sizeof(struct iwpriv_sub_cmd),
    },
    [BL_IOCTL_MP_SET_CWMODE] = {
        .name =         "mp_set_cw_mode",
        .sub_cmd =      iwp_mp_sc_set_cw,
        .sub_cmd_num =  sizeof(iwp_mp_sc_set_cw)/sizeof(struct iwpriv_sub_cmd),
    },
    [BL_IOCTL_MP_SET_PWR] = {
        .name =         "mp_set_power",
        .sub_cmd =      iwp_mp_sc_set_pwr,
        .sub_cmd_num =  sizeof(iwp_mp_sc_set_pwr)/sizeof(struct iwpriv_sub_cmd),
    },
    [BL_IOCTL_MP_SET_TXDUTY] = {
        .name =         "mp_set_tx_duty",
        .sub_cmd =      iwp_mp_sc_set_tx_duty,
        .sub_cmd_num =  sizeof(iwp_mp_sc_set_tx_duty)/sizeof(struct iwpriv_sub_cmd),
    },
    [BL_IOCTL_MP_SET_PWR_OFT_EN] = {
        .name =         "mp_en_pwr_oft",
        .sub_cmd =      iwp_mp_sc_set_pwr_offset_en,
        .sub_cmd_num =  sizeof(iwp_mp_sc_set_pwr_offset_en)/sizeof(struct iwpriv_sub_cmd),
    },
    [BL_IOCTL_MP_SET_XTAL_CAP] = {
        .name =         "mp_set_xtal_cap",
        .sub_cmd =      iwp_mp_sc_set_xtal_cap,
        .sub_cmd_num =  sizeof(iwp_mp_sc_set_xtal_cap)/sizeof(struct iwpriv_sub_cmd),
    },
    [BL_IOCTL_MP_SET_PRIV_PARAM] = {
        .name =         "mp_set_dbg_para",
        .sub_cmd =      iwp_mp_sc_set_misc_param,
        .sub_cmd_num =  sizeof(iwp_mp_sc_set_misc_param)/sizeof(struct iwpriv_sub_cmd),
    },
    [BL_IOCTL_MP_WR_MEM] = {
        .name =         "mp_wr_mem",
        .sub_cmd =      iwp_mp_sc_wr_mem,
        .sub_cmd_num =  sizeof(iwp_mp_sc_wr_mem)/sizeof(struct iwpriv_sub_cmd),
    },
    [BL_IOCTL_MP_RD_MEM] = {
        .name =         "mp_rd_mem",
        .sub_cmd =      iwp_mp_sc_rd_mem,
        .sub_cmd_num =  sizeof(iwp_mp_sc_rd_mem)/sizeof(struct iwpriv_sub_cmd),
    },
};

static int parse_help_args(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len) {
    uint32_t i = 0;
    uint32_t j = 0;
    uint32_t m = 0;
    struct iwpriv_cmd *iwp_cmd = NULL;
    char *not_found_str = "Not found\r\n";
    char *more_details_str = "\nPlease use 'iwpriv <interface> help cmd_name' to get more details about specific cmd\r\n";
    int *iwp_cnt = NULL;
    struct iwpriv_cmd **iwp_cs = NULL;
    int total_cnt = 0;

    //struct iwpriv_cmd *iwp_nmp_cs[] = {
    //    iwp_nmp_cmds,
    //};

    //int iwp_nmp_cnt[] = {
    //    sizeof(iwp_nmp_cmds)/sizeof(struct iwpriv_cmd),
    //};
    
    struct iwpriv_cmd *iwp_mp_cs[] = {
        iwp_mp_single,
        iwp_mp_ms,
        iwp_mp_mg
    };

    int iwp_mp_cnt[] = {
        sizeof(iwp_mp_single)/sizeof(struct iwpriv_cmd),
        sizeof(iwp_mp_ms)/sizeof(struct iwpriv_cmd),
        sizeof(iwp_mp_mg)/sizeof(struct iwpriv_cmd)
    };

    if (!bl_mod_params.mp_mode) {
        //iwp_cs = iwp_nmp_cs;
        //iwp_cnt = iwp_nmp_cnt;
        //total_cnt = 1;
        total_cnt = 0;
    } else {
        iwp_cs = iwp_mp_cs;
        iwp_cnt = iwp_mp_cnt;
        total_cnt = 3;
    }

    *var_len = 0;

    for (m=0; m<total_cnt; m++) {
        for (i=0; i<iwp_cnt[m]; i++) {
            iwp_cmd = iwp_cs[m] + i;
            if (iwp_cmd->name == NULL) {
                // printk("iwp cmds null i:%d\n", i);
                continue;
            } else {
                // printk("iwp cmds i:%d\n", i);
            }

            if (strlen(in_buf) > 0) {
                if (strlen(in_buf) >= strlen(iwp_cmd->name) && strncmp(in_buf, iwp_cmd->name, strlen(iwp_cmd->name)) == 0) {
                // if (strncmp(in_buf, iwp_cmd->name, min(strlen(in_buf), strlen(iwp_cmd->name))) == 0) {
                    for (j=0; j<iwp_cmd->sub_cmd_num; j++) {
                        if (iwp_cmd->sub_cmd[j].sc_helper != NULL) {
                            memcpy(out_buf + *var_len, iwp_cmd->sub_cmd[j].sc_helper, 
                                   strlen(iwp_cmd->sub_cmd[j].sc_helper));
                            *var_len += strlen(iwp_cmd->sub_cmd[j].sc_helper);
                            memcpy(out_buf + *var_len, "\r\n", 2);
                            *var_len += 2;
                        }
                    }
                    break;
                }
            } else {
                memcpy(out_buf + *var_len, iwp_cmd->name, strlen(iwp_cmd->name));
                *var_len += strlen(iwp_cmd->name);
                memcpy(out_buf + *var_len, "\r\n", 2);
                *var_len += 2;

                // for (j=0; j<iwp_cmd->sub_cmd_num; j++) {
                //     if (iwp_cmd->sub_cmd[j].sc_helper != NULL) {
                //         memcpy(out_buf + *var_len, iwp_cmd->sub_cmd[j].sc_helper, strlen(iwp_cmd->sub_cmd[j].sc_helper));
                //         *var_len += strlen(iwp_cmd->sub_cmd[j].sc_helper);
                //         memcpy(out_buf + *var_len, "\r\n", 2);
                //         *var_len += 2;
                //     }
                // }
            }
        }
    }

    if (strlen(in_buf) == 0) {
        memcpy(out_buf + *var_len, more_details_str, strlen(more_details_str));
        *var_len += strlen(more_details_str);
        *var_len += 2;
        // printk("%d, %s\n", *var_len, more_details_str);
    }
    
    if (*var_len == 0) {
        memcpy(out_buf + *var_len, not_found_str, strlen(not_found_str));
        *var_len += strlen(not_found_str);
        // printk("%d, %s\n", *var_len, not_found_str);
    }

    // printk("var_len: %d\n", *var_len);

    return 0;
}

static int parse_str_2_one_bool(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len) {
    uint32_t one_bool = 0;

    if (sscanf(in_buf, "%d", &one_bool) < 1)
        return -EINVAL;

    one_bool = one_bool>0?1:0;

    *var_len = sprintf(out_buf, "%d", one_bool);
    return 0;
}

static int parse_str_2_one_uint(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len) {
    uint32_t one_uint = 0;

    if (sscanf(in_buf, "%d", &one_uint) < 1)
        return -EINVAL;

    *var_len = sprintf(out_buf, "%d", one_uint);
    return 0;
}

static int parse_one_bool(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len) {
    uint32_t one_uint = *(uint32_t *)in_buf;

    one_uint = one_uint>0?1:0;
    *var_len = sprintf(out_buf, "%d", one_uint);
    return 0;
}

static int parse_one_uint(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len) {
    uint32_t one_uint = *(uint32_t *)in_buf;

    *var_len = sprintf(out_buf, "%d", one_uint);
    return 0;
}

static int parse_pkt_len(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len) {
    uint32_t one_uint = *(uint32_t *)in_buf;

    if (one_uint == 0 || one_uint == 1000 || one_uint > 4096) {
        printk("Invalid param value, pkt_len should be in [1, 4096] and not be 1000\r\n");
        return -EINVAL;
    }
    
    *var_len = sprintf(out_buf, "%d", one_uint);
    return 0;
}

static int parse_unicast_tx_param(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len) {
    uint32_t pkt_number = 0;

    pkt_number = *(uint32_t *)in_buf;
    if (pkt_number < 1) {
        printk("Invalid param value, pkt_number should > 0\r\n");
        return -EINVAL;
    }
    *var_len = sprintf(out_buf, "%d%d", 1, pkt_number);

    return 0;
}

static int parse_11n_rate(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len) {
    uint32_t guard_interval = *(uint32_t *)(in_buf + 0); 
    uint32_t mcs_index = *(uint32_t *)(in_buf + 4);
    //modulation type: 2 for HT-MF, 3 for HT-GF
    uint32_t modulation_type = 2;
    uint32_t bandwidth = *(uint32_t *)(in_buf + 8);

    if (guard_interval != 0 && guard_interval != 1) {
        printk("Invalid gurad interval %d, valid is 0 for short guard interval, 1 for long guard interval\n", 
                guard_interval);
        return -EINVAL;
    }

    if (modulation_type != 2 && modulation_type != 3) {
        printk("Invalid modulation type %d, valid is 2 for HT-MF, 3 for HT-GF.\n", modulation_type);
        return -EINVAL;
    }

    if (bandwidth != 2) { // && bandwidth != 4
        printk("Invalid bandwidth %d, valid is 2 for 20MHz, 4 for 40MHz, but BL602 only support 20MHz.\n", bandwidth);
        return -EINVAL;
    }

    if (mcs_index > 7) {
        printk("Invalid mcs_index %d, valid is in range 0-7.\n", mcs_index);
        return -EINVAL;
    }

    out_buf[0] = 'm';
    out_buf[1] = (guard_interval==0) ? 's' : 'l';
    out_buf[2] = (modulation_type==3) ? 'g' : 'm';
    out_buf[3] = (bandwidth==2) ? '2' : '4';
    sprintf(out_buf+4, "%d", mcs_index);
    *var_len = 5;

    return 0;
}

static int parse_11g_rate(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len) {
    uint32_t rate = *(uint32_t *)in_buf;

    if (rate > 7) {
        printk("Invalid rate %d, 2.4G 11g rate idx = 0 - 7, 0:6Mbps 1:9Mbps \
                2:12Mbps 3:18Mbps 4:24Mbps 5:36Mbps 6:48Mbps 7:54Mbps\n", rate);
        return -EINVAL;
    }

    *var_len = sprintf(out_buf, "%d", rate);
    return 0;
}

static int parse_11b_rate(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len) {
    uint32_t rate = *(uint32_t *)in_buf;

    if (rate > 3) {
        printk("Invalid rate %d, 2.4G 11b support rate idx = 0 - 3, 0:1Mbps, \
                1:2Mbps, 2:5.5Mbps, 3:11Mbps\n", rate);
        return -EINVAL;
    }

    *var_len = sprintf(out_buf, "%d", rate);
    return 0;
}

static int parse_set_pkt_freq(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len) {
    uint32_t pkt_freq = *(uint32_t *)in_buf;

    if (pkt_freq > 1000) {
        printk("Invalid channel %d, valid pkt_freq is 1-1000\n", pkt_freq);
        return -EINVAL;
    }

    *var_len = sprintf(out_buf, "%d", pkt_freq);
    return 0;
}

static int parse_set_channel(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len) {
    uint32_t channel_idx = *(uint32_t *)in_buf;

    if (channel_idx > 13 || channel_idx == 0) {
        printk("Invalid channel %d, valid channel is 1-13, map to frequency 2412-2484.\n", channel_idx);
        return -EINVAL;
    }

    *var_len = sprintf(out_buf, "%d", channel_idx);
    return 0;
}

static int parse_set_power(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len) {
    int32_t power = *(int32_t *)in_buf;

    if (power == -1) {
        BL_DBG("use fw's default power\n");
    } else if (power > 23) {
        printk("Valid power is 1-23.\n");
        return -EINVAL;
    }

    *var_len = sprintf(out_buf, "%d", power);
    return 0;
}

static int parse_set_xtal_cap(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len) {
    uint32_t one_uint = *(uint32_t *)in_buf;

    if (one_uint == 0) {
        printk("Invalid xtal cap %d, valid is > 0\n", one_uint);
        return -EINVAL;
    }

    *var_len = sprintf(out_buf, "%d", one_uint);
    return 0;
}

static int parse_set_misc_param(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len) {
    uint32_t tcal_debug = *(uint32_t *)in_buf;
    uint32_t tcal_val_fix = *(uint32_t *)(in_buf+4);
    uint32_t tcal_val_fix_value = 0;

    tcal_val_fix = (tcal_val_fix>0)?1:0;
    if (tcal_val_fix > 0) {
        tcal_val_fix_value = *(int32_t *)(in_buf+8);
    }
    
    *var_len = sprintf(out_buf, "t%d,%d,%d", tcal_debug, tcal_val_fix, tcal_val_fix_value);
    return 0;
}

static int parse_wr_mem(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len) {
    //<addr in hex, 4 bytes align> <value in hex, 32bit>
    //<addr in hex, 4 bytes align> <value in hex> <bit len, int> <bit pos, int>
    uint32_t addr = *(uint32_t *)in_buf;
    uint32_t value = *(uint32_t *)(in_buf+4);
    uint32_t bit_len = 0;
    uint32_t bit_pos = 0;

    if (in_len >= 16) {
        bit_len = *(uint32_t *)(in_buf+8);
        bit_pos = *(uint32_t *)(in_buf+12);
        if (bit_pos > 31 || bit_len + bit_pos > 32) {
            printk("invalid bit_len or invalid bit_pos, %d, %d\n", bit_pos, bit_len);
            return -EINVAL;
        }
    } else if (in_len == 12) {
        printk("invalid param number %d\r\n", in_len/4);
        return -EINVAL;
    }
    
    if ((addr & 0x03) > 0) {
        printk("not 4 bytes align, 0x%x\n", addr);
        return -EINVAL;
    }

    if (in_len >= 16)
        *var_len = sprintf(out_buf, "0x%x 0x%x %d %d", addr, value, bit_len, bit_pos);
    else
        *var_len = sprintf(out_buf, "0x%x 0x%x", addr, value);
    
    return 0;
}

static int parse_rd_mem(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len) {
    //<addr in hex, 4 bytes align>
    //<addr in hex, 4 bytes align> <number of uint32_t to read, 4 at max>
    //<addr in hex, 4 bytes align> <bit len, int> <bit pos, int>
    uint32_t addr = *(uint32_t *)in_buf;
    uint32_t bit_len = 0;
    uint32_t bit_pos = 0;

    if (in_len >= 12) {
        bit_len = *(uint32_t *)(in_buf+4);
        bit_pos = *(uint32_t *)(in_buf+8);
        if (bit_pos > 31 || bit_len + bit_pos > 32) {
            printk("invalid bit_len or invalid bit_pos, %d, %d\n", bit_pos, bit_len);
            return -EINVAL;
        }
    } else if (in_len >= 8) {
        bit_len = *(uint32_t *)(in_buf+4);
        if (bit_len > 4) {
            printk("invalid number of uint32_t to read, %d > max value 4\n", bit_len);
        }
    }
    if ((addr & 0x03) > 0) {
        printk("not 4 bytes align, 0x%x\n", addr);
        return -EINVAL;
    }

    if (in_len >= 12)
        *var_len = sprintf(out_buf, "0x%x %d %d", addr, bit_len, bit_pos);
    else if (in_len >= 8)
        *var_len = sprintf(out_buf, "0x%x %d", addr, bit_len);
    else
        *var_len = sprintf(out_buf, "0x%x", addr);
    
    return 0;
}

static int parse_sleep_dtim(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len) {
    uint32_t dtim_count = 0;
    uint32_t dtim_exit_count = 0;
    
    if (sscanf(in_buf, "%d,%d", &dtim_count, &dtim_exit_count) < 2)
        return -EINVAL;

    if (dtim_count > 9 || dtim_exit_count > 9) {
        printk("sleep_dtim only support dtim_count in [0-9] and dtim_exit_count in [0-9].\n");
        return -EINVAL;
    }

    *var_len = sprintf(out_buf, "%d%d", dtim_count, dtim_exit_count);
    return 0;
}

static int parse_set_duty(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t *var_len) {
    uint32_t duty = *(uint32_t *)in_buf;

    if (duty > 100) {
        printk("valid duty in [0-100]\n");
        return -EINVAL;
    }

    *var_len = sprintf(out_buf, "%d", duty);
    return 0;
}

static struct iwpriv_sub_cmd *get_sub_cmd(struct iwpriv_sub_cmd *iwp_scs, uint32_t sub_cmd_num, 
                                   uint16_t var_type, char *in_buf, uint32_t in_len) 
{
    int i = 0;
    struct iwpriv_sub_cmd *iwp_sc = NULL;

    for (i=0; i < sub_cmd_num; i++) {
        if (var_type == IW_PRIV_TYPE_CHAR) {
            if (in_len >= iwp_scs[i].sc_len &&
                (iwp_scs[i].sc_len == 0 ||
                strncmp(in_buf, iwp_scs[i].sc_name.cname, iwp_scs[i].sc_len) == 0)) 
            {
                iwp_sc = iwp_scs + i;
                break;
            }
        } else if (var_type == IW_PRIV_TYPE_INT) {
            if (in_len >= iwp_scs[i].sc_len &&
                (iwp_scs[i].sc_len == 0 ||
                *(int32_t *)in_buf == iwp_scs[i].sc_name.iname))
            {
                iwp_sc = iwp_scs + i;
                break;
            }
        }
    }

    return iwp_sc;
}

static uint8_t * find_ind_exp(uint8_t *out_buf, uint16_t out_len, char *ind_exp) {
    uint8_t *cursur_1st = out_buf;
    uint8_t *cursur_2nd = NULL;
    uint8_t *found = NULL;
    out_buf[out_len] = '\0';

    ////debug
    //dump_buf(out_buf, out_len);

    while (cursur_1st < out_buf+out_len && *cursur_1st != '\0') {
        uint8_t tmp = 0;

        cursur_2nd = cursur_1st + 1;
        while (cursur_2nd < out_buf+out_len &&
               *cursur_2nd != '\0' && *cursur_2nd != 0x0d && *cursur_2nd != 0x0a) 
        {
            cursur_2nd++;
        }

        tmp = *cursur_2nd;
        *cursur_2nd = '\0';
        if (cursur_2nd-cursur_1st >= strlen(ind_exp) && 
            (found=strstr(cursur_1st, ind_exp)) != NULL) 
        {
            printk("%s, found expected indication:%s\n", __func__, ind_exp);
            *cursur_2nd = tmp;
            return found;
        }
        *cursur_2nd = tmp;
        
        cursur_1st = cursur_2nd;
    }

    return NULL;
}

static uint16_t merge_ind_2_out(struct bl_hw *bl_hw, uint8_t *out_buf) {
    uint16_t out_len = 0;
    uint16_t ind_len = 0;
    uint8_t *purge_ptr = NULL;
    
    purge_ptr = bl_hw->iwp_var.iwpriv_ind;
    ind_len = le16_to_cpu(*(__le16 *)purge_ptr);
    while (ind_len > 0 && purge_ptr < bl_hw->iwp_var.iwpriv_ind + bl_hw->iwp_var.iwpriv_ind_len && 
           ind_len < bl_hw->iwp_var.iwpriv_ind_len - (purge_ptr-bl_hw->iwp_var.iwpriv_ind) && 
           out_len + ind_len < IWPRIV_OUT_BUF_LEN-1)
    {
        // dump_buf_char(purge_ptr + 2, ind_len);
    
        memcpy(out_buf + out_len, purge_ptr + 2, ind_len);
        out_len += ind_len;
        purge_ptr = purge_ptr + ind_len + 2;
        ind_len = le16_to_cpu(*(__le16 *)purge_ptr);
    }

    return out_len;
}

static int mp_efuse_write(struct bl_hw *bl_hw, uint16_t sc_index, char *in_buf, 
                   union iwreq_data *wrqu, char *extra)
{
    uint8_t *out_buf = NULL;
    uint16_t out_len = 0;
    int ret = 0;
    uint8_t mp_cmd[100];
    bool exp_found = true;

    char *mfg_load = "LEA";
    char *mfg_program = "SEA\n";
    char *mfg_load_cap = "LEX\n";
    char *mfg_program_cap = "SEX\n";
    char *mfg_load_pwr = "LEP\n";
    char *mfg_program_pwr = "SEP\n";
    char *mfg_load_mac = "LEM\n";
    char *mfg_program_mac = "SEM\n";
    uint32_t mfg_wr_addr = 0;
    uint32_t mfg_wr_value = 0;
    uint32_t mfg_rd_addr = 0;
    uint32_t mfg_rd_value = 0;
    uint32_t mfg_wr_cap_code = 0;
    uint32_t mfg_rd_cap_code = 0;
    int32_t mfg_wr_pwr_oft[14];
    // uint8_t *mfg_rd_pwr_offset = NULL;
    uint8_t mfg_wr_mac_addr[50];
    //uint8_t *mfg_rd_mac_addr = NULL;

    do {
        out_buf = kzalloc(IWPRIV_OUT_BUF_LEN, GFP_KERNEL);
        if (out_buf == NULL) {
            printk("%s, fail to alloc out_buf\n", __func__);
            ret = -ENOMEM;
            break;
        }
        memset(out_buf, 0, IWPRIV_OUT_BUF_LEN);

        //2nd command, load back to check
        memset(bl_hw->iwp_var.iwpriv_ind, 0, IWPRIV_IND_LEN_MAX);
        bl_hw->iwp_var.iwpriv_ind_len = 0;
        exp_found = false;

        if (sc_index == BL_IOCTL_MP_EFUSE_WR) {
            if(sscanf(in_buf, "%x=%x", &mfg_wr_addr, &mfg_wr_value) < 2) {
                printk("Need (right) efuse write addr in hex and value in hex.\n");
                ret = -EFAULT;
                break;
            }
            sprintf(mp_cmd, "%s0x%x\n", mfg_load, mfg_wr_addr);
        } else if (sc_index == BL_IOCTL_MP_EFUSE_CAP_WR) {
            if(sscanf(in_buf, "%d", &mfg_wr_cap_code) < 1) {
                printk("Need (right) efuse write cap code in decimal.\n");
                ret = -EFAULT;
                break;
            }
            sprintf(mp_cmd, "%s\n", mfg_load_cap);
        } else if (sc_index == BL_IOCTL_MP_EFUSE_PWR_WR) {
            if (sscanf(in_buf, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", 
                       mfg_wr_pwr_oft, mfg_wr_pwr_oft+1, mfg_wr_pwr_oft+2, 
                       mfg_wr_pwr_oft+3, mfg_wr_pwr_oft+4, mfg_wr_pwr_oft+5, 
                       mfg_wr_pwr_oft+6, mfg_wr_pwr_oft+7, mfg_wr_pwr_oft+8, 
                       mfg_wr_pwr_oft+9, mfg_wr_pwr_oft+10, mfg_wr_pwr_oft+11, 
                       mfg_wr_pwr_oft+12, mfg_wr_pwr_oft+13) != 14) 
            {
                printk("Need (right) efuse write power offset value in decimal \
                        like 1,2,3,3,3,2,1,0,1,2,3,4,1,3.\n");
                ret = -EFAULT;
                break;
            }
            sprintf(mp_cmd, "%s\n", mfg_load_pwr);
        } else if (sc_index == BL_IOCTL_MP_EFUSE_MAC_ADR_WR) {
            uint32_t mac_addr[6];
            if (sscanf(in_buf, "%x:%x:%x:%x:%x:%x", 
                       (uint32_t *)mac_addr, (uint32_t *)(mac_addr+1), (uint32_t *)(mac_addr+2), 
                       (uint32_t *)(mac_addr+3), (uint32_t *)(mac_addr+4), (uint32_t *)(mac_addr+5)) != 6) 
            {
                printk("Need (right) efuse write mac addr in hex like 1A:2B:3C:4D:5F:60.\n");
                ret = -EFAULT;
                break;
            }
            memset(mfg_wr_mac_addr, 0, sizeof(mfg_wr_mac_addr));
            sprintf(mfg_wr_mac_addr, "%02x:%02x:%02x:%02x:%02x:%02x", 
                    *mac_addr, *(mac_addr+1), *(mac_addr+2), *(mac_addr+3), *(mac_addr+4), *(mac_addr+5));
            sprintf(mp_cmd, "%s\n", mfg_load_mac);
        } else {
            printk("Unknow efuse sub cmd num:%d\n", sc_index);
            ret = -EFAULT;
            break;
        }
        
        printk("mp_efuse write, mp_cmd:%s\n", mp_cmd);
        // dump_buf(mp_cmd, strlen(mp_cmd));

        ret = bl_send_mp_test_msg(bl_hw, mp_cmd, out_buf + out_len, false);
        if (ret) {
            printk("%s, bl_send_mp_test_msg:%s, error=%d\n", __func__, (char *)mp_cmd, ret);
            break;
        }

        msleep_interruptible(SHORT_WAIT_MS);

        out_len += merge_ind_2_out(bl_hw, out_buf + out_len);

        ////debug
        //dump_buf_char(out_buf, out_len);
        printk("%s, verify, out_buf:%s\n", __func__, out_buf);
        
        if (sc_index == BL_IOCTL_MP_EFUSE_WR) {
            uint8_t *found_ptr = find_ind_exp(out_buf, out_len, "Rd ef ");
            if (found_ptr) {
                if (sscanf(found_ptr, "Rd ef %x=%x", &mfg_rd_addr, &mfg_rd_value) == 2 &&
                    mfg_rd_addr == mfg_wr_addr && mfg_rd_value == mfg_wr_value)
                {
                    printk("verify step, found rd ef, mfg_rd_addr, mfg_rd_value:0x%x=0x%x\n", 
                            mfg_rd_addr, mfg_rd_value);
                    exp_found = true;
                } else {
                    printk("verify step, found rd ef, but not same as wr addr and value, \
                            mfg_rd_addr, mfg_rd_value:0x%x=0x%x\n", 
                            mfg_rd_addr, mfg_rd_value);
                }
            } else {
                printk("verify step, not found rd ef\n");
            }
        } else if (sc_index == BL_IOCTL_MP_EFUSE_CAP_WR) {
            uint8_t *found_ptr = find_ind_exp(out_buf, out_len, "Cap code2:");
            if (found_ptr) {
                if (sscanf(found_ptr, "Cap code2:%d", &mfg_rd_cap_code) == 1 &&
                    mfg_rd_cap_code == mfg_wr_cap_code)
                {
                    printk("verify step, found Cap code read and write value:%d, %d\n", 
                            mfg_rd_cap_code, mfg_wr_cap_code);
                    exp_found = true;
                } else {
                    printk("verify step, found Cap code, but not same as wr cap code, read and write value:%d, %d\n", 
                            mfg_rd_cap_code, mfg_wr_cap_code);
                }
            } else {
                printk("verify step, not found Cap code2:\n");
            }
        } else if (sc_index == BL_IOCTL_MP_EFUSE_PWR_WR) {
            uint8_t *found_ptr = find_ind_exp(out_buf, out_len, "Power offset:");
            if (found_ptr != NULL) 
            {
                int32_t pwr_oft[14];
                if (sscanf(found_ptr + strlen("Power offset:"), 
                           "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", 
                           pwr_oft, pwr_oft+1, pwr_oft+2, pwr_oft+3, pwr_oft+4, pwr_oft+5, 
                           pwr_oft+6, pwr_oft+7, pwr_oft+8, pwr_oft+9, pwr_oft+10, pwr_oft+11, 
                           pwr_oft+12, pwr_oft+13) == 14 &&
                    pwr_oft[0] == mfg_wr_pwr_oft[0] && 
                    pwr_oft[6] == mfg_wr_pwr_oft[6] &&
                    pwr_oft[12] == mfg_wr_pwr_oft[12])
                {
                    printk("verify step, found power offset read and write value\n");
                    exp_found = true;
                } else {
                    printk("verify step, found power offset, but not match\n");
                }
            } else {
                printk("verify step, not found power offset\n");
            }
        } else if (sc_index == BL_IOCTL_MP_EFUSE_MAC_ADR_WR) {
            uint8_t *found_ptr = find_ind_exp(out_buf, out_len, "MAC:");
            if (found_ptr != NULL)
            {
                if (strncmp(found_ptr + strlen("MAC:"), mfg_wr_mac_addr, strlen(mfg_wr_mac_addr)) == 0) 
                {
                    printk("verify step, found mac addr and match\n");
                    exp_found = true;
                } else {
                    printk("verify step, found mac addr but not match\n");
                }
            } else {
                printk("verify step, not found mac addr\n");
            }
        }
        
        if (exp_found == false) {
            printk("Fail when load back the wrote value and check, out_buf:%s\n", out_buf);
            ret = -EFAULT;
            break;
        }

        //3rd command, program efuse
        memset(bl_hw->iwp_var.iwpriv_ind, 0, IWPRIV_IND_LEN_MAX);
        bl_hw->iwp_var.iwpriv_ind_len = 0;
        exp_found = false;

        if (sc_index == BL_IOCTL_MP_EFUSE_WR) {
            sprintf(mp_cmd, "%s\n", mfg_program);
        } else if (sc_index == BL_IOCTL_MP_EFUSE_CAP_WR) {
            sprintf(mp_cmd, "%s\n", mfg_program_cap);
        } else if (sc_index == BL_IOCTL_MP_EFUSE_PWR_WR) {
            sprintf(mp_cmd, "%s\n", mfg_program_pwr);
        } else if (sc_index == BL_IOCTL_MP_EFUSE_MAC_ADR_WR) {
            sprintf(mp_cmd, "%s\n", mfg_program_mac);
        }

        printk("mp_efuse program, mp_cmd:%s\n", mp_cmd);
        //dump_buf(mp_cmd, strlen(mp_cmd));

        ret = bl_send_mp_test_msg(bl_hw, mp_cmd, out_buf + out_len, false);
        if (ret) {
            printk("bl_send_mp_test_msg:%s, error=%d\n", (char *)mp_cmd, ret);
            break;
        }

        msleep_interruptible(SHORT_WAIT_MS);

        out_len += merge_ind_2_out(bl_hw, out_buf + out_len);

        ////debug
        //dump_buf_char(out_buf, *out_len);
        printk("%s, program, out_buf:%s\n", __func__, out_buf);
        
        if (sc_index == BL_IOCTL_MP_EFUSE_WR) {
            uint8_t *found_ptr = find_ind_exp(out_buf, out_len, "Save efuse OK");
            if (found_ptr) {
                printk("program step, found Save efuse OK\n");
                exp_found = true;
            } else {
                printk("program step, not found Save efuse OK\n");
            }
        } else if (sc_index == BL_IOCTL_MP_EFUSE_CAP_WR) {
            uint8_t *found_ptr = find_ind_exp(out_buf, out_len, "Save cap code2 OK");
            if (found_ptr) {
                printk("program step, found Save cap code2 OK\n");
                exp_found = true;
            } else {
                printk("program step, not found Save cap code2 OK\n");
            }
        } else if (sc_index == BL_IOCTL_MP_EFUSE_PWR_WR) {
            uint8_t *found_ptr = find_ind_exp(out_buf, out_len, "Save power offset OK");
            if (found_ptr) {
                printk("program step, found Save power offset OK\n");
                exp_found = true;
            } else {
                printk("program step, not found Save power offset OK\n");
            }
        } else if (sc_index == BL_IOCTL_MP_EFUSE_MAC_ADR_WR) {
            uint8_t *found_ptr = find_ind_exp(out_buf, out_len, "Save MAC address OK");
            if (found_ptr) {
                printk("program step, found Save MAC address OK\n");
                exp_found = true;
            } else {
                printk("program step, not found Save MAC address OK\n");
            }
        }

        if (exp_found == false) {
            printk("out_buf:%s\n", out_buf);
            ret = -EFAULT;
            break;
        }
    } while (0);

    if (ret == 0) {
        if(extra) {
            memcpy(extra + wrqu->data.length, out_buf, out_len);
            wrqu->data.length += out_len;
        } else {
            if (copy_to_user(wrqu->data.pointer + wrqu->data.length, out_buf, out_len)) {
                ret = -EFAULT;
            } else {
                wrqu->data.length += out_len;
            }
        }
    }

    if (out_buf != NULL)
        kfree(out_buf);

    return ret;
}

static int mp_pre_rate_hdl(struct net_device *dev, struct iw_request_info *info, 
                    union iwreq_data *wrqu, char *extra, uint16_t sc_index, uint8_t *in_buf) 
{
    int ret = 0;
    uint32_t rate_11b_pkt_lens[] = {101, 202, 556, 1111};
    uint32_t rate_11b_pkt_freqs[] = {500, 500, 500, 500};
    uint32_t rate_11g_pkt_lens[] = {360, 540, 720, 1080, 1440, 2160, 2880, 3240};
    uint32_t rate_11g_pkt_freqs[] = {1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000};
    uint32_t rate_11n_pkt_lens[] = {384, 767, 1151, 1534, 2301, 3068, 3452, 3835};
    uint32_t rate_11n_pkt_freqs[] = {1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000};
    uint32_t rate_idx = 0;
    uint8_t extra_in_buf[20];
    uint16_t extra_in_len = 0;
    uint16_t extra_sc_index = BL_IOCTL_MP_SET_PKT_FREQ; //BL_IOCTL_MP_SET_PKT_LEN;
    uint32_t *pkt_lens = NULL;
    uint32_t *pkt_freqs = NULL;
    
    if (sc_index == BL_IOCTL_MP_11b_RATE) {
        uint32_t preamble = *(uint32_t *)in_buf;
        if (preamble > 1) {
            printk("Invalid preamble %d, 2.4G 11b support preamble 0 or 1\n", preamble);
            return -EINVAL;
        }
        rate_idx = *(uint32_t *)(in_buf+4);
        if (rate_idx > 3) {
            printk("Invalid rate %d, 2.4G 11b support rate idx = 0 - 3, 0:1Mbps, \
                    1:2Mbps, 2:5.5Mbps, 3:11Mbps\n", rate_idx);
            return -EINVAL;
        }
        pkt_lens = rate_11b_pkt_lens;
        pkt_freqs = rate_11b_pkt_freqs;
    } else if (sc_index == BL_IOCTL_MP_11g_RATE) {
        uint32_t preamble = *(uint32_t *)in_buf;
        if (preamble > 1) {
            printk("Invalid preamble %d, 2.4G 11g support preamble 0 or 1\n", preamble);
            return -EINVAL;
        }
        rate_idx = *(uint32_t *)(in_buf+4);
        if (rate_idx > 7) {
            printk("Invalid rate %d, 2.4G 11g rate idx = 0 - 7, 0:6Mbps 1:9Mbps \
                    2:12Mbps 3:18Mbps 4:24Mbps 5:36Mbps 6:48Mbps 7:54Mbps\n", rate_idx);
            return -EINVAL;
        }
        pkt_lens = rate_11g_pkt_lens;
        pkt_freqs = rate_11g_pkt_freqs;
    } else if (sc_index == BL_IOCTL_MP_11n_RATE) {
        uint32_t guard_interval = *(uint32_t *)(in_buf + 0); 
        uint32_t bandwidth = *(uint32_t *)(in_buf + 8);
        rate_idx = *(uint32_t *)(in_buf + 4);
        
        if (guard_interval != 0 && guard_interval != 1) {
            printk("Invalid gurad interval %d, valid is 0 for short guard interval, \
                    1 for long guard interval\n", 
                    guard_interval);
            return -EINVAL;
        }
        
        if (bandwidth != 2) { // && bandwidth != 4
            printk("Invalid bandwidth %d, valid is 2 for 20MHz, 4 for 40MHz,\
                    but BL602 only support 20MHz.\n", bandwidth);
            return -EINVAL;
        }

        if (rate_idx > 7) {
            printk("Invalid mcs_index %d, valid is in range 0-7.\n", rate_idx);
            return -EINVAL;
        }
        pkt_lens = rate_11n_pkt_lens;
        pkt_freqs = rate_11n_pkt_freqs;
    }

    memset(extra_in_buf, 0, sizeof(extra_in_buf));
    extra_sc_index = BL_IOCTL_MP_SET_TXDUTY;
    extra_in_len = 4;
    *(uint32_t *)extra_in_buf = DEFAULT_TX_DUTY;
    ret = bl_iwpriv_common_hdl(dev, info->cmd, wrqu, extra, iwp_mp_ms,
                               sizeof(iwp_mp_ms)/sizeof(struct iwpriv_cmd), extra_sc_index,
                               extra_in_buf, extra_in_len);
    if (ret != 0) {
        printk("%s, set tx duty 50 failed\n", __func__);
        return ret;
    }

    memset(extra_in_buf, 0, sizeof(extra_in_buf));
    extra_sc_index = BL_IOCTL_MP_SET_PKT_FREQ;
    extra_in_len = 4;
    memcpy(extra_in_buf, pkt_freqs+rate_idx, 4);
    ret = bl_iwpriv_common_hdl(dev, info->cmd, wrqu, extra, iwp_mp_ms,
                               sizeof(iwp_mp_ms)/sizeof(struct iwpriv_cmd), extra_sc_index,
                               extra_in_buf, extra_in_len);
    if (ret != 0) {
        printk("%s, set pkt freq failed\n", __func__);
        return ret;
    }

    memset(extra_in_buf, 0, sizeof(extra_in_buf));
    extra_sc_index = BL_IOCTL_MP_SET_PKT_LEN;
    extra_in_len = 4;
    memcpy(extra_in_buf, pkt_lens+rate_idx, 4);
    ret = bl_iwpriv_common_hdl(dev, info->cmd, wrqu, extra, iwp_mp_ms,
                               sizeof(iwp_mp_ms)/sizeof(struct iwpriv_cmd), extra_sc_index,
                               extra_in_buf, extra_in_len);
    if (ret != 0) {
        printk("%s, set pkt len failed\n", __func__);
    }

    return ret;
}

static int bl_iwpriv_common_hdl(struct net_device *dev, uint16_t cmd, union iwreq_data *wrqu, 
                         char *extra, struct iwpriv_cmd *iwp_cmds, uint32_t sc_num, 
                         uint16_t sc_index, uint8_t *in_buf, uint16_t in_len)
{
    int ret = 0;
    uint8_t *out_buf = NULL;
    uint16_t out_len = 0;
    uint32_t var_len = 0;
    uint16_t var_type = 0;
    uint8_t mp_cmd[100];
    bool exp_found = true;
    struct bl_vif *vif = netdev_priv(dev);
    struct bl_hw *bl_hw = vif->bl_hw;
    struct iwpriv_sub_cmd *iwp_sc = NULL;

    printk("%s, info->cmd:0x%x, sc_index:%d, sc:%s, in_len:%d\n", 
           __func__, cmd, sc_index, in_buf, in_len);

    if (sc_index > sc_num){
        printk("iwpriv cmd: 0x%x, but unknow sub cmd index: 0x%x\n", cmd, sc_index);
        dump_buf(in_buf, in_len);
        return -EINVAL;
    }

    out_buf = kzalloc(IWPRIV_OUT_BUF_LEN, GFP_KERNEL);
    if (out_buf == NULL) {
        printk("%s, fail to alloc out_buf\n", __func__);
        return -ENOMEM;
    }
    memset(out_buf, 0, IWPRIV_OUT_BUF_LEN);

    var_type = get_args_type(dev, cmd);

    ////debug
    //BL_DBG("dump in_buf\n");
    //dump_buf(in_buf, in_len);

    do {
        iwp_sc = get_sub_cmd(iwp_cmds[sc_index].sub_cmd, iwp_cmds[sc_index].sub_cmd_num, 
                             var_type, in_buf, in_len);
        
        if (iwp_sc == NULL) {
            printk("%s, right iwpriv cmd: %s, but unknow sub cmd\n", 
                   __func__, iwp_cmds[sc_index].name);
            dump_buf(in_buf, in_len);
            ret = -EINVAL;
            break;
        }

        if (in_len < (iwp_sc->sc_var_len&(~IS_VAR_LEN)) + iwp_sc->sc_len) {
            printk("%s, right iwpriv cmd: %s, wrong len of iwpriv sub cmd and var\n", 
                   __func__, iwp_cmds[sc_index].name);
            dump_buf(in_buf, in_len);
            ret = -EINVAL;
            break;
        }

        if (iwp_sc->ind_exp) {
            exp_found = false;    
        }

        memcpy(mp_cmd, iwp_sc->mfg_cmd_name, iwp_sc->mfg_cmd_len);

        in_len -= iwp_sc->sc_len;
        if (iwp_sc->sc_var_parser != NULL) {
            ret = iwp_sc->sc_var_parser((uint8_t *)in_buf + iwp_sc->sc_len, 
                                        in_len, mp_cmd + iwp_sc->mfg_cmd_len, &var_len);
            if (ret != 0) {
                printk("%s, right iwpriv cmd: %s, wrong iwpriv sub cmd var\n", 
                       __func__, iwp_cmds[sc_index].name);
                dump_buf(in_buf, in_len);
                ret = -EINVAL;
                break;
            }
            mp_cmd[iwp_sc->mfg_cmd_len + var_len] = '\n';
            mp_cmd[iwp_sc->mfg_cmd_len + var_len + 1] = '\0';
        } else {
            if ((iwp_sc->sc_var_len&IS_VAR_LEN) > 0) {
                var_len = strlen(in_buf) - iwp_sc->sc_len;
            } else if (iwp_sc->sc_var_len > 0) {
                var_len = iwp_sc->sc_var_len;
            }
            memcpy(mp_cmd + iwp_sc->mfg_cmd_len, in_buf + iwp_sc->sc_len, var_len);
            mp_cmd[iwp_sc->mfg_cmd_len + var_len] = '\n';
            mp_cmd[iwp_sc->mfg_cmd_len + var_len + 1] = '\0';
        }

        memset(bl_hw->iwp_var.iwpriv_ind, 0, IWPRIV_IND_LEN_MAX+1);
        bl_hw->iwp_var.iwpriv_ind_len = 0;

        ////debug
        BL_DBG("%s, mp_cmd:%s\n", __func__, mp_cmd);
        //dump_buf(mp_cmd, strlen(mp_cmd));
        
        ret = bl_send_mp_test_msg(bl_hw, mp_cmd, out_buf + out_len, false);
        if (ret) {
            printk("%s, bl_send_mp_test_msg:%s, error=%d\n", __func__, (char *)mp_cmd, ret);
            break;
        }

        if (iwp_sc->ind_wait_ms == 0) {
            uint16_t ind_len = le16_to_cpu(*(__le16 *)(out_buf + out_len));
            memcpy(out_buf + out_len, out_buf + out_len + 2, ind_len);
            out_len += ind_len;
        } else {
            BL_DBG("%s, wait ind and check\n", __func__);
            msleep_interruptible(iwp_sc->ind_wait_ms);
            
            out_len += merge_ind_2_out(bl_hw, out_buf + out_len);
            printk("%s, out_buf:%s\n", __func__, out_buf);

            if (iwp_sc->ind_exp) {
                ////debug
                //dump_buf(out_buf, out_len);
                if (find_ind_exp(out_buf, out_len, iwp_sc->ind_exp)) {
                    exp_found = true;
                    break;
                }
            }

            BL_DBG("%s, finish to wait ind and check\n", __func__);
        }
    } while (0);

    if (ret == 0 && iwp_sc->ind_exp && exp_found == false) {
        ret = 0;//-EPIPE;
    }

    if (ret == 0) {
        if(extra) {
            memcpy(extra + wrqu->data.length, out_buf, out_len);
            wrqu->data.length += out_len;
        } else {
            if (copy_to_user(wrqu->data.pointer + wrqu->data.length, out_buf, out_len)) {
                ret = -EFAULT;
            } else {
                wrqu->data.length += out_len;
            }
        }
    }

    if (out_buf != NULL)
        kfree(out_buf);

    return ret;
}

int bl_iwpriv_mp_ms_hdl(struct net_device *dev, struct iw_request_info *info, 
                        union iwreq_data *wrqu, char *extra)
{
    int ret = 0;
    uint16_t sc_index = wrqu->data.flags;
    uint8_t *in_buf = NULL;
    uint16_t in_len = wrqu->data.length;  
    uint16_t var_type = 0;
    struct bl_vif *vif = netdev_priv(dev);
    struct bl_hw *bl_hw = vif->bl_hw;

    BL_DBG("%s, cmd=0x%x, flags=0x%x, wrqu->len=%d, buf=%p, extra=%p\n", 
           __func__, info->cmd, info->flags, wrqu->data.length, wrqu->data.pointer, extra);

    if (!bl_mod_params.mp_mode) {
        printk("bl driver not in mp_mode\n");
        return -EINVAL;
    }

    var_type = get_args_type(dev, info->cmd);
    if (var_type == IW_PRIV_TYPE_INT)
        in_len *= sizeof(int32_t);

    in_buf = kzalloc(in_len + 1, GFP_KERNEL);
    if (in_buf == NULL) {
        printk("%s, fail to alloc buf for wrqu in data\n", __func__);
        return -ENOMEM;
    }

    if (in_len > 0) {
        if (copy_from_user(in_buf, wrqu->data.pointer, in_len)) {
            printk("%s, copy from user failed\n", __func__);
            kfree(in_buf);
            return -EFAULT;
        }
    }
    in_buf[in_len] = '\0';

    wrqu->data.length = 0;

    if (bl_hw->iwp_var.tx_duty == DEFAULT_TX_DUTY &&
        (sc_index == BL_IOCTL_MP_11b_RATE || sc_index == BL_IOCTL_MP_11g_RATE || sc_index == BL_IOCTL_MP_11n_RATE)) 
    {
        ret = mp_pre_rate_hdl(dev, info, wrqu, extra, sc_index, in_buf);
    }

    if (ret == 0) {
        ret = bl_iwpriv_common_hdl(dev, info->cmd, wrqu, extra, iwp_mp_ms,
                                   sizeof(iwp_mp_ms)/sizeof(struct iwpriv_cmd), sc_index,
                                   in_buf, in_len);
    }
    
    if (ret == 0) {
        if (sc_index == BL_IOCTL_MP_SET_TXDUTY) {
            bl_hw->iwp_var.tx_duty = *(uint32_t *)in_buf;
        } else if (sc_index == BL_IOCTL_MP_TX || sc_index == BL_IOCTL_MP_UNICAST_TX) {
            bl_hw->iwp_var.tx_duty = DEFAULT_TX_DUTY;
        }
    }

    if (in_buf != NULL)
        kfree(in_buf);

    return ret;
}

int bl_iwpriv_mp_mg_hdl(struct net_device *dev, struct iw_request_info *info, 
                        union iwreq_data *wrqu, char *extra)
{
    int ret = 0;
    uint16_t sc_index = wrqu->data.flags;
    uint8_t *in_buf = NULL;
    uint16_t in_len = wrqu->data.length;  
    uint16_t var_type = 0;
    struct bl_vif *vif = netdev_priv(dev);
    struct bl_hw *bl_hw = vif->bl_hw;

    BL_DBG("%s, cmd=0x%x, flags=0x%x, wrqu->len=%d, buf=%p, extra=%p\n", 
           __func__, info->cmd, info->flags, wrqu->data.length, wrqu->data.pointer, extra);

    if (!bl_mod_params.mp_mode) {
        printk("bl driver not in mp_mode\n");
        return -EINVAL;
    }

    var_type = get_args_type(dev, info->cmd);
    if (var_type == IW_PRIV_TYPE_INT)
        in_len *= sizeof(int32_t);

    in_buf = kzalloc(in_len + 1, GFP_KERNEL);
    if (in_buf == NULL) {
        printk("%s, fail to alloc buf for wrqu in data\n", __func__);
        return -ENOMEM;
    }

    if (in_len > 0) {
        if (copy_from_user(in_buf, wrqu->data.pointer, in_len)) {
            printk("%s, copy from user failed\n", __func__);
            kfree(in_buf);
            return -EFAULT;
        }
    }
    in_buf[in_len] = '\0';

    wrqu->data.length = 0;

    ret = bl_iwpriv_common_hdl(dev, info->cmd, wrqu, extra, iwp_mp_mg, 
                               sizeof(iwp_mp_mg)/sizeof(struct iwpriv_cmd), sc_index,
                               in_buf, in_len);

    if (ret == 0 && 
        (sc_index == BL_IOCTL_MP_EFUSE_WR || sc_index == BL_IOCTL_MP_EFUSE_CAP_WR ||
         sc_index == BL_IOCTL_MP_EFUSE_PWR_WR || sc_index == BL_IOCTL_MP_EFUSE_MAC_ADR_WR)) 
    {
        printk("%s, in_buf:%s\n", __func__, in_buf);

        ret = mp_efuse_write(bl_hw, sc_index, in_buf, wrqu, extra);
    }

    if (in_buf != NULL)
        kfree(in_buf);

    return ret;
}

int bl_iwpriv_mp_ind_hdl(struct net_device *dev, struct iw_request_info *info, 
                         union iwreq_data *wrqu, char *extra)
{
    int ret = 0;
    uint16_t out_len = 0;
    uint8_t *out_buf;
    struct bl_vif *vif = netdev_priv(dev);
    struct bl_hw *bl_hw = vif->bl_hw;

    BL_DBG("%s, cmd=0x%x, flags=0x%x, wrqu->len=%d, buf=%p, extra=%p\n", 
           __func__, info->cmd, info->flags, wrqu->data.length, wrqu->data.pointer,extra);

    if (!bl_mod_params.mp_mode) {
        printk("bl driver not in mp_mode\n");
        return -EINVAL;
    }

    out_buf = kzalloc(IWPRIV_OUT_BUF_LEN, GFP_KERNEL);
    if (out_buf == NULL) {
        printk("%s, fail to alloc out_buf\n", __func__);
        return -ENOMEM;
    }

    wrqu->data.length = 0;

    out_len += merge_ind_2_out(bl_hw, out_buf + out_len);

    memset(bl_hw->iwp_var.iwpriv_ind, 0, IWPRIV_IND_LEN_MAX+1);
    bl_hw->iwp_var.iwpriv_ind_len = 0;

    out_buf[out_len] = '\0';
    out_len += 1;
    wrqu->data.length = out_len;

    if(extra) {
        memcpy(extra, out_buf, wrqu->data.length);
    } else {
        if (copy_to_user(wrqu->data.pointer, out_buf, wrqu->data.length))
            ret = -EFAULT;
    }
  
    if (out_buf != NULL)
        kfree(out_buf);

    return ret;
}

int bl_iwpriv_help_hdl(struct net_device *dev, struct iw_request_info *info, 
                       union iwreq_data *wrqu, char *extra)
{
    int ret = 0;
    char * in_buf = NULL;
    uint32_t in_len = 0;
    uint16_t sc_index = 0;
    uint8_t *out_buf;
    uint32_t out_len = 0;
    uint32_t var_len = 0;

    BL_DBG("%s, cmd=0x%x, flags=0x%x, wrqu->len=%d, buf=%p, extra=%p\n", 
           __func__, info->cmd, info->flags, wrqu->data.length, wrqu->data.pointer, extra);

    in_len = wrqu->data.length;
    in_buf = kzalloc(wrqu->data.length + 1, GFP_KERNEL);
    if (in_buf == NULL) {
        printk("%s, fail to alloc buf for wrqu in data\n", __func__);
        return -ENOMEM;
    }

    out_buf = kzalloc(IWPRIV_OUT_BUF_LEN, GFP_KERNEL);
    if (out_buf == NULL) {
        printk("%s, fail to alloc out_buf\n", __func__);
        kfree(in_buf);
        return -ENOMEM;
    }

    if (wrqu->data.length > 0) {
        if (copy_from_user(in_buf, wrqu->data.pointer, wrqu->data.length)) {
            printk("%s, copy from user failed\n", __func__);
            kfree(in_buf);
            kfree(out_buf);
            return -EFAULT;
        }
    }
    in_buf[wrqu->data.length] = '\0';
    wrqu->data.length = 0;

    if (in_len == 0) {
        out_len += sprintf(out_buf, 
            "\nBL Wlan Driver Version is %s\n\nUsage:\n", RELEASE_VERSION);
    } else {        
        out_len += sprintf(out_buf, 
            "\nBL Wlan Driver Version is %s\n\nAll mp commands:\n", RELEASE_VERSION);
    }

    sc_index = info->cmd - SIOCIWFIRSTPRIV;
    iwp_mp_single[sc_index].sub_cmd[0].sc_var_parser(in_buf, in_len, out_buf + out_len, &var_len);
    out_len += var_len;

    //BL_DBG("%s, out_buf:%s\n", __func__, out_buf);

    if(extra) {
        memcpy(extra, out_buf, out_len);
        wrqu->data.length += out_len;
    } else {
        if (copy_to_user(wrqu->data.pointer, out_buf, out_len))
            ret = -EFAULT;
        else
            wrqu->data.length += out_len;
    }
  
    if (out_buf != NULL)
        kfree(out_buf);

    if (in_buf != NULL)
        kfree(in_buf);

    return ret;
}

int bl_iwpriv_mp_load_caldata_hdl(struct net_device *dev, struct iw_request_info *info, 
                                  union iwreq_data *wrqu, char *extra)
{
    int ret = 0;
    uint16_t in_len = 0;
    char * in_buf = NULL;
    uint16_t out_len = 0;
    uint8_t *out_buf;
    struct bl_vif *vif = netdev_priv(dev);
    struct bl_hw *bl_hw = vif->bl_hw;
    bool exp_found = true;
    struct iwpriv_sub_cmd *iwp_sc = &iwp_mp_sc_load_cal_data[0];

    int i = 0;
    uint8_t *cfg_buf;
    uint16_t cfg_buf_len = 512;
    uint8_t *pos = NULL;
    uint8_t hex_buf[256]={0};
    uint8_t buf_len = 0;    
    uint8_t xtalmode[6]={0x01, 0x00, 0x02, 0x00, 0x4d, 0x46}; //"MF" , first EFSUE, later ram
    uint8_t xtalmode_len=6;
    uint8_t capcode[24]={0x02, 0x00, 0x14, 0x00, 0x22, 0x00, 0x00, 0x00, 0x22, 0x00,
                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00};
    uint8_t capcode_len=24;
    uint8_t pwr_mode[6]={0x03, 0x00, 0x02, 0x00, 0x42, 0x46}; //"BF", first EFSUE, later ram
    uint8_t pwr_mode_len=6;
    uint8_t pwr_11b[8]={0x05, 0x00, 0x04, 0x00, 0x13, 0x13, 0x13, 0x13};
    uint8_t pwr_11b_len=8;
    uint8_t pwr_11g[12]={0x06, 0x00, 0x08, 0x00, 0x10, 0x10, 0x10, 0x10,0x10, 0x10, 0x10, 0x10};    
    uint8_t pwr_11g_len=12;
    uint8_t pwr_11n[12]={0x07, 0x00, 0x08, 0x00, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f};    
    uint8_t pwr_11n_len=12;
    uint8_t pwr_offset[18]={0x08, 0x00, 0x0e, 0x00, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a};    
    uint8_t pwr_offset_len=18;
    uint8_t sign[16]={0x42, 0x4c, 0x52, 0x46, 0x50, 0x41, 0x52, 0x41, 
                      0x4f, 0x36, 0x44, 0x6b, 0x58, 0x62, 0x31, 0x6b};
    uint8_t sign_len = 16;
	uint8_t en_tcal[5]={0x20, 0x00, 0x01, 0x00, 0x00};
	uint8_t en_tcal_len = 5;
	uint8_t linear_or_follow[5]={0x21, 0x00, 0x01, 0x00, 0x01};
	uint8_t linear_or_follow_len = 5;
	uint8_t channel[14]={0x22, 0x00, 0x0a, 0x00, 0x6c, 0x09, 0x7b, 0x09, 0x8a, 0x09, 0x99, 0x09, 0xa8, 0x09};
	uint8_t chan_len = 14;
	uint8_t chan_os[14] = {0x23, 0x00, 0x0a, 0x00, 0xb4, 0x00, 0xa8, 0x00, 0xa3, 0x00, 0xa0, 0x00, 0x9d, 0x00};
	uint8_t chan_os_len = 14;
	uint8_t chan_os_low[14] = {0x24, 0x00, 0x0a, 0x00, 0xc7, 0x00, 0xba, 0x00, 0xaa, 0x00, 0xa5, 0x00, 0xa0, 0x00};
	uint8_t chan_os_low_len = 14;
    uint8_t temp[14] = {0x25, 0x00, 0x02, 0x00, 0x00, 0x01, 0x30, 0x00, 0x04, 0x00, 0x0d, 0x00, 0x00, 0x00};
    uint8_t temp_len = 14;
    
    struct cfg_kv_t cfg_kv[] = {
        [CFG_CAPCODE_ID] = {
            .key =      "capcode",
            .sv_cnt =   1,
            .valid =    false,
        },
        [CFG_PWR_OFT_ID] = {
            .key =      "power_offset",
            .sv_cnt =   14,
            .valid =    false,
        },
        [CFG_PWR_MODE_ID] = {
            .key =      "pwr_mode",
            .sv_cnt =   2,
            .valid =    false,
        },
        [CFG_MAC_ID] = {
            .key =      "mac",
            .sv_cnt =   6,
            .valid =    false,
        },
        [CFG_EN_TCAL_ID] = {
            .key =      "en_tcal",
            .sv_cnt =   1,
            .valid =    false,
        },
        [CFG_TCHANNEL_OS_ID] = {
            .key =      "Tchannel_os",
            .sv_cnt =   10,
            .valid =    false,
        },
        [CFG_TCHANNEL_OS_LOW_ID] = {
            .key =      "Tchannel_os_low",
            .sv_cnt =   10,
            .valid =    false,
        },              
    };

    BL_DBG("%s, cmd=0x%x, flags=0x%x, wrqu->len=%d, buf=%p, extra=%p\n", 
           __func__, info->cmd, info->flags, wrqu->data.length, wrqu->data.pointer, extra);

    if (!bl_mod_params.mp_mode) {
        printk("bl driver in not mp_mode\n");
        return -EINVAL;
    }

    in_len = wrqu->data.length + 1;
    wrqu->data.length = 0;
    in_buf = kzalloc(in_len, GFP_KERNEL);
    if (in_buf == NULL) {
        printk("%s, fail to alloc buf for wrqu in data\n", __func__);
        return -ENOMEM;
    }

    out_buf = kzalloc(IWPRIV_OUT_BUF_LEN, GFP_KERNEL);
    if (out_buf == NULL) {
        printk("%s, fail to alloc out_buf\n", __func__);
        kfree(in_buf);
        return -ENOMEM;
    }
    memset(out_buf, 0, IWPRIV_OUT_BUF_LEN);

    if (in_len > 0) {
        if (copy_from_user(in_buf, wrqu->data.pointer, in_len)) {
            printk("%s, copy from user failed\n", __func__);
            kfree(in_buf);
            kfree(out_buf);
            return -EFAULT;
        }
    }
    in_buf[in_len] = '\0';

    cfg_buf = kzalloc(cfg_buf_len, GFP_KERNEL);
    if (cfg_buf == NULL) {
        printk("%s, fail to alloc cfg_buf\n", __func__);
        kfree(in_buf);
        kfree(out_buf);
        return -ENOMEM;
    }
    memset(cfg_buf, 0, cfg_buf_len);

    //debug
    //BL_DBG("%s, in_buf:%s\n", __func__, in_buf);

    memset(bl_hw->iwp_var.iwpriv_ind, 0, IWPRIV_IND_LEN_MAX+1);
    bl_hw->iwp_var.iwpriv_ind_len = 0;

    //debug
    //BL_DBG("%s, in_buf:%s\n", __func__, in_buf);

    do {
        ret = load_cal_data(in_buf, cfg_kv, sizeof(cfg_kv)/sizeof(cfg_kv[0]));
        if (ret < 0) {
            printk("%s, read file error, ret:%d\n", __func__, ret);
            break;
        }

        if (cfg_kv[CFG_CAPCODE_ID].valid) {
            *(uint32_t *)(capcode + 4) = (uint32_t)cfg_kv[CFG_CAPCODE_ID].sv[0];
            *(uint32_t *)(capcode + 8) = (uint32_t)cfg_kv[CFG_CAPCODE_ID].sv[0];
            //*(uint32_t *)(capcode + 12) = (uint32_t)cfg_kv[CFG_CAPCODE_ID].sv[0];
            //*(uint32_t *)(capcode + 12) = 0;
            //*(uint32_t *)(capcode + 16) = (uint32_t)cfg_kv[CFG_CAPCODE_ID].sv[0];
            //*(uint32_t *)(capcode + 20) = (uint32_t)cfg_kv[CFG_CAPCODE_ID].sv[0];
        }
        if (cfg_kv[CFG_PWR_OFT_ID].valid) {
            for (i = 0; i< 14; i++) {
                pwr_offset[4+i]= cfg_kv[CFG_PWR_OFT_ID].sv[i] + 10;
            }
        }
        if (cfg_kv[CFG_PWR_MODE_ID].valid) {
            for (i = 0; i< 2; i++) {
                pwr_mode[4+i]= cfg_kv[CFG_PWR_MODE_ID].sv[i];
            }
        }
        if (cfg_kv[CFG_EN_TCAL_ID].valid) {
            en_tcal[4] = cfg_kv[CFG_EN_TCAL_ID].sv[0];
        }
        if (cfg_kv[CFG_TCHANNEL_OS_ID].valid) {
            for (i = 0; i< 10; i++) {
                chan_os[4+i]= cfg_kv[CFG_TCHANNEL_OS_ID].sv[i];
            }
        }
		if (cfg_kv[CFG_TCHANNEL_OS_LOW_ID].valid) {
            for (i = 0; i< 10; i++) {
                chan_os_low[4+i]= cfg_kv[CFG_TCHANNEL_OS_LOW_ID].sv[i];
            }
        }
        //copy data to buf
        pos = hex_buf;
        buf_len = 0;
        //sign header
        memcpy(pos, sign, sign_len);
        buf_len += sign_len;
        pos +=sign_len;
        //xtal mode
        memcpy(pos, xtalmode, xtalmode_len);
        buf_len +=xtalmode_len;
        pos +=xtalmode_len;
        //xtal capcode
        memcpy(pos, capcode, capcode_len);
        buf_len +=capcode_len;
        pos +=capcode_len;        
        //power mode
        memcpy(pos, pwr_mode, pwr_mode_len);
        buf_len +=pwr_mode_len;
        pos +=pwr_mode_len;    
        //11b power
        memcpy(pos, pwr_11b, pwr_11b_len);
        buf_len +=pwr_11b_len;
        pos +=pwr_11b_len;        
        //11g power
        memcpy(pos, pwr_11g, pwr_11g_len);
        buf_len +=pwr_11g_len;
        pos +=pwr_11g_len;        
        //11n power
        memcpy(pos, pwr_11n, pwr_11n_len);
        buf_len +=pwr_11n_len;
        pos +=pwr_11n_len;        
        //powr offset
        memcpy(pos, pwr_offset, pwr_offset_len);
        buf_len +=pwr_offset_len;
        pos +=pwr_offset_len;
		//en_tcal
        memcpy(pos, en_tcal, en_tcal_len);
        buf_len +=en_tcal_len;
        pos +=en_tcal_len;		
		//linear_or_follow
        memcpy(pos, linear_or_follow, linear_or_follow_len);
        buf_len +=linear_or_follow_len;
        pos +=linear_or_follow_len;			
		//channel
        memcpy(pos, channel, chan_len);
        buf_len +=chan_len;
        pos +=chan_len;			
		//Tchannel_os
        memcpy(pos, chan_os, chan_os_len);
        buf_len +=chan_os_len;
        pos +=chan_os_len;			
		//Tchannel_os_low
        memcpy(pos, chan_os_low, chan_os_low_len);
        buf_len +=chan_os_low_len;
        pos +=chan_os_low_len;		
		//
        //temperature and ble parameter
        memcpy(pos, temp, temp_len);
        buf_len +=temp_len;
        pos +=temp_len;      

        //mfg cmd
        memcpy(cfg_buf, iwp_sc->mfg_cmd_name, iwp_sc->mfg_cmd_len);
        cfg_buf_len = iwp_sc->mfg_cmd_len;
        for (i = 0; i < buf_len; i++) {
            cfg_buf_len += sprintf(cfg_buf + cfg_buf_len, "%02x", hex_buf[i]);
        }
        cfg_buf[cfg_buf_len] = '\n';
        cfg_buf[cfg_buf_len + 1] = '\0';
        cfg_buf_len += 1;
        
        printk("%s, cfg_buf_len:%d, cfg_buf:%s\n", __func__, cfg_buf_len, cfg_buf);

        if (iwp_sc->ind_exp) {
            exp_found = false;    
        }
        
        memset(bl_hw->iwp_var.iwpriv_ind, 0, IWPRIV_IND_LEN_MAX+1);
        bl_hw->iwp_var.iwpriv_ind_len = 0;
        
        ret = bl_send_mp_test_msg(bl_hw, cfg_buf, out_buf + out_len, false);
    
        if (ret) {
            printk("%s, bl_send_mp_test_msg, error=%d\n", __func__, ret);
            break;
        } else {
            out_len += sprintf(out_buf, "\n%s, success to recv cfm from firmware\n", __func__);
        }
    
        if (iwp_sc->ind_wait_ms == 0) {
            uint16_t ind_len = le16_to_cpu(*(__le16 *)(out_buf + out_len));
            memcpy(out_buf + out_len, out_buf + out_len + 2, ind_len);
            out_len += ind_len;
        } else {
            BL_DBG("%s, wait ind and check\n", __func__);
            msleep_interruptible(iwp_sc->ind_wait_ms);
            
            out_len += merge_ind_2_out(bl_hw, out_buf + out_len);
    
            if (iwp_sc->ind_exp) {
                ////debug
                //dump_buf(out_buf, out_len);
                if (find_ind_exp(out_buf, out_len, iwp_sc->ind_exp)) {
                    exp_found = true;
                    break;
                }
            }
    
            BL_DBG("%s, finish to wait ind and check\n", __func__);
        }
    } while (0);

    out_buf[out_len] = '\0';
    out_len += 1;
    
    if (ret == 0 && iwp_sc->ind_exp && exp_found == false) {
        ret = -EPIPE;
    }
    
    if(extra) {
        memcpy(extra + wrqu->data.length, out_buf, out_len);
        wrqu->data.length += out_len;
    } else {
        if (copy_to_user(wrqu->data.pointer + wrqu->data.length, out_buf, out_len))
            ret = -EFAULT;
        else
            wrqu->data.length += out_len;
    }
    
    if (in_buf != NULL)
        kfree(in_buf);
    
    if (out_buf != NULL)
        kfree(out_buf);
    
    if (cfg_buf != NULL)
        kfree(cfg_buf);
    
    return ret;

}

#endif //end of CONFIG_BL_MP

int bl_iwpriv_wmmcfg_hdl(struct net_device *dev, struct iw_request_info *info, 
                         union iwreq_data *wrqu, char *extra)
{
    int ret = 0;
    char * in_buf = NULL;
    uint32_t in_len = 0;
    uint8_t *out_buf = NULL;
    uint16_t out_len = 0;
	uint8_t out_buf_len = 200;
    uint32_t var_value = 0;
    uint16_t var_type = 0;
    struct bl_vif *vif = netdev_priv(dev);
    struct bl_hw *bl_hw = vif->bl_hw;

    BL_DBG("%s, cmd=0x%x, flags=0x%x, wrqu->len=%d, buf=%p, extra=%p\n", __func__, info->cmd, info->flags, wrqu->data.length, wrqu->data.pointer, extra);

    if (bl_mod_params.mp_mode) {
        printk("bl driver in mp_mode\n");
        return -EINVAL;
    }

    var_type = get_args_type(dev, info->cmd);
    in_len = wrqu->data.length;
    if (var_type == IW_PRIV_TYPE_INT)
        in_len *= sizeof(int32_t);

    in_buf = kzalloc(in_len + 1, GFP_KERNEL);
    if (in_buf == NULL)
        return -ENOMEM;

    out_buf = kzalloc(out_buf_len, GFP_KERNEL);
    if (out_buf == NULL) {
        kfree(in_buf);
        return -ENOMEM;
    }

    if (in_len > 0) {
        if (copy_from_user(in_buf, wrqu->data.pointer, in_len)) {
            kfree(in_buf);
            kfree(out_buf);
            return -EFAULT;
        }
    }
    wrqu->data.length = 0;

    // dump_buf(in_buf, in_len);

    do {
        var_value = *(uint32_t *)in_buf;
        
        ret = bl_send_wmmcfg(bl_hw, var_value);
        if (ret) {
            printk("bl_send_wmmcfg, error=%d\n", ret);
            if (ret == -ETIMEDOUT)
                wrqu->data.length += sprintf(out_buf, "\nFail, timeout when wait for wmmcfg response from firmware\n");
            else
                wrqu->data.length += sprintf(out_buf, "\nFail to wmmcfg, %d\n", ret);
            break;
        }

        out_buf[wrqu->data.length] = '\0';
        wrqu->data.length += 1;
    } while (0);

    if(extra) {
        memcpy(extra, out_buf, wrqu->data.length);
    } else {
        if (copy_to_user(wrqu->data.pointer, out_buf, wrqu->data.length)) {
            ret = -EFAULT;
        }
    }

    if (out_buf != NULL)
        kfree(out_buf);

    if (in_buf != NULL)
        kfree(in_buf);

    return ret;
}


int bl_caldata_cfg_file_handle(struct bl_hw *bl_hw, char *file_path)
{
    int ret = 0;
    int i = 0, j=0;

    struct cfg_kv_t cfg_kv[] = {
        [CFG_CAPCODE_ID] = {
            .key =      "capcode",
            .sv_cnt =   1,
            .valid =    false,
        },
        [CFG_PWR_OFT_ID] = {
            .key =      "power_offset",
            .sv_cnt =   14,
            .valid =    false,
        },
        [CFG_PWR_MODE_ID] = {
            .key =      "pwr_mode",
            .sv_cnt =   2,
            .valid =    false,
        },
        [CFG_MAC_ID] = {
            .key =      "mac",
            .sv_cnt =   6,
            .valid =    false,
        },
    };

    if (bl_mod_params.mp_mode) {
        printk("bl driver in mp_mode\n");
        return -EINVAL;
    }

    //debug
    //BL_DBG("%s, in_buf:%s\n", __func__, in_buf);

    do {
        ret = load_cal_data(file_path, cfg_kv, sizeof(cfg_kv)/sizeof(cfg_kv[0]));
        if (ret < 0) {
            printk("%s, read file or parse error, ret:%d\n", __func__, ret);
            break;
        }
        
        ret = bl_send_cal_cfg(bl_hw, cfg_kv[CFG_CAPCODE_ID].valid, cfg_kv[CFG_CAPCODE_ID].sv[0], 
                              cfg_kv[CFG_PWR_OFT_ID].valid, cfg_kv[CFG_PWR_OFT_ID].sv, 
                              cfg_kv[CFG_MAC_ID].valid, cfg_kv[CFG_MAC_ID].sv);
        if (ret) {
            printk("%s, bl_send_cal_cfg, error=%d\n", __func__, ret);
        } 
    } while(0);

    return ret;
}

int bl_iwpriv_ver_hdl(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
    struct bl_vif *bl_vif = netdev_priv(dev);
    struct bl_hw *bl_hw = bl_vif->bl_hw;

    int ret = 0;
    char * in_buf = NULL;
    u8 out_buf[128];

    BL_DBG("%s, cmd=0x%x, flags=0x%x, wrqu->len=%d, buf=%p, extra=%p\n", 
           __func__, info->cmd, info->flags, wrqu->data.length, wrqu->data.pointer,extra);

    in_buf = kzalloc(wrqu->data.length + 1, GFP_KERNEL);
    if (in_buf == NULL)
        return -ENOMEM;

    if (wrqu->data.length > 0) {
        if (copy_from_user(in_buf, wrqu->data.pointer, wrqu->data.length)) {
            kfree(in_buf);
            return -EFAULT;
        }
    }
    in_buf[wrqu->data.length] = '\0';
    wrqu->data.length = 0;
		
    if (strcmp(in_buf, "Signed") == 0) {
	    if (bl_mod_params.mp_mode) {
	        wrqu->data.length += sprintf(out_buf, "\nBL Wlan Driver Ver: %s.\nSigned: %s\n", 
	                                         RELEASE_VERSION, RELEASE_SIGNED);
        } else {
            wrqu->data.length += sprintf(out_buf, "\nBL Wlan Driver Ver: %s. FW Ver: %s.\nSigned:%s, %s\n", 
                                         RELEASE_VERSION, bl_hw->wiphy->fw_version, RELEASE_SIGNED, fw_signed);
        }
    } else {
        if (bl_mod_params.mp_mode) {
            wrqu->data.length += sprintf(out_buf, "\nBL Wlan Driver Ver: %s\n", 
                                         RELEASE_VERSION);
        } else {
            wrqu->data.length += sprintf(out_buf, "\nBL Wlan Driver Ver: %s. FW Ver: %s\n", 
                                         RELEASE_VERSION, bl_hw->wiphy->fw_version);
        }
    }
    
    if(extra) {
        memcpy(extra, out_buf, wrqu->data.length);
    } else {
        if (copy_to_user(wrqu->data.pointer, out_buf, wrqu->data.length))
            ret = -EFAULT;
    }
  
    if (in_buf != NULL)
        kfree(in_buf);

    return ret;
}

int bl_iwpriv_dbg_hdl(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
    int ret = 0;
    char * in_buf = NULL;
    uint32_t in_len = 0;

    uint32_t var_value = 0;
    uint16_t var_type = 0;
    struct bl_vif *vif = netdev_priv(dev);
    struct bl_hw *bl_hw = vif->bl_hw;

    BL_DBG("%s, cmd=0x%x, flags=0x%x, wrqu->len=%d, buf=%p, extra=%p\n", __func__, info->cmd, info->flags, wrqu->data.length, wrqu->data.pointer, extra);

    if (bl_mod_params.mp_mode) {
        printk("bl driver in mp_mode\n");
        return -EINVAL;
    }

    var_type = get_args_type(dev, info->cmd);
    in_len = wrqu->data.length;
    if (var_type == IW_PRIV_TYPE_INT)
        in_len *= sizeof(int32_t);

    in_buf = kzalloc(in_len + 1, GFP_KERNEL);
    if (in_buf == NULL)
        return -ENOMEM;


    if (in_len > 0) {
        if (copy_from_user(in_buf, wrqu->data.pointer, in_len)) {
            kfree(in_buf);
            return -EFAULT;
        }
    }
    wrqu->data.length = 0;
	var_value = *(uint32_t *)in_buf;

    if (var_value)
		bl_hw->dbg = var_value;
	else
		bl_hw->dbg = 0;

	dump_info(bl_hw);

    if (in_buf != NULL)
        kfree(in_buf);

    return ret;
}


int bl_iwpriv_temp_read_hdl(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
    struct bl_vif *bl_vif = netdev_priv(dev);
    struct bl_hw *bl_hw = bl_vif->bl_hw;

    int ret = 0;
    int temp = 0;

    BL_DBG("%s, cmd=0x%x, flags=0x%x, wrqu->len=%d, buf=%p, extra=%p\n", __func__, info->cmd, info->flags, wrqu->data.length, wrqu->data.pointer,extra);

    wrqu->data.length = 0;

    if(bl_send_temp_read_req(bl_hw, &temp))
        return -1;

    wrqu->data.length += sizeof(temp);

    if(extra) {
        memcpy(extra, &temp, wrqu->data.length);
    } else {
        if (copy_to_user(wrqu->data.pointer, &temp, wrqu->data.length))
            ret = -EFAULT;
    }

    return ret;

}


/*formatModTx, mcs, nss, bwTx, shortGITx*/
int bl_iwpriv_set_rate_hdl(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
    int ret = 0;
    union bl_rate_ctrl_info rate_config;
    union bl_mcs_index *r = (union bl_mcs_index *)&rate_config;
    struct bl_vif *vif = netdev_priv(dev);
    struct bl_hw *bl_hw = vif->bl_hw;
    struct bl_sta *sta =vif->sta.ap;
    s8 info_buf[6] = {0};

    BL_DBG("%s, cmd=0x%x, flags=0x%x, wrqu->len=%d, buf=%p, extra=%p\n", 
           __func__, info->cmd, info->flags, wrqu->data.length, wrqu->data.pointer, extra);

    if (bl_mod_params.mp_mode) {
        printk("bl driver in mp_mode\n");
        return -EINVAL;
    }

    rate_config.value = 0;

    if((wrqu->data.length < 1) || (wrqu->data.length > 6) ) {
        printk("cmd usage: more info see README:\n");
        printk("cancel fixrate: iwpriv wlanx setrate -1\n");
	    printk("setrate: iwpriv wlanx setrate [format_mode][mcs] \n");
        printk("setrate: iwpriv wlanx setrate [format_mode][mcs][giAndpreType] \n");	
        printk("setrate: iwpriv wlanx setrate [format_mode][mcs][giAndpreType][nss][bw]  \n");
        printk("setrate: iwpriv wlanx setrate [format_mode][mcs][giAndpreType][nss][bw][dcm]  \n");
        return 0;
    }

    if(vif->sta.ap == NULL) {
        printk("connect ap first\n");
        return 0;
    }

    printk("vif->sta_idx=%d, valid=%d\n", vif->sta.ap->sta_idx, vif->sta.ap->valid);

    if (copy_from_user(info_buf, wrqu->data.pointer, sizeof(info_buf)))
        return -EFAULT;

    if(info_buf[0] == -1) {
        rate_config.value = (u32)-1;
    } else {
        rate_config.formatModTx = info_buf[0];
        if(rate_config.formatModTx == FORMATMOD_NON_HT) {
            rate_config.mcsIndexTx = info_buf[1];
        } else if(rate_config.formatModTx == FORMATMOD_HT_MF) {
            r->ht.mcs = info_buf[1];
			if(wrqu->data.length > 3)
            r->ht.nss = info_buf[3];//info_buf[2];
        }else { 
            r->vht.mcs = info_buf[1];
			if(wrqu->data.length > 3)
            r->vht.nss = info_buf[3];//info_buf[2];
        }

		if(wrqu->data.length > 2) {
            if(rate_config.formatModTx == FORMATMOD_NON_HT)
                rate_config.preTypeTx = info_buf[2];
            else
                rate_config.shortGITx = info_buf[2];//info_buf[4];
                rate_config.preTypeTx = (info_buf[2]>>1);
        }
     
		if(wrqu->data.length > 4)
        rate_config.bwTx = info_buf[4];
    }

    if(info_buf[0] != -1)
        printk("info_buf: %d %d %d %d %d\n", info_buf[0], info_buf[1], info_buf[2], info_buf[3], info_buf[4]);
    else
        printk("info_buf: %d\n", info_buf[0]);

    printk("rate_config.value=%0x\n", rate_config.value);

     // Forward the request to the LMAC
     if ((ret = bl_send_me_rc_set_rate(bl_hw, sta->sta_idx,
                                           (u16)rate_config.value)) != 0)
     {
         printk("%s failed, ret=%d\n", __func__, ret);
         return ret;
     }

    return ret;
}

int bl_iwpriv_rw_mem_hdl(struct net_device *dev, struct iw_request_info *info, union iwreq_data *wrqu, char *extra)
{
    int ret = 0;
    char * in_buf = NULL;
    uint32_t in_len = 0;
    uint8_t *out_buf = NULL;
	uint8_t out_buf_len = 200;
    uint16_t var_type = 0;
    struct bl_vif *vif = netdev_priv(dev);
    struct bl_hw *bl_hw = vif->bl_hw;

    struct dbg_mem_read_cfm cfm;
    uint32_t mem_addr = 0;
    uint32_t mem_data = 0;
    uint32_t rw_cmd = 0;

    /** Read addr: iwpriv wlan0 rw_mem 0 addr */
    /** write addr: iwpriv wlan0 rw_mem 1 addr value */
    BL_DBG("%s, cmd=0x%x, flags=0x%x, wrqu->len=%d, buf=%p, extra=%p\n", 
           __func__, info->cmd, info->flags, wrqu->data.length, wrqu->data.pointer, extra);

    var_type = get_args_type(dev, info->cmd);
    in_len = wrqu->data.length;
    if (var_type == IW_PRIV_TYPE_INT)
        in_len *= sizeof(int32_t);

    in_buf = kzalloc(in_len + 1, GFP_KERNEL);
    if (in_buf == NULL)
        return -ENOMEM;

    out_buf = kzalloc(out_buf_len, GFP_KERNEL);
    if (out_buf == NULL) {
        kfree(in_buf);
        return -ENOMEM;
    }

    if (in_len > 0) {
        if (copy_from_user(in_buf, wrqu->data.pointer, in_len)) {
            kfree(in_buf);
            kfree(out_buf);
            return -EFAULT;
        }
    }
    wrqu->data.length = 0;

    do {
        memset(&cfm, 0, sizeof(cfm));

        if (in_len < 2*sizeof(uint32_t)) {
            printk("%s, wrong input param len %d, at least 2 uint32_t as rw cmd and param\n",
                   __func__, in_len);
            ret = -EINVAL;
            break;
        }

        rw_cmd = *(uint32_t *)in_buf;
        mem_addr = *(uint32_t *)(in_buf + 4);

        if (mem_addr&0x3) {
            printk("%s, addr 0x%x is not 4 bytes align\r\n", __func__, mem_addr);
            ret = -EINVAL;
            break;
        }
        
        if (rw_cmd != 0) //write
        {
            if (in_len != 3*sizeof(uint32_t)) {
                printk("%s, wrong input param len %d for write mem cmd, should be %d=3*sizeof(uint32_t)\n",
                       __func__, in_len, 3*sizeof(uint32_t));
                ret = -EINVAL;
                break;
            }
            mem_data = *(uint32_t *)(in_buf + 8);
            ret = bl_send_dbg_mem_write_req(bl_hw, mem_addr, mem_data);
        }
        else
        {
            ret = bl_send_dbg_mem_read_req(bl_hw, mem_addr, &cfm);
        }

        if (ret) {
            printk("bl_send_dbg_mem_write/read_req, error=%d\n", ret);
            if (ret == -ETIMEDOUT)
                wrqu->data.length += sprintf(out_buf, "\nFail, timeout when wait for rw mem response from firmware\n");
            else
                wrqu->data.length += sprintf(out_buf, "\nFail to rw mem, %d\n", ret);
            break;
        }

        if (rw_cmd != 0) {
            printk("%s, success to write addr 0x%x with value 0x%x\n", __func__, mem_addr, mem_data);
        } 
        else 
        {
            printk("%s, success to read addr 0x%x with value 0x%x\n", __func__, cfm.memaddr, cfm.memdata);
        }

        out_buf[wrqu->data.length] = '\0';
        wrqu->data.length += 1;
    } while (0);

    if(extra) {
        memcpy(extra, out_buf, wrqu->data.length);
    } else {
        if (copy_to_user(wrqu->data.pointer, out_buf, wrqu->data.length)) {
            ret = -EFAULT;
        }
    }

    if (out_buf != NULL)
        kfree(out_buf);

    if (in_buf != NULL)
        kfree(in_buf);

    return ret;

}



//**************************************************************************
//****** set cmd table for iwpriv set, parameter in INT type *****
//**************************************************************************
static const struct bl_dev_priv_cmd_node bl_set_sub_int_cmd_table[] = {
    0
};

int bl_iwpriv_cmd_set_type_int(struct net_device *dev, struct iw_request_info *info, 
                                        union iwreq_data *wrqu, char *extra)
{
    int ret = 0;    
    uint16_t sub_oid = wrqu->data.flags;

    BL_DBG("%s, cmd=0x%x, flags=0x%x, wrqu->len=%d, buf=%p, extra=%p\n", 
           __func__, info->cmd, info->flags, wrqu->data.length, wrqu->data.pointer, extra);

    if (bl_mod_params.mp_mode) {
        printk("bl driver in mp_mode\n");
        ret = -EINVAL;
        return ret;
    }

    if (sub_oid > sizeof(bl_set_sub_int_cmd_table)/sizeof(struct bl_dev_priv_cmd_node)) {
        printk("sub_oid overflow array number\n");
        ret = -EINVAL;
        return ret;
    }
    
    ret = bl_set_sub_int_cmd_table[sub_oid].hdl(dev, info, wrqu, extra);

    return ret;
}

//dbg purpose, need firmware to support
int bl_iwpriv_cmd_read_scratch_hdl(struct net_device *dev, struct iw_request_info *info, 
                                              union iwreq_data *wrqu, char *extra)
{
    struct bl_vif *bl_vif = netdev_priv(dev);
    struct bl_hw *bl_hw = bl_vif->bl_hw;

    int ret = 0;
    u32 rd_addr;
    u8 out_buf[128];
    uint32_t scratch_reg = 0x60;
    uint32_t mask = 0x80000000;
    uint32_t read_cnt_max = 1000;
    uint32_t read_cnt = 0;
    const int dump_cnt = 8;
    uint8_t dump_data[8];

    char in_buf[128];
    uint32_t in_len = 0;
    uint16_t out_len = 0;
    int i = 0;

    BL_DBG("%s, cmd=0x%x, flags=0x%x, wrqu->len=%d, buf=%p, extra=%p\n", 
           __func__, info->cmd, info->flags, wrqu->data.length, wrqu->data.pointer, extra);

    in_len = wrqu->data.length*4;
    if (copy_from_user(in_buf, wrqu->data.pointer, in_len)) {
        kfree(in_buf);
        return -EFAULT;
    }

    wrqu->data.length = 0;

    rd_addr = *(uint32_t *)in_buf;
    if (rd_addr != 0xffffffff && rd_addr != 0x7fffffff && (rd_addr&0x3) > 0)
    {
        printk("%s not 4 bytes align addr:0x%x\r\n", __func__, rd_addr);
        return -EFAULT;
    }
    
    rd_addr = rd_addr|mask;
    for(i=0; i<dump_cnt/2; i++) {
        bl_write_reg(bl_hw, scratch_reg + i, *((uint8_t *)&rd_addr + i));
    }

    if (rd_addr == 0x7fffffff || rd_addr == 0xffffffff) {
        out_len += sprintf(out_buf+out_len, "dumping\r\n");
        void dump_fw_criticals(struct bl_hw *bl_hw);
        dump_fw_criticals(bl_hw);
        out_len += sprintf(out_buf+out_len, "dump done\r\n");
    } else {
        rd_addr = rd_addr&~mask;
        
        while (read_cnt++ < read_cnt_max) {
            msleep_interruptible(1);

            for (i=0; i<dump_cnt/2; i++) {
                bl_read_reg(bl_hw, scratch_reg+i, dump_data+i);
                //printk("%d: 0x%x\r\n", i, dump_data[i]);
            }

            //printk("read_cnt:%d\r\n", read_cnt);
            
            if (*(uint32_t *)dump_data == rd_addr) {
                for (i=0; i<dump_cnt; i++) {
                    bl_read_reg(bl_hw, scratch_reg+i, dump_data+i);
                    //printk("%d: 0x%x\r\n", i, dump_data[i]);
                }

                //printk(KERN_CRIT"read scratch: 0x%x\n", *(uint32_t *)(dump_data+4));
                out_len += sprintf(out_buf+out_len, "0x%08x:0x%08x\r\n", rd_addr, *(uint32_t *)(dump_data+4));
                break;
            }
        }

        if (read_cnt >= read_cnt_max)
            out_len += sprintf(out_buf+out_len, "failed\r\n");
    }
    
    wrqu->data.length += out_len;
    out_buf[wrqu->data.length] = '\0';
    wrqu->data.length += 1;

    if(extra) {
        memcpy(extra, out_buf, wrqu->data.length);
    } else {
        if (copy_to_user(wrqu->data.pointer, out_buf, wrqu->data.length))
            ret = -EFAULT;
    }
    
    return ret;
}

//**************************************************************************
//****** get cmd table for iwpriv set, parameter in INT type *****
//**************************************************************************
static const struct bl_dev_priv_cmd_node bl_get_sub_int_cmd_table[] = {
    //dbg purpose, need firmware to support
    {"read_scratch",                    bl_iwpriv_cmd_read_scratch_hdl},
};

int bl_iwpriv_cmd_get_type_int(struct net_device *dev, struct iw_request_info *info, 
                                        union iwreq_data *wrqu, char *extra)
{
    int ret = 0;    
    uint16_t sub_oid = wrqu->data.flags;

    BL_DBG("%s, cmd=0x%x, flags=0x%x, wrqu->len=%d, buf=%p, extra=%p\n", 
           __func__, info->cmd, info->flags, wrqu->data.length, wrqu->data.pointer, extra);

    if (bl_mod_params.mp_mode) {
        printk("bl driver in mp_mode\n");
        ret = -EINVAL;
        return ret;
    }

    if (sub_oid > sizeof(bl_get_sub_int_cmd_table)/sizeof(struct bl_dev_priv_cmd_node)) {
        printk("sub_oid overflow array number\n");
        ret = -EINVAL;
        return ret;
    }
    
    ret = bl_get_sub_int_cmd_table[sub_oid].hdl(dev, info, wrqu, extra);
   
    return ret;
}
