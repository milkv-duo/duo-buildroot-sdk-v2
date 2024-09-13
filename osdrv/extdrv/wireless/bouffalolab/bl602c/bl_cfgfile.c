/**
 ******************************************************************************
 *
 *  @file bl_cfgfile.c
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

#include <linux/firmware.h>
#include <net/mac80211.h>

#include "bl_defs.h"
#include "bl_cfgfile.h"

/**
 *
 */
static const char *bl_find_tag(const u8 *file_data, unsigned int file_size,
                                 const char *tag_name, unsigned int tag_len)
{
    unsigned int curr, line_start = 0, line_size;

    BL_DBG(BL_FN_ENTRY_STR);

    /* Walk through all the lines of the configuration file */
    while (line_start < file_size) {
        /* Search the end of the current line (or the end of the file) */
        for (curr = line_start; curr < file_size; curr++)
            if (file_data[curr] == '\n')
                break;

        /* Compute the line size */
        line_size = curr - line_start;

        /* Check if this line contains the expected tag */
        if ((line_size == (strlen(tag_name) + tag_len)) &&
            (!strncmp(&file_data[line_start], tag_name, strlen(tag_name))))
            return (&file_data[line_start + strlen(tag_name)]);

        /* Move to next line */
        line_start = curr + 1;
    }

    /* Tag not found */
    return NULL;
}

int bl_read_file(uint8_t *file_name, uint8_t *buf, uint16_t buf_len) {
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

/**
 * Parse the Config file used at init time
 */
int bl_parse_configfile(struct bl_hw *bl_hw, const char *filename,
                          struct bl_conf_file *config)
{
    const struct firmware *config_fw;
    u8 dflt_mac[ETH_ALEN] = { 0xaa, 0x00, 0xcc, 0xdd, 0xec, 0xf8 };
    int ret;
    const u8 *tag_ptr;

    BL_DBG(BL_FN_ENTRY_STR);

    if ((ret = request_firmware(&config_fw, filename, bl_hw->dev))) {
        BL_DBG(KERN_CRIT "%s: Failed to get %s (%d)\n", __func__, filename, ret);
        return ret;
    }

    /* Get MAC Address */
    tag_ptr = bl_find_tag(config_fw->data, config_fw->size,
                            "MAC_ADDR=", strlen("00:00:00:00:00:00"));
    if (tag_ptr != NULL) {
        u8 *addr = config->mac_addr;
        if (sscanf(tag_ptr,
                   "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                   addr + 0, addr + 1, addr + 2,
                   addr + 3, addr + 4, addr + 5) != ETH_ALEN)
            memcpy(config->mac_addr, dflt_mac, ETH_ALEN);
    } else
        memcpy(config->mac_addr, dflt_mac, ETH_ALEN);

    printk("MAC Address is:\n%pM\n", config->mac_addr);

    /* Release the configuration file */
    release_firmware(config_fw);

    return 0;
}

/**
 * Parse the Config file used at init time
 */
int bl_parse_phy_configfile(struct bl_hw *bl_hw, const char *filename,
                              struct bl_phy_conf_file *config)
{
    const struct firmware *config_fw;
    int ret;
    const u8 *tag_ptr;

    BL_DBG(BL_FN_ENTRY_STR);

    if ((ret = request_firmware(&config_fw, filename, bl_hw->dev))) {
        BL_DBG(KERN_CRIT "%s: Failed to get %s (%d)\n", __func__, filename, ret);
        return ret;
    }

    /* Get Trident path mapping */
    tag_ptr = bl_find_tag(config_fw->data, config_fw->size,
                            "TRD_PATH_MAPPING=", strlen("00"));
    if (tag_ptr != NULL) {
        u8 val;
        if (sscanf(tag_ptr, "%hhx", &val) == 1)
            config->trd.path_mapping = val;
        else
            config->trd.path_mapping = bl_hw->mod_params->phy_cfg;
    } else
        config->trd.path_mapping = bl_hw->mod_params->phy_cfg;

    BL_DBG("Trident path mapping is: %d\n", config->trd.path_mapping);

    /* Get DC offset compensation */
    tag_ptr = bl_find_tag(config_fw->data, config_fw->size,
                            "TX_DC_OFF_COMP=", strlen("00000000"));
    if (tag_ptr != NULL) {
        if (sscanf(tag_ptr, "%08x", &config->trd.tx_dc_off_comp) != 1)
            config->trd.tx_dc_off_comp = 0;
    } else
        config->trd.tx_dc_off_comp = 0;

    BL_DBG("TX DC offset compensation is: %08X\n", config->trd.tx_dc_off_comp);

    /* Get Karst TX IQ compensation value for path0 on 2.4GHz */
    tag_ptr = bl_find_tag(config_fw->data, config_fw->size,
                            "KARST_TX_IQ_COMP_2_4G_PATH_0=", strlen("00000000"));
    if (tag_ptr != NULL) {
        if (sscanf(tag_ptr, "%08x", &config->karst.tx_iq_comp_2_4G[0]) != 1)
            config->karst.tx_iq_comp_2_4G[0] = 0x01000000;
    } else
        config->karst.tx_iq_comp_2_4G[0] = 0x01000000;

    BL_DBG("Karst TX IQ compensation for path 0 on 2.4GHz is: %08X\n", config->karst.tx_iq_comp_2_4G[0]);

    /* Get Karst TX IQ compensation value for path1 on 2.4GHz */
    tag_ptr = bl_find_tag(config_fw->data, config_fw->size,
                            "KARST_TX_IQ_COMP_2_4G_PATH_1=", strlen("00000000"));
    if (tag_ptr != NULL) {
        if (sscanf(tag_ptr, "%08x", &config->karst.tx_iq_comp_2_4G[1]) != 1)
            config->karst.tx_iq_comp_2_4G[1] = 0x01000000;
    } else
        config->karst.tx_iq_comp_2_4G[1] = 0x01000000;

    BL_DBG("Karst TX IQ compensation for path 1 on 2.4GHz is: %08X\n", config->karst.tx_iq_comp_2_4G[1]);

    /* Get Karst RX IQ compensation value for path0 on 2.4GHz */
    tag_ptr = bl_find_tag(config_fw->data, config_fw->size,
                            "KARST_RX_IQ_COMP_2_4G_PATH_0=", strlen("00000000"));
    if (tag_ptr != NULL) {
        if (sscanf(tag_ptr, "%08x", &config->karst.rx_iq_comp_2_4G[0]) != 1)
            config->karst.rx_iq_comp_2_4G[0] = 0x01000000;
    } else
        config->karst.rx_iq_comp_2_4G[0] = 0x01000000;

    BL_DBG("Karst RX IQ compensation for path 0 on 2.4GHz is: %08X\n", config->karst.rx_iq_comp_2_4G[0]);

    /* Get Karst RX IQ compensation value for path1 on 2.4GHz */
    tag_ptr = bl_find_tag(config_fw->data, config_fw->size,
                            "KARST_RX_IQ_COMP_2_4G_PATH_1=", strlen("00000000"));
    if (tag_ptr != NULL) {
        if (sscanf(tag_ptr, "%08x", &config->karst.rx_iq_comp_2_4G[1]) != 1)
            config->karst.rx_iq_comp_2_4G[1] = 0x01000000;
    } else
        config->karst.rx_iq_comp_2_4G[1] = 0x01000000;

    BL_DBG("Karst RX IQ compensation for path 1 on 2.4GHz is: %08X\n", config->karst.rx_iq_comp_2_4G[1]);

    /* Get Karst TX IQ compensation value for path0 on 5GHz */
    tag_ptr = bl_find_tag(config_fw->data, config_fw->size,
                            "KARST_TX_IQ_COMP_5G_PATH_0=", strlen("00000000"));
    if (tag_ptr != NULL) {
        if (sscanf(tag_ptr, "%08x", &config->karst.tx_iq_comp_5G[0]) != 1)
            config->karst.tx_iq_comp_5G[0] = 0x01000000;
    } else
        config->karst.tx_iq_comp_5G[0] = 0x01000000;

    BL_DBG("Karst TX IQ compensation for path 0 on 5GHz is: %08X\n", config->karst.tx_iq_comp_5G[0]);

    /* Get Karst TX IQ compensation value for path1 on 5GHz */
    tag_ptr = bl_find_tag(config_fw->data, config_fw->size,
                            "KARST_TX_IQ_COMP_5G_PATH_1=", strlen("00000000"));
    if (tag_ptr != NULL) {
        if (sscanf(tag_ptr, "%08x", &config->karst.tx_iq_comp_5G[1]) != 1)
            config->karst.tx_iq_comp_5G[1] = 0x01000000;
    } else
        config->karst.tx_iq_comp_5G[1] = 0x01000000;

    BL_DBG("Karst TX IQ compensation for path 1 on 5GHz is: %08X\n", config->karst.tx_iq_comp_5G[1]);

    /* Get Karst RX IQ compensation value for path0 on 5GHz */
    tag_ptr = bl_find_tag(config_fw->data, config_fw->size,
                            "KARST_RX_IQ_COMP_5G_PATH_0=", strlen("00000000"));
    if (tag_ptr != NULL) {
        if (sscanf(tag_ptr, "%08x", &config->karst.rx_iq_comp_5G[0]) != 1)
            config->karst.rx_iq_comp_5G[0] = 0x01000000;
    } else
        config->karst.rx_iq_comp_5G[0] = 0x01000000;

    BL_DBG("Karst RX IQ compensation for path 0 on 5GHz is: %08X\n", config->karst.rx_iq_comp_5G[0]);

    /* Get Karst RX IQ compensation value for path1 on 5GHz */
    tag_ptr = bl_find_tag(config_fw->data, config_fw->size,
                            "KARST_RX_IQ_COMP_5G_PATH_1=", strlen("00000000"));
    if (tag_ptr != NULL) {
        if (sscanf(tag_ptr, "%08x", &config->karst.rx_iq_comp_5G[1]) != 1)
            config->karst.rx_iq_comp_5G[1] = 0x01000000;
    } else
        config->karst.rx_iq_comp_5G[1] = 0x01000000;

    BL_DBG("Karst RX IQ compensation for path 1 on 5GHz is: %08X\n", config->karst.rx_iq_comp_5G[1]);

    /* Get Karst default path */
    tag_ptr = bl_find_tag(config_fw->data, config_fw->size,
                            "KARST_DEFAULT_PATH=", strlen("00"));
    if (tag_ptr != NULL) {
        u8 val;
        if (sscanf(tag_ptr, "%hhx", &val) == 1)
            config->karst.path_used = val;
        else
            config->karst.path_used = bl_hw->mod_params->phy_cfg;
    } else
        config->karst.path_used = bl_hw->mod_params->phy_cfg;

    BL_DBG("Karst default path is: %d\n", config->karst.path_used);

    /* Release the configuration file */
    release_firmware(config_fw);

    return 0;
}

int bl_find_rftlv(u8_l * buff_addr, u16_l buff_len, u16_l type, u16_l len, u8_l *value)
{
    u8_l * curr_addr = NULL;
    u16_l type_tmp = 0;
    u16_l len_tmp = 0;

    if(!buff_addr || !value || !buff_len || type == RFTLV_TYPE_INVALID) {
        printk("%s input error\n", __func__);
        return -1;
    }

    curr_addr = buff_addr;
    while(1) {
        if(curr_addr - buff_addr >= buff_len - RFTLV_SIZE_TL) {
            printk("type=%d not found\n", type);
            return -1;
        }
        type_tmp = *curr_addr | (*(curr_addr + 1) << 8);
        len_tmp  = *(curr_addr + RFTLV_SIZE_TYPE) | (*(curr_addr + RFTLV_SIZE_TYPE + 1) << 8);
        if(type_tmp == RFTLV_TYPE_INVALID)
            return -1;
        else if(type_tmp != type) {
            curr_addr += (RFTLV_SIZE_TL + len_tmp);
            continue;
        }

        if(len_tmp > len)
            return 0;

        memcpy(value, (curr_addr + RFTLV_SIZE_TL), len);

        return 1;
    }

    return 0;
}

int bl_parse_caldata(u8_l * buff_addr, u16_l buff_len, struct mm_set_rftlv_req * req)
{
    int i = 0;

    /* Skip 8Byte Magic head RFTLF_TYPE_MAGIC_FW_HEAD2 */
    if (buff_addr && (RFTLF_TYPE_MAGIC_FW_HEAD2 == (*((uint64_t *)(buff_addr))))) {
        buff_addr += 8;
        buff_len -= 8;
    } else {
        printk("caldata header is broken, skip\n");
        return -1;
    }

    /* XTAL */
    if(bl_find_rftlv(buff_addr, buff_len, RFTLV_TYPE_XTAL_MODE, RFTLV_MAXLEN_XTAL_MODE, (u8_l *)&req->xtal.xtal_mode) > 0) {
        req->xtal_valid = 1;
        BL_DBG("RFTLV_TYPE_XTAL_MODE OK %c %c\n", req->xtal.xtal_mode[0], req->xtal.xtal_mode[1]);
    }
    if(bl_find_rftlv(buff_addr, buff_len, RFTLV_TYPE_XTAL, RFTLV_MAXLEN_XTAL, (u8_l *)&req->xtal.xtal_cap) > 0) {
        req->xtal_valid = 1;
        BL_DBG("RFTLV_TYPE_XTAL OK %x %x %x %x %x\n", req->xtal.xtal_cap[0], req->xtal.xtal_cap[1], req->xtal.xtal_cap[2], req->xtal.xtal_cap[3], req->xtal.xtal_cap[4]);
    }


    /* TCAL */
    if(bl_find_rftlv(buff_addr, buff_len, RFTLV_TYPE_EN_TCAL, RFTLV_MAXLEN_EN_TCAL, (u8_l *)&req->tcal.en_tcal) > 0) {
        req->tcal_valid = 1;
        BL_DBG("RFTLV_TYPE_EN_TCAL OK\n");
    }

    if(bl_find_rftlv(buff_addr, buff_len, RFTLV_TYPE_LINEAR_OR_FOLLOW, RFTLV_MAXLEN_LINEAR_OR_FOLLOW, (u8_l *)&req->tcal.linear_or_follow) > 0) {
        BL_DBG("RFTLV_TYPE_LINEAR_OR_FOLLOW OK %d\n", req->tcal.linear_or_follow);
    }

    if(bl_find_rftlv(buff_addr, buff_len, RFTLV_TYPE_TCHANNELS, RFTLV_MAXLEN_TCHANNELS, (u8_l *)&req->tcal.Tchannels) > 0) {
        BL_DBG("RFTLV_TYPE_TCHANNELS OK %d %d %d %d %d\n",req->tcal.Tchannels[0], req->tcal.Tchannels[1], req->tcal.Tchannels[2], req->tcal.Tchannels[3], req->tcal.Tchannels[4]);
    }

    if(bl_find_rftlv(buff_addr, buff_len, RFTLV_TYPE_TCHANNEL_OS, RFTLV_MAXLEN_TCHANNEL_OS, (u8_l *)&req->tcal.Tchannel_os) > 0) {
        BL_DBG("RFTLV_TYPE_TCHANNEL_OS OK %d %d %d %d %d\n", req->tcal.Tchannel_os[0],req->tcal.Tchannel_os[1],req->tcal.Tchannel_os[2],req->tcal.Tchannel_os[3],req->tcal.Tchannel_os[4]);
    }

    if(bl_find_rftlv(buff_addr, buff_len, RFTLV_TYPE_TCHANNEL_OS_LOW, RFTLV_MAXLEN_TCHANNEL_OS_LOW, (u8_l *)&req->tcal.Tchannel_os_low) > 0) {
        BL_DBG("RFTLV_TYPE_TCHANNEL_OS_LOW OK %d %d %d %d %d\n", req->tcal.Tchannel_os_low[0], req->tcal.Tchannel_os_low[1], req->tcal.Tchannel_os_low[2], req->tcal.Tchannel_os_low[3], req->tcal.Tchannel_os_low[4]);
    }

    if(bl_find_rftlv(buff_addr, buff_len, RFTLV_TYPE_TROOM_OS, RFTLV_MAXLEN_TROOM_OS, (u8_l *)&req->tcal.Troom_os) > 0) {
        /* negative value is NOT supported. So we use '256' for 0, '255' for -1, '257' for 1,'511' for 256 */
        req->tcal.Troom_os = req->tcal.Troom_os - 256;
        BL_DBG("RFTLV_TYPE_TROOM_OS OK %x\n", req->tcal.Troom_os);
    }

    /* Power cal data */
    if(bl_find_rftlv(buff_addr, buff_len, RFTLV_TYPE_PWR_MODE, RFTLV_MAXLEN_PWR_MODE, (u8_l *)&req->pwr_wifi.pwr_mode) > 0) {
        req->pwr_wifi_valid = 1;
        BL_DBG("RFTLV_TYPE_PWR_MODE OK %c %c\n", req->pwr_wifi.pwr_mode[0],req->pwr_wifi.pwr_mode[1]);
    }
    if(bl_find_rftlv(buff_addr, buff_len, RFTLV_TYPE_PWR_TABLE_11B, RFTLV_MAXLEN_PWR_TABLE_11B, (u8_l *)&req->pwr_wifi.pwr_table_11b) > 0) {
        for(i = 0; i < RFTLV_MAXLEN_PWR_TABLE_11B; i++)
            BL_DBG("%d,", req->pwr_wifi.pwr_table_11b[i]);
        BL_DBG("RFTLV_TYPE_PWR_TABLE_11B OK\n");
    }
    if(bl_find_rftlv(buff_addr, buff_len, RFTLV_TYPE_PWR_TABLE_11G, RFTLV_MAXLEN_PWR_TABLE_11G, (u8_l *)&req->pwr_wifi.pwr_table_11g) > 0) {
        for(i = 0; i < RFTLV_MAXLEN_PWR_TABLE_11G; i++)
            BL_DBG("%d,", req->pwr_wifi.pwr_table_11g[i]);
        BL_DBG("RFTLV_TYPE_PWR_TABLE_11G OK\n");
    }
    if(bl_find_rftlv(buff_addr, buff_len, RFTLV_TYPE_PWR_TABLE_11N, RFTLV_MAXLEN_PWR_TABLE_11N, (u8_l *)&req->pwr_wifi.pwr_table_11n) > 0) {
        for(i = 0; i < RFTLV_TYPE_PWR_TABLE_11N; i++)
            BL_DBG("%d,", req->pwr_wifi.pwr_table_11n[i]);
        BL_DBG("RFTLV_TYPE_PWR_TABLE_11N OK\n");
    }
    if(bl_find_rftlv(buff_addr, buff_len, RFTLV_TYPE_PWR_OFFSET, RFTLV_MAXLEN_PWR_OFFSET, (u8_l *)&req->pwr_wifi.pwr_offset) > 0) {
        /* In DTS file, use 10 instead of real 0 */
        for(i = 0; i < RFTLV_MAXLEN_PWR_OFFSET; i++) {
            req->pwr_wifi.pwr_offset[i] -= 10;
        //    req->pwr_wifi.pwr_offset[i] = req->pwr_wifi.pwr_offset[i] * 4;
            BL_DBG("%d,", req->pwr_wifi.pwr_offset[i]);
        }
        BL_DBG("RFTLV_TYPE_PWR_OFFSET OK\n");
    }
    return 0;
}


int bl_parse_mac_config(char *file_path, struct bl_conf_file *config) 
{
    int ret = 0;
	u8 *addr = config->mac_addr;
    u8 *cfg_buf;
    u16 cfg_buf_len = 64;

    cfg_buf = kzalloc(cfg_buf_len, GFP_KERNEL);
    if (cfg_buf == NULL) {
        printk("%s, fail to alloc cfg_buf\n", __func__);
        return -ENOMEM;
    }
    memset(cfg_buf, 0, cfg_buf_len);
    
    ret = bl_read_file(file_path, cfg_buf, cfg_buf_len-1);
    if (ret < 0) {
        printk("%s, read file error, ret:%d\n", __func__, ret);
        kfree(cfg_buf);
        return ret;
    }
    cfg_buf[ret] = '\0';

    printk("%s, cfg file:%s\n", __func__, cfg_buf);

	if (sscanf(cfg_buf,
			   "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
			   addr + 0, addr + 1, addr + 2,
			   addr + 3, addr + 4, addr + 5) != ETH_ALEN) {
			   printk("wrong input mac %s \n", cfg_buf);
              ret = -1;
	}
	else
		printk("wifi mac %x :%x :%x: %x :%x :%x \n", addr[0], addr[1], addr[2],addr[3], addr[4],addr[5]);
			   

    if (cfg_buf != NULL)
        kfree(cfg_buf);

    return ret;
}

