/**
 ******************************************************************************
 *
 *  @file bl_main.c
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
#include "bl_version.h"
#include "bl_iwpriv.h"
#include "bl_nl_events.h"


#define RW_DRV_DESCRIPTION  "BouffaloLab 11n driver for Linux cfg80211"
#define RW_DRV_COPYRIGHT    "Copyright(c) 2017-2024 BouffaloLab"
#define RW_DRV_AUTHOR       "BouffaloLab S.A.S"

#define BL_PRINT_CFM_ERR(req) \
        printk(KERN_CRIT "%s: Status Error(%d)\n", #req, (&req##_cfm)->status)

#define BL_HT_CAPABILITIES                                    \
{                                                               \
    .ht_supported   = true,                                     \
    .cap            = 0,                                        \
    .ampdu_factor   = IEEE80211_HT_MAX_AMPDU_64K,               \
    .ampdu_density  = IEEE80211_HT_MPDU_DENSITY_16,             \
    .mcs        = {                                             \
        .rx_mask = { 0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, },        \
        .rx_highest = cpu_to_le16(65),                          \
        .tx_params = IEEE80211_HT_MCS_TX_DEFINED,               \
    },                                                          \
}

#define BL_VHT_CAPABILITIES                                   \
{                                                               \
    .vht_supported = false,                                     \
    .cap       =                                                \
      (7 << IEEE80211_VHT_CAP_MAX_A_MPDU_LENGTH_EXPONENT_SHIFT),\
    .vht_mcs       = {                                          \
        .rx_mcs_map = cpu_to_le16(                              \
                      IEEE80211_VHT_MCS_SUPPORT_0_9    << 0  |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 2  |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 4  |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 6  |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 8  |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 10 |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 12 |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 14),  \
        .tx_mcs_map = cpu_to_le16(                              \
                      IEEE80211_VHT_MCS_SUPPORT_0_9    << 0  |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 2  |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 4  |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 6  |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 8  |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 10 |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 12 |  \
                      IEEE80211_VHT_MCS_NOT_SUPPORTED  << 14),  \
    }                                                           \
}

#define RATE(_bitrate, _hw_rate, _flags) {      \
    .bitrate    = (_bitrate),                   \
    .flags      = (_flags),                     \
    .hw_value   = (_hw_rate),                   \
}

#define CHAN(_freq) {                           \
    .center_freq    = (_freq),                  \
    .max_power  = 30, /* FIXME */               \
}

void *kbuff = NULL;
extern struct device_attribute dev_attr_filter_severity;
extern struct device_attribute dev_attr_filter_module;
#ifdef  BL_RX_DEFRAG
extern struct rxdefrag_list rx_defrag_pool[BL_RX_DEFRAG_POOL_SIZE];
#endif

static struct ieee80211_rate bl_ratetable[] = {
    RATE(10,  0x00, 0),
    RATE(20,  0x01, IEEE80211_RATE_SHORT_PREAMBLE),
    RATE(55,  0x02, IEEE80211_RATE_SHORT_PREAMBLE),
    RATE(110, 0x03, IEEE80211_RATE_SHORT_PREAMBLE),
    RATE(60,  0x04, 0),
    RATE(90,  0x05, 0),
    RATE(120, 0x06, 0),
    RATE(180, 0x07, 0),
    RATE(240, 0x08, 0),
    RATE(360, 0x09, 0),
    RATE(480, 0x0A, 0),
    RATE(540, 0x0B, 0),
};

/* The channels indexes here are not used anymore */
static struct ieee80211_channel bl_2ghz_channels[] = {
    CHAN(2412),
    CHAN(2417),
    CHAN(2422),
    CHAN(2427),
    CHAN(2432),
    CHAN(2437),
    CHAN(2442),
    CHAN(2447),
    CHAN(2452),
    CHAN(2457),
    CHAN(2462),
    CHAN(2467),
    CHAN(2472),
    CHAN(2484),
};

static struct ieee80211_supported_band bl_band_2GHz = {
    .channels   = bl_2ghz_channels,
    .n_channels = ARRAY_SIZE(bl_2ghz_channels),
    .bitrates   = bl_ratetable,
    .n_bitrates = ARRAY_SIZE(bl_ratetable),
    .ht_cap     = BL_HT_CAPABILITIES,
};

static struct ieee80211_iface_limit bl_limits[] = {
    { .max = NX_VIRT_DEV_MAX, .types = BIT(NL80211_IFTYPE_AP) |
                                       BIT(NL80211_IFTYPE_STATION) |
                                       BIT(NL80211_IFTYPE_P2P_GO) |
                                       BIT(NL80211_IFTYPE_P2P_CLIENT)}
};

static struct ieee80211_iface_limit bl_limits_dfs[] = {
    { .max = NX_VIRT_DEV_MAX, .types = BIT(NL80211_IFTYPE_AP)}
};

static const struct ieee80211_iface_combination bl_combinations[] = {
    {
        .limits                 = bl_limits,
        .n_limits               = ARRAY_SIZE(bl_limits),
        .num_different_channels = NX_CHAN_CTXT_CNT,
        .max_interfaces         = NX_VIRT_DEV_MAX,
    },
    /* Keep this combination as the last one */
    {
        .limits                 = bl_limits_dfs,
        .n_limits               = ARRAY_SIZE(bl_limits_dfs),
        .num_different_channels = 1,
        .max_interfaces         = NX_VIRT_DEV_MAX,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0)
        .radar_detect_widths = (BIT(NL80211_CHAN_WIDTH_20_NOHT) |
                                BIT(NL80211_CHAN_WIDTH_20) |
                                BIT(NL80211_CHAN_WIDTH_40) |
                                BIT(NL80211_CHAN_WIDTH_80)),
#endif
    }
};

/* There isn't a lot of sense in it, but you can transmit anything you like */
static struct ieee80211_txrx_stypes
bl_default_mgmt_stypes[NUM_NL80211_IFTYPES] = {
    [NL80211_IFTYPE_STATION] = {
        .tx = 0xffff,
        .rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
               BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
               BIT(IEEE80211_STYPE_AUTH >> 4),
    },
    [NL80211_IFTYPE_AP] = {
        .tx = 0xffff,
        .rx = BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
            BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
            BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
            BIT(IEEE80211_STYPE_DISASSOC >> 4) |
            BIT(IEEE80211_STYPE_AUTH >> 4) |
            BIT(IEEE80211_STYPE_DEAUTH >> 4) |
            BIT(IEEE80211_STYPE_ACTION >> 4),
    },
    [NL80211_IFTYPE_AP_VLAN] = {
        /* copy AP */
        .tx = 0xffff,
        .rx = BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
            BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
            BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
            BIT(IEEE80211_STYPE_DISASSOC >> 4) |
            BIT(IEEE80211_STYPE_AUTH >> 4) |
            BIT(IEEE80211_STYPE_DEAUTH >> 4) |
            BIT(IEEE80211_STYPE_ACTION >> 4),
    },
    [NL80211_IFTYPE_P2P_CLIENT] = {
        .tx = 0xffff,
        .rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
            BIT(IEEE80211_STYPE_PROBE_REQ >> 4),
    },
    [NL80211_IFTYPE_P2P_GO] = {
        .tx = 0xffff,
        .rx = BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
            BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
            BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
            BIT(IEEE80211_STYPE_DISASSOC >> 4) |
            BIT(IEEE80211_STYPE_AUTH >> 4) |
            BIT(IEEE80211_STYPE_DEAUTH >> 4) |
            BIT(IEEE80211_STYPE_ACTION >> 4),
    },
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
    [NL80211_IFTYPE_P2P_DEVICE] = {
        .tx = 0xffff,
        .rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
            BIT(IEEE80211_STYPE_PROBE_REQ >> 4),
    },
#endif
};


static u32 cipher_suites[] = {
    WLAN_CIPHER_SUITE_WEP40,
    WLAN_CIPHER_SUITE_WEP104,
    WLAN_CIPHER_SUITE_TKIP,
    WLAN_CIPHER_SUITE_CCMP,
    0, // reserved entries to enable AES-CMAC and/or SMS4
    0,
};
#define NB_RESERVED_CIPHER 2;

static const int bl_ac2hwq[1][NL80211_NUM_ACS] = {
    {
        [NL80211_TXQ_Q_VO] = BL_HWQ_VO,
        [NL80211_TXQ_Q_VI] = BL_HWQ_VI,
        [NL80211_TXQ_Q_BE] = BL_HWQ_BE,
        [NL80211_TXQ_Q_BK] = BL_HWQ_BK
    }
};

const int bl_tid2hwq[IEEE80211_NUM_TIDS] = {
    BL_HWQ_BE,
    BL_HWQ_BK,
    BL_HWQ_BK,
    BL_HWQ_BE,
    BL_HWQ_VI,
    BL_HWQ_VI,
    BL_HWQ_VO,
    BL_HWQ_VO,
    /* TID_8 is used for management frames */
    BL_HWQ_VO,
    /* At the moment, all others TID are mapped to BE */
    BL_HWQ_BE,
    BL_HWQ_BE,
    BL_HWQ_BE,
    BL_HWQ_BE,
    BL_HWQ_BE,
    BL_HWQ_BE,
    BL_HWQ_BE,
};

static const int bl_hwq2uapsd[NL80211_NUM_ACS] = {
    [BL_HWQ_VO] = IEEE80211_WMM_IE_STA_QOSINFO_AC_VO,
    [BL_HWQ_VI] = IEEE80211_WMM_IE_STA_QOSINFO_AC_VI,
    [BL_HWQ_BE] = IEEE80211_WMM_IE_STA_QOSINFO_AC_BE,
    [BL_HWQ_BK] = IEEE80211_WMM_IE_STA_QOSINFO_AC_BK,
};

/*********************************************************************
 * helper
 *********************************************************************/ 
static u32_l bl_nl_bcst_seq = 0;
void bl_nl_broadcast_event(struct bl_hw *bl_hw, u32 event_id, u8* payload, u16 len)
{
    struct sk_buff *skb = NULL;
    struct nlmsghdr *nl_hdr = NULL;
    struct sock *sk = bl_hw->netlink_sock;
    struct bl_nl_event nl_event;
    int ret = 0, i = 0;

    if (sk) {
        skb = dev_alloc_skb(NLMSG_SPACE(BL_NL_BUF_MAX_LEN));
        if (!skb) {
            printk("ERR: no memory for nl msg\n");
            return;
        }

        memset(skb->data, 0, NLMSG_SPACE(BL_NL_BUF_MAX_LEN));

        nl_hdr = (struct nlmsghdr *)skb->data;
        nl_hdr->nlmsg_len = NLMSG_SPACE(len + sizeof(nl_event));
        nl_hdr->nlmsg_pid = 0;
        nl_hdr->nlmsg_flags = 0;
        nl_hdr->nlmsg_seq = bl_nl_bcst_seq++;

        BL_DBG("event_id=%d, nlmsg_len=%d seq=%d len=%d\n", 
                event_id, nl_hdr->nlmsg_len, nl_hdr->nlmsg_seq, len);

        skb_put(skb, nl_hdr->nlmsg_len);
        nl_event.event_id = event_id;
        nl_event.payload_len = len;
        memcpy(NLMSG_DATA(nl_hdr), &nl_event, sizeof(nl_event));
        memcpy((u8_l *)NLMSG_DATA(nl_hdr) + sizeof(nl_event), payload, len);


        NETLINK_CB(skb).portid = 0;
        NETLINK_CB(skb).dst_group = BL_NL_BCAST_GROUP_ID;

        ret = netlink_broadcast(sk, skb, 0, BL_NL_BCAST_GROUP_ID, GFP_ATOMIC);
        if (ret) {
            BL_DBG("WARN: broadcast fail ret = %d\n", ret);
        }
        BL_DBG("Finish event 0x%x broadcast\n", event_id);
    } else {
        BL_DBG("WARN: No available netlink socket\n");
    }
}

struct bl_sta *bl_get_sta(struct bl_hw *bl_hw, const u8 *mac_addr)
{
    int i;

    for (i = 0; i < NX_REMOTE_STA_MAX; i++) {
        struct bl_sta *sta = &bl_hw->sta_table[i];
        if (sta->valid && (memcmp(mac_addr, &sta->mac_addr, 6) == 0))
            return sta;
    }

    return NULL;
}

void bl_enable_wapi(struct bl_hw *bl_hw)
{
    cipher_suites[bl_hw->wiphy->n_cipher_suites] = WLAN_CIPHER_SUITE_SMS4;
    bl_hw->wiphy->n_cipher_suites ++;
    bl_hw->wiphy->flags |= WIPHY_FLAG_CONTROL_PORT_PROTOCOL;
}

void bl_enable_mfp(struct bl_hw *bl_hw)
{
    cipher_suites[bl_hw->wiphy->n_cipher_suites] = WLAN_CIPHER_SUITE_AES_CMAC;
    bl_hw->wiphy->n_cipher_suites ++;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
u8 *bl_build_bcn(struct bl_bcn *bcn, struct cfg80211_beacon_data *new)
#else
u8 *bl_build_bcn(struct bl_bcn *bcn, struct beacon_parameters *new)
#endif
{
    u8 *buf, *pos;

    if (new->head) {
        u8 *head = kmalloc(new->head_len, GFP_KERNEL);

        if (!head)
            return NULL;

        if (bcn->head)
            kfree(bcn->head);

        bcn->head = head;
        bcn->head_len = new->head_len;
        memcpy(bcn->head, new->head, new->head_len);
    }
    if (new->tail) {
        u8 *tail = kmalloc(new->tail_len, GFP_KERNEL);

        if (!tail)
            return NULL;

        if (bcn->tail)
            kfree(bcn->tail);

        bcn->tail = tail;
        bcn->tail_len = new->tail_len;
        memcpy(bcn->tail, new->tail, new->tail_len);
    }

    if (!bcn->head)
        return NULL;

    bcn->tim_len = 6;
    bcn->len = bcn->head_len + bcn->tail_len + bcn->ies_len + bcn->tim_len;

    buf = kmalloc(bcn->len, GFP_KERNEL);
    if (!buf)
        return NULL;

    // Build the beacon buffer
    pos = buf;
    memcpy(pos, bcn->head, bcn->head_len);
    pos += bcn->head_len;
    *pos++ = WLAN_EID_TIM;
    *pos++ = 4;
    *pos++ = 0;
    *pos++ = bcn->dtim;
    *pos++ = 0;
    *pos++ = 0;
    if (bcn->tail) {
        memcpy(pos, bcn->tail, bcn->tail_len);
        pos += bcn->tail_len;
    }
    if (bcn->ies) {
        memcpy(pos, bcn->ies, bcn->ies_len);
    }

    return buf;
}


static void bl_del_bcn(struct bl_bcn *bcn)
{
    if (bcn->head) {
        kfree(bcn->head);
        bcn->head = NULL;
    }
    bcn->head_len = 0;

    if (bcn->tail) {
        kfree(bcn->tail);
        bcn->tail = NULL;
    }
    bcn->tail_len = 0;

    if (bcn->ies) {
        kfree(bcn->ies);
        bcn->ies = NULL;
    }
    bcn->ies_len = 0;
    bcn->tim_len = 0;
    bcn->dtim = 0;
    bcn->len = 0;
}

/**
 * Link channel ctxt to a vif and thus increments count for this context.
 */
void bl_chanctx_link(struct bl_vif *vif, u8 ch_idx, void *chandef_param)
{
    struct bl_chanctx *ctxt;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
    struct cfg80211_chan_def *chandef = (struct cfg80211_chan_def *)chandef_param;
#endif

    if (ch_idx >= NX_CHAN_CTXT_CNT) {
        WARN(1, "Invalid channel ctxt id %d", ch_idx);
        return;
    }

    vif->ch_index = ch_idx;
    ctxt = &vif->bl_hw->chanctx_table[ch_idx];
    ctxt->count++;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
    // For now chandef is NULL for STATION interface
    if (chandef) {
        if (!ctxt->chan_def.chan)
            ctxt->chan_def = *chandef;
        else {
            // TODO. check that chandef is the same as the one already
            // set for this ctxt
        }
    }
#endif
}

/**
 * Unlink channel ctxt from a vif and thus decrements count for this context
 */
void bl_chanctx_unlink(struct bl_vif *vif)
{
    struct bl_chanctx *ctxt;

    if (vif->ch_index == BL_CH_NOT_SET)
        return;

    ctxt = &vif->bl_hw->chanctx_table[vif->ch_index];

    if (ctxt->count == 0) {
        WARN(1, "Chan ctxt ref count is already 0");
    } else {
        ctxt->count--;
    }

    if (ctxt->count == 0) {
        /* set chan to null, so that if this ctxt is relinked to a vif that
           don't have channel information, don't use wrong information */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
        ctxt->chan_def.chan = NULL;
#endif
    }
    vif->ch_index = BL_CH_NOT_SET;
}

int bl_chanctx_valid(struct bl_hw *bl_hw, u8 ch_idx)
{
    if (ch_idx >= NX_CHAN_CTXT_CNT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
         || bl_hw->chanctx_table[ch_idx].chan_def.chan == NULL
#endif
        ) {
        return 0;
    }

    return 1;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 12, 0)
static void bl_del_csa(struct bl_vif *vif)
{
    struct bl_csa *csa = vif->ap.csa;

    if (!csa)
        return;

    if (csa->dma.buf) {
        kfree(csa->dma.buf);
    }
    bl_del_bcn(&csa->bcn);
    kfree(csa);
    vif->ap.csa = NULL;
}

static void bl_csa_finish(struct work_struct *ws)
{
    struct bl_csa *csa = container_of(ws, struct bl_csa, work);
    struct bl_vif *vif = csa->vif;
    struct bl_hw *bl_hw = vif->bl_hw;
    int error = csa->status;

    if (!error)
        error = bl_send_bcn_change(bl_hw, vif->vif_index, csa->dma.buf,
                                     csa->bcn.len, csa->bcn.head_len,
                                     csa->bcn.tim_len, NULL);

    if (error)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0)
        cfg80211_stop_iface(bl_hw->wiphy, &vif->wdev, GFP_KERNEL);
#else
        printk("%s change beacon fail \n", __func__);//todo: fail case
#endif
    else {
        bl_chanctx_unlink(vif);
        bl_chanctx_link(vif, csa->ch_idx, &csa->chandef);
        spin_lock_bh(&bl_hw->cb_lock);
        if (bl_hw->cur_chanctx == csa->ch_idx) {
            bl_txq_vif_start(vif, BL_TXQ_STOP_CHAN, bl_hw);
        } else
            bl_txq_vif_stop(vif, BL_TXQ_STOP_CHAN, bl_hw);
        spin_unlock_bh(&bl_hw->cb_lock);
        cfg80211_ch_switch_notify(vif->ndev, &csa->chandef);
    }
    bl_del_csa(vif);
}
#endif

/**
 * bl_external_auth_enable - Enable external authentication on a vif
 *
 * @vif: VIF on which external authentication must be enabled
 *
 * External authentication requires to start TXQ for unknown STA in
 * order to send auth frame pusehd by user space.
 * Note: It is assumed that fw is on the correct channel.
 */
void bl_external_auth_enable(struct bl_vif *vif)
{
    vif->sta.flags |= RWNX_STA_EXT_AUTH;
    bl_txq_unk_vif_init(vif);
    bl_txq_start(bl_txq_vif_get(vif, NX_UNK_TXQ_TYPE, NULL), 0);
}
/**
 * bl_external_auth_disable - Disable external authentication on a vif
 *
 * @vif: VIF on which external authentication must be disabled
 */
void bl_external_auth_disable(struct bl_vif *vif)
{
    if (!(vif->sta.flags & RWNX_STA_EXT_AUTH))
        return;

    vif->sta.flags &= ~RWNX_STA_EXT_AUTH;
    bl_txq_unk_vif_deinit(vif);
}

/*********************************************************************
 * netdev callbacks
 ********************************************************************/
/**
 * int (*ndo_open)(struct net_device *dev);
 *     This function is called when network device transistions to the up
 *     state.
 *
 * - Start FW if this is the first interface opened
 * - Add interface at fw level
 */
static int bl_open(struct net_device *dev)
{
    struct bl_vif *bl_vif = netdev_priv(dev);
    struct bl_hw *bl_hw = bl_vif->bl_hw;
    struct mm_add_if_cfm add_if_cfm;
    int error = 0;

    BL_DBG(BL_FN_ENTRY_STR);
	
    if (bl_mod_params.mp_mode)
        goto next;


    // Check if it is the first opened VIF
    if (bl_hw->vif_started == 0)
    {
       bl_send_caldata(bl_hw);
        // Start the FW
       if ((error = bl_send_start(bl_hw)))
           return error;

       /* Device is now started */
       set_bit(BL_DEV_STARTED, &bl_hw->drv_flags);
    }

    if (BL_VIF_TYPE(bl_vif) == NL80211_IFTYPE_AP_VLAN) {
        /* For AP_vlan use same fw and drv indexes. We ensure that this index
           will not be used by fw for another vif by taking index >= NX_VIRT_DEV_MAX */
        add_if_cfm.inst_nbr = bl_vif->drv_vif_index;
        netif_tx_stop_all_queues(dev);
    } else {
        /* Forward the information to the LMAC,
         *     p2p value not used in FMAC configuration, iftype is sufficient */
        if ((error = bl_send_add_if(bl_hw, dev->dev_addr,
                                      BL_VIF_TYPE(bl_vif), false, &add_if_cfm)))
            return error;

        if (add_if_cfm.status != 0) {
            BL_PRINT_CFM_ERR(add_if);
            return -EIO;
        }
    }


next:
    /* Save the index retrieved from LMAC */
    bl_vif->vif_index = add_if_cfm.inst_nbr;
    bl_vif->up = true;
    bl_hw->vif_started++;
    bl_hw->vif_table[add_if_cfm.inst_nbr] = bl_vif;

    netif_carrier_off(dev);

    return error;
}

/**
 * int (*ndo_stop)(struct net_device *dev);
 *     This function is called when network device transistions to the down
 *     state.
 *
 * - Remove interface at fw level
 * - Reset FW if this is the last interface opened
 */
static int bl_close(struct net_device *dev)
{
    struct bl_vif *bl_vif = netdev_priv(dev);
    struct bl_hw *bl_hw = bl_vif->bl_hw;

    BL_DBG(BL_FN_ENTRY_STR);

    netdev_info(dev, "CLOSE");
	
    if (bl_mod_params.mp_mode)
        return 0;
	
    bl_hw->vif_started--;
    bl_vif->up = false;

    /* Abort scan request on the vif */
    if (bl_hw->scan_request &&
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0)
        bl_hw->scan_request->wdev == &bl_vif->wdev) {
#else
        bl_hw->scan_request->dev->ieee80211_ptr == &bl_vif->wdev) {
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
        struct cfg80211_scan_info info = {
            .aborted = true,
        };

        cfg80211_scan_done(bl_hw->scan_request, &info);
#else
        cfg80211_scan_done(bl_hw->scan_request, true);
#endif
        bl_hw->scan_request = NULL;
    }

    /* Ensure that we won't process disconnect ind */
    spin_lock_bh(&bl_hw->cb_lock);
    if (netif_carrier_ok(dev)) {
        if (BL_VIF_TYPE(bl_vif) == NL80211_IFTYPE_STATION ||
            BL_VIF_TYPE(bl_vif) == NL80211_IFTYPE_P2P_CLIENT) {
		//	if (bl_vif->connected)
		//	bl_send_sm_disconnect_req(bl_hw, bl_vif, WLAN_REASON_DEAUTH_LEAVING);
            cfg80211_disconnected(dev, WLAN_REASON_DEAUTH_LEAVING,
                                  NULL, 0, true, GFP_ATOMIC);
            netif_tx_stop_all_queues(dev);
            netif_carrier_off(dev);
        } else if (BL_VIF_TYPE(bl_vif) == NL80211_IFTYPE_AP_VLAN) {
            netif_carrier_off(dev);
        } else {
            netdev_warn(dev, "AP not stopped when disabling interface");
        }
    }
   // bl_hw->vif_table[bl_vif->vif_index] = NULL;
    spin_unlock_bh(&bl_hw->cb_lock);

    msleep(200);
    bl_send_remove_if(bl_hw, bl_vif->vif_index);

    if (bl_hw->vif_started == 0) {
        /* This also lets both ipc sides remain in sync before resetting */
        bl_ipc_tx_drain(bl_hw);

        bl_send_reset(bl_hw);

        // Set parameters to firmware
        bl_send_me_config_req(bl_hw);

        // Set channel parameters to firmware
        bl_send_me_chan_config_req(bl_hw);

        clear_bit(BL_DEV_STARTED, &bl_hw->drv_flags);
    }

    if (bl_hw->roc_elem && (bl_hw->roc_elem->wdev == &bl_vif->wdev)) {		
        kfree(bl_hw->roc_elem);
        /* Initialize RoC element pointer to NULL, indicate that RoC can be started */
        bl_hw->roc_elem = NULL;
    }

    return 0;
}

/**
 * struct net_device_stats* (*ndo_get_stats)(struct net_device *dev);
 *	Called when a user wants to get the network device usage
 *	statistics. Drivers must do one of the following:
 *	1. Define @ndo_get_stats64 to fill in a zero-initialised
 *	   rtnl_link_stats64 structure passed by the caller.
 *	2. Define @ndo_get_stats to update a net_device_stats structure
 *	   (which should normally be dev->stats) and return a pointer to
 *	   it. The structure may be changed asynchronously only if each
 *	   field is written atomically.
 *	3. Update dev->stats asynchronously and atomically, and define
 *	   neither operation.
 */
static struct net_device_stats *bl_get_stats(struct net_device *dev)
{
    struct bl_vif *vif = netdev_priv(dev);

    return &vif->net_stats;
}

static const u16 bl_1d_to_wmm_queue[8] = { 1, 0, 0, 1, 2, 2, 3, 3 };
/**
 * u16 (*ndo_select_queue)(struct net_device *dev, struct sk_buff *skb,
 *                         void *accel_priv, select_queue_fallback_t fallback);
 *	Called to decide which queue to when device supports multiple
 *	transmit queues.
 */
u16 bl_select_queue(struct net_device *dev, struct sk_buff *skb,
                    struct net_device *sb_dev)
{
    struct bl_vif *bl_vif = netdev_priv(dev);
    struct bl_hw *bl_hw = bl_vif->bl_hw;
    struct wireless_dev *wdev = &bl_vif->wdev;
    struct bl_sta *sta = NULL;
    struct bl_txq *txq;
    u16 netdev_queue;

	BL_DBG(BL_FN_ENTRY_STR);
#define PRIO_STA_NULL 0xAA

    switch (wdev->iftype) {
    case NL80211_IFTYPE_STATION:
    case NL80211_IFTYPE_P2P_CLIENT:
    {
        struct ethhdr *eth;
        eth = (struct ethhdr *)skb->data;
        sta = bl_vif->sta.ap;
        break;
    }
    case NL80211_IFTYPE_AP_VLAN:
        if (bl_vif->ap_vlan.sta_4a) {
            sta = bl_vif->ap_vlan.sta_4a;
            break;
        }

        /* AP_VLAN interface is not used for a 4A STA,
           fallback searching sta amongs all AP's clients */
        bl_vif = bl_vif->ap_vlan.master;
		/* fall through */
    case NL80211_IFTYPE_AP:
    case NL80211_IFTYPE_P2P_GO:
    {
        struct bl_sta *cur;
        struct ethhdr *eth = (struct ethhdr *)skb->data;

        if (is_multicast_ether_addr(eth->h_dest)) {
            sta = &bl_hw->sta_table[bl_vif->ap.bcmc_index];
        } else {
            list_for_each_entry(cur, &bl_vif->ap.sta_list, list) {
                if (!memcmp(cur->mac_addr, eth->h_dest, ETH_ALEN)) {
                    sta = cur;
                    break;
                }
            }
        }

        break;
    }
    default:
        break;
    }

    if (sta && sta->qos)
    {
        /* use the data classifier to determine what 802.1d tag the data frame has */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 0)
        skb->priority = cfg80211_classify8021d(skb, NULL) & IEEE80211_QOS_CTL_TAG1D_MASK;
#else
        skb->priority = cfg80211_classify8021d(skb) & IEEE80211_QOS_CTL_TAG1D_MASK;
#endif
        if (sta->acm)
            bl_downgrade_ac(sta, skb);

        txq = bl_txq_sta_get(sta, skb->priority, NULL, bl_hw);
        netdev_queue = txq->ndev_idx;
    }
    else if (sta)
    {
        skb->priority = 0xFF;
        txq = bl_txq_sta_get(sta, 0, NULL, bl_hw);
        netdev_queue = txq->ndev_idx;
    }
    else
    {
        /* This packet will be dropped in xmit function, still need to select
           an active queue for xmit to be called. As it most likely to happen
           for AP interface, select BCMC queue
           (TODO: select another queue if BCMC queue is stopped) */
        skb->priority = PRIO_STA_NULL;
        netdev_queue = NX_BCMC_TXQ_NDEV_IDX;
    }

    if(skb->priority < 8)
        return bl_1d_to_wmm_queue[skb->priority];
	else
        return 0;

    //BUG_ON(netdev_queue >= NX_NB_NDEV_TXQ);
    //return netdev_queue;
}

/**
 * int (*ndo_set_mac_address)(struct net_device *dev, void *addr);
 *	This function  is called when the Media Access Control address
 *	needs to be changed. If this interface is not defined, the
 *	mac address can not be changed.
 */
static int bl_set_mac_address(struct net_device *dev, void *addr)
{
    struct sockaddr *sa = addr;
    int ret;

    ret = eth_mac_addr(dev, sa);

    return ret;
}

/* 0x8B00 - 0x8BDF */
static const iw_handler bl_iw_std_hdl_array[] = {
    (iw_handler)NULL,       /* SIOCSIWCOMMIT */
};

/* 0x8BE0 - 0x8BFF */
static const iw_handler bl_iw_priv_hdl_array[] = {
    [BL_IOCTL_VERSION - SIOCIWFIRSTPRIV] =      (iw_handler)bl_iwpriv_ver_hdl,
    [BL_IOCTL_WMMCFG  - SIOCIWFIRSTPRIV] =      (iw_handler)bl_iwpriv_wmmcfg_hdl,    
    [BL_IOCTL_DBG     - SIOCIWFIRSTPRIV] =      (iw_handler)bl_iwpriv_dbg_hdl,
    [BL_IOCTL_TEMP    - SIOCIWFIRSTPRIV] =      (iw_handler)bl_iwpriv_temp_read_hdl,
    [BL_IOCTL_SETRATE - SIOCIWFIRSTPRIV] =      (iw_handler)bl_iwpriv_set_rate_hdl,
    [BL_IOCTL_RW_MEM-SIOCIWFIRSTPRIV] =         (iw_handler)bl_iwpriv_rw_mem_hdl,

#ifdef CONFIG_BL_MP
    [BL_IOCTL_MP_CFG  - SIOCIWFIRSTPRIV] =      (iw_handler)bl_iwpriv_mp_load_caldata_hdl,
    [BL_IOCTL_MP_HELP - SIOCIWFIRSTPRIV] =      (iw_handler)bl_iwpriv_help_hdl,
    [BL_IOCTL_MP_MS   - SIOCIWFIRSTPRIV] =      (iw_handler)bl_iwpriv_mp_ms_hdl,
    [BL_IOCTL_MP_MG   - SIOCIWFIRSTPRIV] =      (iw_handler)bl_iwpriv_mp_mg_hdl,
    [BL_IOCTL_MP_IND  - SIOCIWFIRSTPRIV] =      (iw_handler)bl_iwpriv_mp_ind_hdl,
#endif

    [BL_IOCTL_SET_CMD_TYPE_INT  - SIOCIWFIRSTPRIV] =      (iw_handler)bl_iwpriv_cmd_set_type_int,
    [BL_IOCTL_GET_CMD_TYPE_INT  - SIOCIWFIRSTPRIV] =      (iw_handler)bl_iwpriv_cmd_get_type_int,
};

static const struct iw_priv_args bl_iw_priv_args[] = {
    {BL_IOCTL_VERSION,    IW_PRIV_TYPE_CHAR | 128,  IW_PRIV_TYPE_CHAR | 128,                         "version" },
    {BL_IOCTL_WMMCFG,     IW_PRIV_TYPE_INT | 128,   IW_PRIV_TYPE_INT | IW_PRIV_SIZE_MASK,            "wmmcfg" },    
    {BL_IOCTL_DBG,        IW_PRIV_TYPE_INT | 128,   IW_PRIV_TYPE_INT | IW_PRIV_SIZE_MASK,            "dbg" },
    {BL_IOCTL_TEMP,       IW_PRIV_TYPE_INT | 128,   IW_PRIV_TYPE_INT | IW_PRIV_SIZE_MASK,            "temp" },
    {BL_IOCTL_SETRATE,    IW_PRIV_TYPE_BYTE | IW_PRIV_SIZE_MASK|3,0,                                 "setrate" },
    {BL_IOCTL_RW_MEM,     IW_PRIV_TYPE_INT | 512,   IW_PRIV_TYPE_INT | IW_PRIV_SIZE_MASK,            "rw_mem" },

#ifdef CONFIG_BL_MP
    {BL_IOCTL_MP_CFG,     IW_PRIV_TYPE_CHAR|IW_PRIV_SIZE_MASK, IW_PRIV_TYPE_CHAR| IW_PRIV_SIZE_MASK, "mp_load_caldata" },
    {BL_IOCTL_MP_HELP,    IW_PRIV_TYPE_CHAR|IW_PRIV_SIZE_MASK, IW_PRIV_TYPE_CHAR|IW_PRIV_SIZE_MASK,  "help" },
    
    {BL_IOCTL_MP_MS ,               IW_PRIV_TYPE_INT | 128,  IW_PRIV_TYPE_INT | IW_PRIV_SIZE_MASK,   "" },
    {BL_IOCTL_MP_UNICAST_TX,        IW_PRIV_TYPE_INT | 128,  IW_PRIV_TYPE_INT | IW_PRIV_SIZE_MASK,   "mp_uc_tx" },
    {BL_IOCTL_MP_11n_RATE,          IW_PRIV_TYPE_INT | 128,  IW_PRIV_TYPE_INT | IW_PRIV_SIZE_MASK,   "mp_11n_rate" },
    {BL_IOCTL_MP_11g_RATE,          IW_PRIV_TYPE_INT | 128,  IW_PRIV_TYPE_INT | IW_PRIV_SIZE_MASK,   "mp_11g_rate" },
    {BL_IOCTL_MP_11b_RATE,          IW_PRIV_TYPE_INT | 128,  IW_PRIV_TYPE_INT | IW_PRIV_SIZE_MASK,   "mp_11b_rate" },
    {BL_IOCTL_MP_TX,                IW_PRIV_TYPE_INT | 128,  IW_PRIV_TYPE_INT | IW_PRIV_SIZE_MASK,   "mp_tx" }, 
    {BL_IOCTL_MP_SET_PKT_FREQ,      IW_PRIV_TYPE_INT | 128,  IW_PRIV_TYPE_INT | IW_PRIV_SIZE_MASK,   "mp_set_pkt_freq" },    
    {BL_IOCTL_MP_SET_CHANNEL,       IW_PRIV_TYPE_INT | 128,  IW_PRIV_TYPE_INT | IW_PRIV_SIZE_MASK,   "mp_set_channel" },    
    {BL_IOCTL_MP_SET_PKT_LEN,       IW_PRIV_TYPE_INT | 128,  IW_PRIV_TYPE_INT | IW_PRIV_SIZE_MASK,	 "mp_set_pkt_len" },	 
    {BL_IOCTL_MP_SET_CWMODE,        IW_PRIV_TYPE_INT | 128,  IW_PRIV_TYPE_INT | IW_PRIV_SIZE_MASK,   "mp_set_cw_mode" },    
    {BL_IOCTL_MP_SET_PWR,           IW_PRIV_TYPE_INT | 128,  IW_PRIV_TYPE_INT | IW_PRIV_SIZE_MASK,   "mp_set_power" },    
    {BL_IOCTL_MP_SET_TXDUTY,        IW_PRIV_TYPE_INT | 128,  IW_PRIV_TYPE_INT | IW_PRIV_SIZE_MASK,   "mp_set_tx_duty" },    
    {BL_IOCTL_MP_SET_PWR_OFT_EN,    IW_PRIV_TYPE_INT | 128,  IW_PRIV_TYPE_INT | IW_PRIV_SIZE_MASK,   "mp_en_pwr_oft" },    
    {BL_IOCTL_MP_SET_XTAL_CAP,      IW_PRIV_TYPE_INT | 128,  IW_PRIV_TYPE_INT | IW_PRIV_SIZE_MASK,   "mp_set_xtal_cap" },
    {BL_IOCTL_MP_SET_PRIV_PARAM,    IW_PRIV_TYPE_INT | 128,  IW_PRIV_TYPE_INT | IW_PRIV_SIZE_MASK,   "mp_set_dbg_para" },
    {BL_IOCTL_MP_WR_MEM,            IW_PRIV_TYPE_INT | 128,  IW_PRIV_TYPE_INT | IW_PRIV_SIZE_MASK,   "mp_wr_mem" },
    {BL_IOCTL_MP_RD_MEM,            IW_PRIV_TYPE_INT | 128,  IW_PRIV_TYPE_INT | IW_PRIV_SIZE_MASK,   "mp_rd_mem" },

    {BL_IOCTL_MP_MG,                IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_MASK,  "" },
    {BL_IOCTL_MP_HELLO,             IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_MASK,  "mp_hello" },
    {BL_IOCTL_MP_UNICAST_RX,        IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_MASK,  "mp_uc_rx" },
    {BL_IOCTL_MP_RX,                IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_MASK,  "mp_rx" },
    // {BL_IOCTL_MP_PM,                IW_PRIV_TYPE_CHAR| 128, IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_MASK,   "mp_pm" },
    // {BL_IOCTL_MP_RESET,             IW_PRIV_TYPE_CHAR| 128, IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_MASK,   "mp_reset" },
    {BL_IOCTL_MP_GET_FW_VER,        IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_MASK,  "mp_get_fw_ver" },    
    {BL_IOCTL_MP_GET_BUILD_INFO,    IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_MASK,  "mp_get_buildinf" },    
    {BL_IOCTL_MP_GET_POWER,         IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_MASK,  "mp_get_power" },    
    {BL_IOCTL_MP_GET_FREQ,          IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_MASK,  "mp_get_channel" },
    {BL_IOCTL_MP_GET_TX_STATUS,     IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_MASK,  "mp_get_tx_state" },    
    {BL_IOCTL_MP_GET_PKT_FREQ,      IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_MASK,  "mp_get_pkt_freq" },    
    {BL_IOCTL_MP_GET_XTAL_CAP,      IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_MASK,  "mp_get_xtal_cap" },   
    {BL_IOCTL_MP_GET_CWMODE,        IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_MASK,  "mp_get_cw_mode" },    
    {BL_IOCTL_MP_GET_TX_DUTY,       IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_MASK,  "mp_get_tx_duty" },
    {BL_IOCTL_MP_EFUSE_RD,          IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_MASK,  "mp_efuse_read" },
    {BL_IOCTL_MP_EFUSE_WR,          IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_MASK,  "mp_efuse_write" },
    {BL_IOCTL_MP_EFUSE_CAP_RD,      IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_MASK,  "mp_efuse_cap_rd" },
    {BL_IOCTL_MP_EFUSE_CAP_WR,      IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_MASK,  "mp_efuse_cap_wr" },
    {BL_IOCTL_MP_EFUSE_PWR_RD,      IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_MASK,  "mp_efuse_pwr_rd" },
    {BL_IOCTL_MP_EFUSE_PWR_WR,      IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_MASK,  "mp_efuse_pwr_wr" },
    {BL_IOCTL_MP_EFUSE_MAC_ADR_RD,  IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_MASK,  "mp_efuse_mac_rd" },
    {BL_IOCTL_MP_EFUSE_MAC_ADR_WR,  IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_MASK,  "mp_efuse_mac_wr" },
    {BL_IOCTL_MP_GET_TEMPERATURE,   IW_PRIV_TYPE_CHAR | 128, IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_MASK,  "mp_get_temp" },

    {BL_IOCTL_MP_IND,               IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_MASK, IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_MASK,  "mp_ind" },
#endif

     // Normal set cmd -- type INT
    {BL_IOCTL_SET_CMD_TYPE_INT,      IW_PRIV_TYPE_INT | 64,    IW_PRIV_TYPE_BYTE | 128,               "" },

     // Normal get cmd -- type INT
    {BL_IOCTL_GET_CMD_TYPE_INT,      IW_PRIV_TYPE_INT | 128,    IW_PRIV_TYPE_CHAR | 128,               "" },
    //dbg purpose, need firmware to support
    {BL_IOCIL_SUB_READ_SCRATCH,      IW_PRIV_TYPE_INT | 128,    IW_PRIV_TYPE_CHAR | 128,               "read_scratch" },
};

/* From Kernel 4.12 when CONFIG_WEXT_CORE enable,
   will only process stand/priv ioctl and ignore old driver API (ndo_do_ioctl).
*/
static const struct iw_handler_def bl_iw_handler = {
    standard: (iw_handler *)bl_iw_std_hdl_array,
    num_standard: sizeof(bl_iw_std_hdl_array) / sizeof(iw_handler),
#ifdef CONFIG_WEXT_PRIV
    num_private:  sizeof(bl_iw_priv_hdl_array) / sizeof(iw_handler),
    num_private_args: sizeof(bl_iw_priv_args) / sizeof(struct iw_priv_args),
    private:  (iw_handler *)bl_iw_priv_hdl_array,
    private_args: (struct iw_priv_args *)bl_iw_priv_args,
#endif
};

static const struct bl_dev_priv_cmd_node bl_dev_priv_cmd_table[] = {
    {"version",          bl_iwpriv_ver_hdl},
    {"temp",             bl_iwpriv_temp_read_hdl},
};

int bl_ioctl(struct net_device *dev, struct ifreq *ifreq, int cmd)
{
    struct iwreq *wrq = (struct iwreq *)ifreq;
    struct iw_request_info info;
    struct bl_dev_priv_cmd_node * priv_cmd = NULL;
    int ret = 0;
    u8  i = 0;

#ifdef CONFIG_SUPPORT_WEXT_MODE
    if(bl_mod_params.mp_mode)
        return ret;
#endif

    info.cmd = cmd;
    info.flags = 0;

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0)
    // can still process iwpriv ioctl when dev->wireless_handlers and dev->ieee80211_ptr->wiphy->wext both NULL.
    if (cmd >= SIOCIWFIRSTPRIV && cmd <= SIOCIWLASTPRIV) {
        if (bl_iw_priv_hdl_array[cmd - SIOCIWFIRSTPRIV])
            ret = bl_iw_priv_hdl_array[cmd - SIOCIWFIRSTPRIV](dev, &info, &wrq->u, NULL);
    } else
#endif

    if (cmd == BL_DEV_PRIV_IOCTL_DEFAULT) {
        BL_DBG("%s, cmd=0x%x, %s\n", __func__, cmd, wrq->u.data.pointer);
        for (i = 0; i < (u8)ARRAY_SIZE(bl_dev_priv_cmd_table); i++) {
            priv_cmd = &bl_dev_priv_cmd_table[i];
            if (priv_cmd->hdl && !strncmp(priv_cmd->name, (char *)wrq->u.data.pointer, strlen(priv_cmd->name))) {
                ret = priv_cmd->hdl(dev, &info, &wrq->u, NULL);
                break;
            }
        }

        if(i == ARRAY_SIZE(bl_dev_priv_cmd_table)) {
            printk("command(%s) handler not found\n", (char *)wrq->u.data.pointer);
            ret = -2;
        }
    } else {
        printk("Unknown cmd 0x%x\n", cmd);
        ret = -3;
    }

    return ret;
}

static const struct net_device_ops bl_netdev_ops = {
    .ndo_open               = bl_open,
    .ndo_stop               = bl_close,
    .ndo_do_ioctl           = bl_ioctl,
    .ndo_start_xmit         = bl_start_xmit,
    .ndo_get_stats          = bl_get_stats,
    .ndo_select_queue       = bl_select_queue,
    .ndo_set_mac_address    = bl_set_mac_address
};

static void bl_netdev_setup(struct net_device *dev)
{
    ether_setup(dev);
    dev->priv_flags &= ~IFF_TX_SKB_SHARING;
    dev->netdev_ops = &bl_netdev_ops;
#if LINUX_VERSION_CODE <  KERNEL_VERSION(4, 12, 0)
    dev->destructor = free_netdev;
#else
    dev->needs_free_netdev = true;
#endif
#ifdef CONFIG_WIRELESS_EXT
	dev->wireless_handlers = &bl_iw_handler;
#ifdef CONFIG_CFG80211_WEXT
#ifdef CONFIG_SUPPORT_WEXT_MODE
    //don't register iw handler here if need support wext standard ioctl
    dev->wireless_handlers = NULL;
    printk("don't register wireless handler here \n");
#endif
#endif
#endif

    dev->watchdog_timeo = BL_TX_LIFETIME_MS;

    dev->needed_headroom = sizeof(struct bl_txhdr) + BL_SWTXHDR_ALIGN_SZ;
#ifdef CONFIG_BL_AMSDUS_TX
    dev->needed_headroom = max(dev->needed_headroom,
                               (unsigned short)(sizeof(struct bl_amsdu_txhdr)
                                                + sizeof(struct ethhdr) + 4
                                                + sizeof(rfc1042_header) + 2));
#endif /* CONFIG_BL_AMSDUS_TX */
	dev->flags |= IFF_BROADCAST | IFF_MULTICAST;

    dev->hw_features = 0;
}

/*********************************************************************
 * Cfg80211 callbacks (and helper)
 *********************************************************************/
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 6, 0)
static struct net_device *bl_interface_add(struct bl_hw *bl_hw, const char *name, enum nl80211_iftype type, struct vif_params *params)
#else
static struct wireless_dev *bl_interface_add(struct bl_hw *bl_hw, const char *name, enum nl80211_iftype type, struct vif_params *params)
#endif
{
    struct net_device *ndev;
    struct bl_vif *vif;
    int min_idx, max_idx;
    int vif_idx = -1;
    int i;

	BL_DBG(BL_FN_ENTRY_STR);
	
    // Look for an available VIF
    if (type == NL80211_IFTYPE_AP_VLAN) {
        min_idx = NX_VIRT_DEV_MAX;
        max_idx = NX_VIRT_DEV_MAX + NX_REMOTE_STA_MAX;
    } else {
        min_idx = 0;
        max_idx = NX_VIRT_DEV_MAX;
    }

    for (i = min_idx; i < max_idx; i++) {
        if ((bl_hw->avail_idx_map) & BIT(i)) {
            vif_idx = i;
            break;
        }
    }
    if (vif_idx < 0)
        return NULL;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0)
    ndev = alloc_netdev_mqs(sizeof(*vif), name, NET_NAME_UNKNOWN,
                            bl_netdev_setup, IEEE80211_NUM_ACS, 1);
#else
    ndev = alloc_netdev_mqs(sizeof(*vif), name, bl_netdev_setup,
                            IEEE80211_NUM_ACS, 1);
#endif
    if (!ndev)
        return NULL;

    bl_hw->ndev = ndev;
    vif = netdev_priv(ndev);
    ndev->ieee80211_ptr = &vif->wdev;
    vif->wdev.wiphy = bl_hw->wiphy;
    vif->bl_hw = bl_hw;
    vif->ndev = ndev;
    vif->drv_vif_index = vif_idx;
    SET_NETDEV_DEV(ndev, wiphy_dev(vif->wdev.wiphy));
    vif->wdev.netdev = ndev;
    vif->wdev.iftype = type;
    vif->up = false;
    vif->ch_index = BL_CH_NOT_SET;
    memset(&vif->net_stats, 0, sizeof(vif->net_stats));
    memset(&vif->rx_pn, 0, sizeof(vif->rx_pn));

    ndev->priv_flags &= ~IFF_DONT_BRIDGE;
    switch (type) {
    case NL80211_IFTYPE_STATION:
    case NL80211_IFTYPE_P2P_CLIENT:
        vif->sta.ap = NULL;
        vif->sta.tdls_sta = NULL;
        break;
    case NL80211_IFTYPE_MESH_POINT:
        INIT_LIST_HEAD(&vif->ap.mpath_list);
        INIT_LIST_HEAD(&vif->ap.proxy_list);
        vif->ap.create_path = false;
        vif->ap.generation = 0;
        // no break
    case NL80211_IFTYPE_AP:
    case NL80211_IFTYPE_P2P_GO:
        INIT_LIST_HEAD(&vif->ap.sta_list);
        memset(&vif->ap.bcn, 0, sizeof(vif->ap.bcn));
        break;
    case NL80211_IFTYPE_AP_VLAN:
    {
        struct bl_vif *master_vif;
        bool found = false;
        list_for_each_entry(master_vif, &bl_hw->vifs, list) {
            if ((BL_VIF_TYPE(master_vif) == NL80211_IFTYPE_AP) &&
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0)
                !(!memcmp(master_vif->ndev->dev_addr, params->macaddr, ETH_ALEN))) {
#else
                !(!memcmp(master_vif->ndev->dev_addr, bl_hw->wiphy->perm_addr, ETH_ALEN))) {
#endif
                 found=true;
                 break;
            }
        }

        if (!found) {
            printk("%s, Mac not match with devices:%pM\n", __func__, master_vif->ndev->dev_addr);
            goto err;
        }

         vif->ap_vlan.master = master_vif;
         vif->ap_vlan.sta_4a = NULL;
         break;
    }
    default:
        break;
    }

    if (type == NL80211_IFTYPE_AP_VLAN)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0)
        memcpy(ndev->dev_addr, params->macaddr, ETH_ALEN);
#else
        memcpy(ndev->dev_addr, bl_hw->wiphy->perm_addr, ETH_ALEN);
#endif
    else {
        memcpy(ndev->dev_addr, bl_hw->wiphy->perm_addr, ETH_ALEN);
        ndev->dev_addr[5] ^= vif_idx;
    }

    if (params) {
        vif->use_4addr = params->use_4addr;
        //ndev->ieee80211_ptr->use_4addr = params->use_4addr;   /* Mark this to avoid repeater mode STA interface deauth by AP (reason=6) */
        /* Add this flag for adding STA interface to bridge */
        vif->wdev.use_4addr = 1;
    } else
        vif->use_4addr = false;

    if (register_netdevice(ndev))
        goto err;

#ifdef CONFIG_SUPPORT_WEXT_MODE
#ifdef CONFIG_CFG80211_WEXT
#ifdef CONFIG_WIRELESS_EXT
    if(bl_hw->mod_params->mp_mode)
        ndev->wireless_handlers = &bl_iw_handler;
#endif
#endif
#endif

	
    list_add_tail(&vif->list, &bl_hw->vifs);
    bl_hw->avail_idx_map &= ~BIT(vif_idx);

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 6, 0)
    return vif->wdev.netdev;
#else
    return &vif->wdev;
#endif

err:
    free_netdev(ndev);
    return NULL;
}


/*
 * @brief Retrieve the bl_sta object allocated for a given MAC address
 * and a given role.
 */
static struct bl_sta *bl_retrieve_sta(struct bl_hw *bl_hw,
                                          struct bl_vif *bl_vif, u8 *addr,
                                          __le16 fc, bool ap)
{
    if (ap) {
        /* only deauth, disassoc and action are bufferable MMPDUs */
        bool bufferable = ieee80211_is_deauth(fc) ||
                          ieee80211_is_disassoc(fc) ||
                          ieee80211_is_action(fc);

        /* Check if the packet is bufferable or not */
        if (bufferable)
        {
            /* Check if address is a broadcast or a multicast address */
            if (is_broadcast_ether_addr(addr) || is_multicast_ether_addr(addr)) {
                /* Returned STA pointer */
                struct bl_sta *bl_sta = &bl_hw->sta_table[bl_vif->ap.bcmc_index];

                if (bl_sta->valid)
                    return bl_sta;
            } else {
                /* Returned STA pointer */
                struct bl_sta *bl_sta;

                /* Go through list of STAs linked with the provided VIF */
                list_for_each_entry(bl_sta, &bl_vif->ap.sta_list, list) {
                    if (bl_sta->valid &&
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0)
                        ether_addr_equal(bl_sta->mac_addr, addr)) {
#else
                        !compare_ether_addr(bl_sta->mac_addr, addr)) {
#endif
                        /* Return the found STA */
                        return bl_sta;
                    }
                }
            }
        }
    } else {
        return bl_vif->sta.ap;
    }

    return NULL;
}

/**
 * @add_virtual_intf: create a new virtual interface with the given name,
 *	must set the struct wireless_dev's iftype. Beware: You must create
 *	the new netdev in the wiphy's network namespace! Returns the struct
 *	wireless_dev, or an ERR_PTR. For P2P device wdevs, the driver must
 *	also set the address member in the wdev.
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0)
static struct wireless_dev *
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 38)
static struct net_device *
#else
static int
#endif
bl_cfg80211_add_iface(struct wiphy *wiphy,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
                        const char *name,
#else
                        char *name,
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0)
                        unsigned char name_assign_type,
#endif
                        enum nl80211_iftype type,
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0)
                        u32 *flags,
#endif
                        struct vif_params *params)
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);

    return bl_interface_add(bl_hw, name, type, params);
}

/**
 * @del_virtual_intf: remove the virtual interface
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 6, 0)
static int bl_cfg80211_del_iface(struct wiphy *wiphy, struct net_device *dev)
#else
static int bl_cfg80211_del_iface(struct wiphy *wiphy, struct wireless_dev *wdev)
#endif
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0)
    struct net_device *dev = wdev->netdev;
#endif
    struct bl_vif *bl_vif = netdev_priv(dev);
    struct bl_hw *bl_hw = wiphy_priv(wiphy);

    netdev_info(dev, "Remove Interface");

    if (dev->reg_state == NETREG_REGISTERED) {
        /* Will call bl_close if interface is UP */
        unregister_netdevice(dev);
    }

    list_del(&bl_vif->list);
    bl_hw->avail_idx_map |= BIT(bl_vif->drv_vif_index);
    bl_vif->ndev = NULL;

    /* Clear the priv in adapter */
    dev->ieee80211_ptr = NULL;

    return 0;
}

/**
 * @change_virtual_intf: change type/configuration of virtual interface,
 *	keep the struct wireless_dev's iftype updated.
 */
static int bl_cfg80211_change_iface(struct wiphy *wiphy,
                   struct net_device *dev,
                   enum nl80211_iftype type,
                   struct vif_params *params)
{
    struct bl_vif *vif = netdev_priv(dev);

    if (vif->up)
        return (-EBUSY);

    switch (type) {
    case NL80211_IFTYPE_STATION:
    case NL80211_IFTYPE_P2P_CLIENT:
        vif->sta.ap = NULL;
        vif->sta.tdls_sta = NULL;
        break;
    case NL80211_IFTYPE_MESH_POINT:
        INIT_LIST_HEAD(&vif->ap.mpath_list);
        INIT_LIST_HEAD(&vif->ap.proxy_list);
        vif->ap.create_path = false;
        vif->ap.generation = 0;
        /* fall through */
    case NL80211_IFTYPE_AP:
    case NL80211_IFTYPE_P2P_GO:
        INIT_LIST_HEAD(&vif->ap.sta_list);
        memset(&vif->ap.bcn, 0, sizeof(vif->ap.bcn));
        break;
    case NL80211_IFTYPE_AP_VLAN:
        return -EPERM;
    default:
        break;
    }

    vif->wdev.iftype = type;
    if (params->use_4addr != -1)
        vif->use_4addr = params->use_4addr;

    return 0;
}

/**
 * @scan: Request to do a scan. If returning zero, the scan request is given
 *	the driver, and will be valid until passed to cfg80211_scan_done().
 *	For scan results, call cfg80211_inform_bss(); you can call this outside
 *	the scan/scan_done bracket too.
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0)
static int bl_cfg80211_scan(struct wiphy *wiphy,
                              struct cfg80211_scan_request *request)
#else
static int bl_cfg80211_scan(struct wiphy *wiphy, struct net_device *dev,
                              struct cfg80211_scan_request *request)
#endif
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0)
    struct bl_vif *bl_vif = container_of(request->wdev, struct bl_vif,
                                             wdev);
#else
    struct bl_vif *bl_vif = container_of(request->dev->ieee80211_ptr, struct bl_vif,
                                             wdev);
#endif
    int error;

    BL_DBG(BL_FN_ENTRY_STR);

    if(bl_hw->scan_request){
        printk("scan already in processing ...\n");
        return -EAGAIN;
    }
	
    bl_hw->scan_request = request;

    if ((error = bl_send_scanu_req(bl_hw, bl_vif, request))) {
        bl_hw->scan_request = NULL;
        return error;
    }


    return 0;
}

/**
 * @add_key: add a key with the given parameters. @mac_addr will be %NULL
 *	when adding a group key.
 */
static int bl_cfg80211_add_key(struct wiphy *wiphy, struct net_device *netdev,
                                 u8 key_index, bool pairwise, const u8 *mac_addr,
                                 struct key_params *params)
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct bl_vif *vif = netdev_priv(netdev);
    int i, error = 0;
    struct mm_key_add_cfm key_add_cfm;
    u8_l cipher = 0;
    struct bl_sta *sta = NULL;
    struct bl_key *bl_key;

    BL_DBG(BL_FN_ENTRY_STR);

    if (mac_addr) {
        sta = bl_get_sta(bl_hw, mac_addr);
        if (!sta)
            return -EINVAL;
        bl_key = &sta->key;
    }
    else
        bl_key = &vif->key[key_index];

    /* Retrieve the cipher suite selector */
    switch (params->cipher) {
    case WLAN_CIPHER_SUITE_WEP40:
        cipher = MAC_RSNIE_CIPHER_WEP40;
        break;
    case WLAN_CIPHER_SUITE_WEP104:
        cipher = MAC_RSNIE_CIPHER_WEP104;
        break;
    case WLAN_CIPHER_SUITE_TKIP:
        cipher = MAC_RSNIE_CIPHER_TKIP;
        break;
    case WLAN_CIPHER_SUITE_CCMP:
        cipher = MAC_RSNIE_CIPHER_CCMP;
        break;
    case WLAN_CIPHER_SUITE_AES_CMAC:
        cipher = MAC_RSNIE_CIPHER_AES_CMAC;
        break;
    case WLAN_CIPHER_SUITE_SMS4:
    {
        // Need to reverse key order
        u8 tmp, *key = (u8 *)params->key;
        cipher = MAC_RSNIE_CIPHER_SMS4;
        for (i = 0; i < WPI_SUBKEY_LEN/2; i++) {
            tmp = key[i];
            key[i] = key[WPI_SUBKEY_LEN - 1 - i];
            key[WPI_SUBKEY_LEN - 1 - i] = tmp;
        }
        for (i = 0; i < WPI_SUBKEY_LEN/2; i++) {
            tmp = key[i + WPI_SUBKEY_LEN];
            key[i + WPI_SUBKEY_LEN] = key[WPI_KEY_LEN - 1 - i];
            key[WPI_KEY_LEN - 1 - i] = tmp;
        }
        break;
    }
    default:
        return -EINVAL;
    }

    //if((error = bl_send_key_del(bl_hw, key_index)))
    //    return error;

    if ((error = bl_send_key_add(bl_hw, vif->vif_index,
                                   (sta ? sta->sta_idx : 0xFF), pairwise,
                                   (u8 *)params->key, params->key_len,
                                   key_index, cipher, &key_add_cfm)))
        return error;

    if (key_add_cfm.status != 0) {
        BL_PRINT_CFM_ERR(key_add);
        return -EIO;
    }

    /* Save the index retrieved from LMAC */
    bl_key->hw_idx = key_add_cfm.hw_key_idx;

    return 0;
}

/**
 * @get_key: get information about the key with the given parameters.
 *	@mac_addr will be %NULL when requesting information for a group
 *	key. All pointers given to the @callback function need not be valid
 *	after it returns. This function should return an error if it is
 *	not possible to retrieve the key, -ENOENT if it doesn't exist.
 *
 */
static int bl_cfg80211_get_key(struct wiphy *wiphy, struct net_device *netdev,
                                 u8 key_index, bool pairwise, const u8 *mac_addr,
                                 void *cookie,
                                 void (*callback)(void *cookie, struct key_params*))
{
    BL_DBG(BL_FN_ENTRY_STR);

    return -1;
}


/**
 * @del_key: remove a key given the @mac_addr (%NULL for a group key)
 *	and @key_index, return -ENOENT if the key doesn't exist.
 */
static int bl_cfg80211_del_key(struct wiphy *wiphy, struct net_device *netdev,
                                 u8 key_index, bool pairwise, const u8 *mac_addr)
{
	struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct bl_vif *vif = netdev_priv(netdev);
    int error;
    struct bl_sta *sta = NULL;
    struct bl_key *bl_key;

    BL_DBG(BL_FN_ENTRY_STR);
    if (mac_addr) {
        sta = bl_get_sta(bl_hw, mac_addr);
        if (!sta)
            return -EINVAL;
        bl_key = &sta->key;
    }
    else
        bl_key = &vif->key[key_index];

    error = bl_send_key_del(bl_hw, bl_key->hw_idx);
    return error;
}

/**
 * @set_default_key: set the default key on an interface
 */
static int bl_cfg80211_set_default_key(struct wiphy *wiphy,
                                         struct net_device *netdev,
                                         u8 key_index, bool unicast, bool multicast)
{
    BL_DBG(BL_FN_ENTRY_STR);

    return 0;
}

/**
 * @set_default_mgmt_key: set the default management frame key on an interface
 */
static int bl_cfg80211_set_default_mgmt_key(struct wiphy *wiphy,
                                              struct net_device *netdev,
                                              u8 key_index)
{
    return 0;
}

/**
 * @connect: Connect to the ESS with the specified parameters. When connected,
 *	call cfg80211_connect_result() with status code %WLAN_STATUS_SUCCESS.
 *	If the connection fails for some reason, call cfg80211_connect_result()
 *	with the status from the AP.
 *	(invoked with the wireless_dev mutex held)
 */
static int bl_cfg80211_connect(struct wiphy *wiphy, struct net_device *dev,
                                 struct cfg80211_connect_params *sme)
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct bl_vif *bl_vif = netdev_priv(dev);
    struct sm_connect_cfm sm_connect_cfm;
    int error = 0;

    BL_DBG(BL_FN_ENTRY_STR);

#ifdef CONFIG_SUPPORT_WEXT_MODE
    if (sme->bssid == NULL) {
        printk("%s, sme->bssid is NULL, NOT connnect\r\n", __func__);
        return -EINVAL;
    }
#endif

    if(bl_vif->connected && (sme->auth_type == NL80211_AUTHTYPE_OPEN_SYSTEM))
	    bl_send_sm_disconnect_req(bl_hw, bl_vif, 0);
    /* For SHARED-KEY authentication, must install key first */
    if (sme->auth_type == NL80211_AUTHTYPE_SHARED_KEY && sme->key)
    {
        struct key_params key_params;
        key_params.key = sme->key;
        key_params.seq = NULL;
        key_params.key_len = sme->key_len;
        key_params.seq_len = 0;
        key_params.cipher = sme->crypto.cipher_group;
        bl_cfg80211_add_key(wiphy, dev, sme->key_idx, false, NULL, &key_params);
    }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0) || defined(BL_WPA3_COMPAT)
    else if ((sme->auth_type == NL80211_AUTHTYPE_SAE) &&
        !(sme->flags & CONNECT_REQ_EXTERNAL_AUTH_SUPPORT)) {
        netdev_err(dev, "Doesn't support SAE without external authentication\n");
        return -EINVAL;
    }
#endif

    /* Forward the information to the LMAC */
    if ((error = bl_send_sm_connect_req(bl_hw, bl_vif, sme, &sm_connect_cfm)))
        return error;

    // Check the status
    switch (sm_connect_cfm.status)
    {
        case CO_OK:
            error = 0;
            break;
        case CO_BUSY:
            error = -EINPROGRESS;
            break;
        case CO_OP_IN_PROGRESS:
            error = -EALREADY;
            break;
        default:
            error = -EIO;
            break;
    }

    return error;
}

/**
 * @disconnect: Disconnect from the BSS/ESS.
 *	(invoked with the wireless_dev mutex held)
 */
static int bl_cfg80211_disconnect(struct wiphy *wiphy, struct net_device *dev,
                                    u16 reason_code)
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct bl_vif *bl_vif = netdev_priv(dev);

    BL_DBG(BL_FN_ENTRY_STR);

    return(bl_send_sm_disconnect_req(bl_hw, bl_vif, reason_code));
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0) || defined(BL_WPA3_COMPAT)
/**
 * @external_auth: indicates result of offloaded authentication processing from
 *     user space
 */
static int bl_cfg80211_external_auth(struct wiphy *wiphy, struct net_device *dev,
                                       struct cfg80211_external_auth_params *params)
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct bl_vif *bl_vif = netdev_priv(dev);

    if (!(bl_vif->sta.flags & RWNX_STA_EXT_AUTH))
        return -EINVAL;

    bl_external_auth_disable(bl_vif);
    return bl_send_sm_external_auth_required_rsp(bl_hw, bl_vif,
                                                   params->status);
}
#endif

/**
 * @add_station: Add a new station.
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0)
static int bl_cfg80211_add_station(struct wiphy *wiphy, struct net_device *dev,
                                     const u8 *mac, struct station_parameters *params)

#else
static int bl_cfg80211_add_station(struct wiphy *wiphy, struct net_device *dev,
                                     u8 *mac, struct station_parameters *params)
#endif
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct bl_vif *bl_vif = netdev_priv(dev);
    struct me_sta_add_cfm me_sta_add_cfm;
    int error = 0;

    BL_DBG(BL_FN_ENTRY_STR);

    WARN_ON(BL_VIF_TYPE(bl_vif) == NL80211_IFTYPE_AP_VLAN);

    /* Do not add TDLS station */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0)
    if (params->sta_flags_set & BIT(NL80211_STA_FLAG_TDLS_PEER))
        return 0;
#endif

    /* Indicate we are in a STA addition process - This will allow handling
     * potential PS mode change indications correctly
     */
    bl_hw->adding_sta = true;

    /* Forward the information to the LMAC */
    if ((error = bl_send_me_sta_add(bl_hw, params, mac, bl_vif->vif_index,
                                      &me_sta_add_cfm)))
        return error;

    // Check the status
    switch (me_sta_add_cfm.status)
    {
        case CO_OK:
        {
            struct bl_sta *sta = &bl_hw->sta_table[me_sta_add_cfm.sta_idx];

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0)
            int tid;
#endif

            sta->aid = params->aid;

            sta->sta_idx = me_sta_add_cfm.sta_idx;
            sta->ch_idx = bl_vif->ch_index;
            sta->vif_idx = bl_vif->vif_index;
            sta->vlan_idx = sta->vif_idx;
            sta->qos = (params->sta_flags_set & BIT(NL80211_STA_FLAG_WME)) != 0;
            sta->ht = params->ht_capa ? 1 : 0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
            sta->vht = params->vht_capa ? 1 : 0;
#else
            sta->vht = 0;
#endif
            sta->acm = 0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0)
            for (tid = 0; tid < NX_NB_TXQ_PER_STA; tid++) {
                int uapsd_bit = bl_hwq2uapsd[bl_tid2hwq[tid]];
                if (params->uapsd_queues & uapsd_bit)
                    sta->uapsd_tids |= 1 << tid;
                else
                    sta->uapsd_tids &= ~(1 << tid);
            }
#endif
            memcpy(sta->mac_addr, mac, ETH_ALEN);
           
#ifdef CONFIG_BL_DEBUGFS
            bl_dbgfs_register_rc_stat(bl_hw, sta);
#endif

            /* Ensure that we won't process PS change or channel switch ind*/
            spin_lock_bh(&bl_hw->cb_lock);
            bl_txq_sta_init(bl_hw, sta, bl_txq_vif_get_status(bl_vif));
            list_add_tail(&sta->list, &bl_vif->ap.sta_list);
            sta->valid = true;
            bl_ps_bh_enable(bl_hw, sta, sta->ps.active || me_sta_add_cfm.pm_state);
            spin_unlock_bh(&bl_hw->cb_lock);

            error = 0;

#ifdef CONFIG_BL_BFMER
            if (bl_hw->mod_params->bfmer)
                bl_send_bfmer_enable(bl_hw, sta, params->vht_capa);

            bl_mu_group_sta_init(sta, params->vht_capa);
#endif /* CONFIG_BL_BFMER */

            #define PRINT_STA_FLAG(f)                               \
                (params->sta_flags_set & BIT(NL80211_STA_FLAG_##f) ? "["#f"]" : "")

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0)
            netdev_info(dev, "Add sta %d (%pM) flags=%s%s%s%s%s%s%s",
                        sta->sta_idx, mac,
                        PRINT_STA_FLAG(AUTHORIZED),
                        PRINT_STA_FLAG(SHORT_PREAMBLE),
                        PRINT_STA_FLAG(WME),
                        PRINT_STA_FLAG(MFP),
                        PRINT_STA_FLAG(AUTHENTICATED),
                        PRINT_STA_FLAG(TDLS_PEER),
                        PRINT_STA_FLAG(ASSOCIATED));
#else
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0)
            netdev_info(dev, "Add sta %d (%pM) flags=%s%s%s%s%s%s",
                        sta->sta_idx, mac,
                        PRINT_STA_FLAG(AUTHORIZED),
                        PRINT_STA_FLAG(SHORT_PREAMBLE),
                        PRINT_STA_FLAG(WME),
                        PRINT_STA_FLAG(MFP),
                        PRINT_STA_FLAG(AUTHENTICATED)
                        , PRINT_STA_FLAG(TDLS_PEER)
                        );
#else
            netdev_info(dev, "Add sta %d (%pM) flags=%s%s%s%s%s",
                        sta->sta_idx, mac,
                        PRINT_STA_FLAG(AUTHORIZED),
                        PRINT_STA_FLAG(SHORT_PREAMBLE),
                        PRINT_STA_FLAG(WME),
                        PRINT_STA_FLAG(MFP),
                        PRINT_STA_FLAG(AUTHENTICATED)
                        );
#endif
#endif

            #undef PRINT_STA_FLAG
            break;
        }
        default:
            error = -EBUSY;
            break;
    }

    bl_hw->adding_sta = false;

    return error;
}

/**
 * @del_station: Remove a station
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)
static int bl_cfg80211_del_station(struct wiphy *wiphy, struct net_device *dev,
                              struct station_del_parameters *params)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0)
static int bl_cfg80211_del_station(struct wiphy *wiphy, struct net_device *dev,
                              const u8 *mac)
#else
static int bl_cfg80211_del_station(struct wiphy *wiphy, struct net_device *dev,
                              u8 *mac)
#endif
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct bl_vif *bl_vif = netdev_priv(dev);
    struct bl_sta *cur, *tmp;
    int error = 0, found = 0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)
    const u8 *mac = params->mac;
#endif

    list_for_each_entry_safe(cur, tmp, &bl_vif->ap.sta_list, list) {
        if ((!mac) || (!memcmp(cur->mac_addr, mac, ETH_ALEN))) {
            netdev_info(dev, "Del sta %d (%pM)", cur->sta_idx, cur->mac_addr);
            /* Ensure that we won't process PS change ind */
            spin_lock_bh(&bl_hw->cb_lock);
            cur->ps.active = false;
            cur->valid = false;
            spin_unlock_bh(&bl_hw->cb_lock);

            if (cur->vif_idx != cur->vlan_idx) {
                struct bl_vif *vlan_vif;
                vlan_vif = bl_hw->vif_table[cur->vlan_idx];
                if (vlan_vif->up) {
                    if ((BL_VIF_TYPE(vlan_vif) == NL80211_IFTYPE_AP_VLAN) &&
                        (vlan_vif->use_4addr)) {
                        vlan_vif->ap_vlan.sta_4a = NULL;
                    } else {
                        WARN(1, "Deleting sta belonging to VLAN other than AP_VLAN 4A");
                    }
                }
            }

            bl_txq_sta_deinit(bl_hw, cur);
            error = bl_send_me_sta_del(bl_hw, cur->sta_idx, false);
            if ((error != 0) && (error != -EPIPE))
                return error;

#ifdef CONFIG_BL_BFMER
            // Disable Beamformer if supported
            bl_bfmer_report_del(bl_hw, cur);
            bl_mu_group_sta_del(bl_hw, cur);
#endif /* CONFIG_BL_BFMER */

            list_del(&cur->list);
            
#ifdef CONFIG_BL_DEBUGFS
            bl_dbgfs_unregister_rc_stat(bl_hw, cur);
#endif

            found ++;
            break;
        }
    }

    if (!found)
        return -ENOENT;
    else
        return 0;
}

/**
 * @change_station: Modify a given station. Note that flags changes are not much
 *	validated in cfg80211, in particular the auth/assoc/authorized flags
 *	might come to the driver in invalid combinations -- make sure to check
 *	them, also against the existing state! Drivers must call
 *	cfg80211_check_station_change() to validate the information.
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0)
static int bl_cfg80211_change_station(struct wiphy *wiphy, struct net_device *dev,
                                        const u8 *mac, struct station_parameters *params)
#else
static int bl_cfg80211_change_station(struct wiphy *wiphy, struct net_device *dev,
                                        u8 *mac, struct station_parameters *params)
#endif
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct bl_vif *vif = netdev_priv(dev);
    struct bl_sta *sta;

	BL_DBG(BL_FN_ENTRY_STR);

    sta = bl_get_sta(bl_hw, mac);
    if (!sta)
    {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0)
        /* Add the TDLS station */
        if (params->sta_flags_set & BIT(NL80211_STA_FLAG_TDLS_PEER))
        {
            struct bl_vif *bl_vif = netdev_priv(dev);
            struct me_sta_add_cfm me_sta_add_cfm;
            int error = 0;

            /* Indicate we are in a STA addition process - This will allow handling
             * potential PS mode change indications correctly
             */
            bl_hw->adding_sta = true;

            /* Forward the information to the LMAC */
            if ((error = bl_send_me_sta_add(bl_hw, params, mac, bl_vif->vif_index,
                                              &me_sta_add_cfm)))
                return error;

            // Check the status
            switch (me_sta_add_cfm.status)
            {
                case CO_OK:
                {
                    int tid;
                    sta = &bl_hw->sta_table[me_sta_add_cfm.sta_idx];
                    sta->aid = params->aid;
                    sta->sta_idx = me_sta_add_cfm.sta_idx;
                    sta->ch_idx = bl_vif->ch_index;
                    sta->vif_idx = bl_vif->vif_index;
                    sta->vlan_idx = sta->vif_idx;
                    sta->qos = (params->sta_flags_set & BIT(NL80211_STA_FLAG_WME)) != 0;
                    sta->ht = params->ht_capa ? 1 : 0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
                    sta->vht = params->vht_capa ? 1 : 0;
#else
                    sta->vht = 0;
#endif
                    sta->acm = 0;
                    for (tid = 0; tid < NX_NB_TXQ_PER_STA; tid++) {
                        int uapsd_bit = bl_hwq2uapsd[bl_tid2hwq[tid]];
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0)
                        if (params->uapsd_queues & uapsd_bit)
                            sta->uapsd_tids |= 1 << tid;
                        else
#endif
                            sta->uapsd_tids &= ~(1 << tid);
                    }
                    memcpy(sta->mac_addr, mac, ETH_ALEN);
#ifdef CONFIG_BL_DEBUGFS
                    bl_dbgfs_register_rc_stat(bl_hw, sta);
#endif
                    /* Ensure that we won't process PS change or channel switch ind*/
                    spin_lock_bh(&bl_hw->cb_lock);
                    bl_txq_sta_init(bl_hw, sta, bl_txq_vif_get_status(bl_vif));
                    sta->valid = true;
                    spin_unlock_bh(&bl_hw->cb_lock);

                    #define PRINT_STA_FLAG(f)                               \
                        (params->sta_flags_set & BIT(NL80211_STA_FLAG_##f) ? "["#f"]" : "")

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0)
                    netdev_info(dev, "Add TDLS sta %d (%pM) flags=%s%s%s%s%s%s%s",
                                sta->sta_idx, mac,
                                PRINT_STA_FLAG(AUTHORIZED),
                                PRINT_STA_FLAG(SHORT_PREAMBLE),
                                PRINT_STA_FLAG(WME),
                                PRINT_STA_FLAG(MFP),
                                PRINT_STA_FLAG(AUTHENTICATED),
                                PRINT_STA_FLAG(TDLS_PEER),
                                PRINT_STA_FLAG(ASSOCIATED));
#else
                    netdev_info(dev, "Add TDLS sta %d (%pM) flags=%s%s%s%s%s%s",
                                sta->sta_idx, mac,
                                PRINT_STA_FLAG(AUTHORIZED),
                                PRINT_STA_FLAG(SHORT_PREAMBLE),
                                PRINT_STA_FLAG(WME),
                                PRINT_STA_FLAG(MFP),
                                PRINT_STA_FLAG(AUTHENTICATED),
                                PRINT_STA_FLAG(TDLS_PEER));
#endif
                    #undef PRINT_STA_FLAG

                    break;
                }
                default:
                    error = -EBUSY;
                    break;
            }

            bl_hw->adding_sta = false;
        } else 
#endif // 3.2
        {
            return -EINVAL;
        }
    }

    if (params->sta_flags_mask & BIT(NL80211_STA_FLAG_AUTHORIZED))
        bl_send_me_set_control_port_req(bl_hw,
                (params->sta_flags_set & BIT(NL80211_STA_FLAG_AUTHORIZED)) != 0,
                sta->sta_idx);

    if (params->vlan) {
        uint8_t vlan_idx;

        vif = netdev_priv(params->vlan);
        vlan_idx = vif->vif_index;

        if (sta->vlan_idx != vlan_idx) {
            struct bl_vif *old_vif;
            old_vif = bl_hw->vif_table[sta->vlan_idx];
            bl_txq_sta_switch_vif(sta, old_vif, vif);
            sta->vlan_idx = vlan_idx;

            if ((BL_VIF_TYPE(vif) == NL80211_IFTYPE_AP_VLAN) &&
                (vif->use_4addr)) {
                WARN((vif->ap_vlan.sta_4a),
                     "4A AP_VLAN interface with more than one sta");
                vif->ap_vlan.sta_4a = sta;
            }

            if ((BL_VIF_TYPE(old_vif) == NL80211_IFTYPE_AP_VLAN) &&
                (old_vif->use_4addr)) {
                old_vif->ap_vlan.sta_4a = NULL;
            }
        }
    }

    return 0;
}

/**
 * @start_ap: Start acting in AP mode defined by the parameters.
 */
static int bl_cfg80211_start_ap(struct wiphy *wiphy, struct net_device *dev,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
                                  struct cfg80211_ap_settings *settings
#else
                                  struct beacon_parameters *settings
#endif
                                  )
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct bl_vif *bl_vif = netdev_priv(dev);
    struct apm_start_cfm apm_start_cfm;
    struct bl_dma_elem elem;
    struct bl_sta *sta;
    int error = 0;
    void * chan_def = NULL;

    BL_DBG(BL_FN_ENTRY_STR);

    /* Forward the information to the LMAC */
    if ((error = bl_send_apm_start_req(bl_hw, bl_vif, settings, &apm_start_cfm, &elem))) {
        return error;
	}

    // Check the status
    switch (apm_start_cfm.status)
    {
        case CO_OK:
        {
            u8 txq_status = 0;
            bl_vif->ap.bcmc_index = apm_start_cfm.bcmc_idx;
            bl_vif->ap.flags = 0;
            sta = &bl_hw->sta_table[apm_start_cfm.bcmc_idx];
            sta->valid = true;
            sta->aid = 0;
            sta->sta_idx = apm_start_cfm.bcmc_idx;
            sta->ch_idx = apm_start_cfm.ch_idx;
            sta->vif_idx = bl_vif->vif_index;
            sta->qos = false;
            sta->acm = 0;
            sta->ps.active = false;
            spin_lock_bh(&bl_hw->cb_lock);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
            chan_def = &settings->chandef;
#endif
            bl_chanctx_link(bl_vif, apm_start_cfm.ch_idx, chan_def);
            if (bl_hw->cur_chanctx != apm_start_cfm.ch_idx) {
                txq_status = BL_TXQ_STOP_CHAN;
            }
            bl_txq_vif_init(bl_hw, bl_vif, txq_status);
            spin_unlock_bh(&bl_hw->cb_lock);

            netif_tx_start_all_queues(dev);
            netif_carrier_on(dev);
            error = 0;
            break;
        }
        case CO_BUSY:
            error = -EINPROGRESS;
            break;
        case CO_OP_IN_PROGRESS:
            error = -EALREADY;
            break;
        default:
            error = -EIO;
            break;
    }

    if (error) {
        netdev_info(dev, "Failed to start AP (%d)", error);
    } else {
        netdev_info(dev, "AP started: ch=%d, bcmc_idx=%d",
                    bl_vif->ch_index, bl_vif->ap.bcmc_index);
    }

    // Free the buffer used to build the beacon
    kfree(elem.buf);

    return error;
}


/**
 * @change_beacon: Change the beacon parameters for an access point mode
 *	interface. This should reject the call when AP mode wasn't started.
 */
static int bl_cfg80211_change_beacon(struct wiphy *wiphy, struct net_device *dev,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
                                       struct cfg80211_beacon_data *info
#else
                                       struct beacon_parameters *info
#endif
                                       )
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct bl_vif *vif = netdev_priv(dev);
    struct bl_bcn *bcn = &vif->ap.bcn;
    struct bl_dma_elem elem;
    u8 *buf;
    int error = 0;

    BL_DBG(BL_FN_ENTRY_STR);

    // Build the beacon
    buf = bl_build_bcn(bcn, info);
    if (!buf)
        return -ENOMEM;

    // Fill in the DMA structure
    elem.buf = buf;
    elem.len = bcn->len;

    // Forward the information to the LMAC
    error = bl_send_bcn_change(bl_hw, vif->vif_index, elem.buf,
                                 bcn->len, bcn->head_len, bcn->tim_len, NULL);

    kfree(elem.buf);

    return error;
}

/**
 * * @stop_ap: Stop being an AP, including stopping beaconing.
 */
static int bl_cfg80211_stop_ap(struct wiphy *wiphy, struct net_device *dev)
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct bl_vif *bl_vif = netdev_priv(dev);
    struct bl_sta *sta;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)
    struct station_del_parameters params;
    params.mac = NULL;
#endif

    bl_send_apm_stop_req(bl_hw, bl_vif);
    bl_chanctx_unlink(bl_vif);

    /* delete any remaining STA*/
    while (!list_empty(&bl_vif->ap.sta_list)) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)
        bl_cfg80211_del_station(wiphy, dev, &params);
#else
        bl_cfg80211_del_station(wiphy, dev, NULL);
#endif
    }

    /* delete BC/MC STA */
    sta = &bl_hw->sta_table[bl_vif->ap.bcmc_index];
    bl_txq_vif_deinit(bl_hw, bl_vif);
    bl_del_bcn(&bl_vif->ap.bcn);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 12, 0)
    bl_del_csa(bl_vif);
#endif

    netif_tx_stop_all_queues(dev);
    netif_carrier_off(dev);

    netdev_info(dev, "AP Stopped");

    return 0;
}

/**
 * @set_monitor_channel: Set the monitor mode channel for the device. If other
 *	interfaces are active this callback should reject the configuration.
 *	If no interfaces are active or the device is down, the channel should
 *	be stored for when a monitor interface becomes active.
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0)
static int bl_cfg80211_set_monitor_channel(struct wiphy *wiphy,
                                             struct cfg80211_chan_def *chandef)
{
    return 0;
}
#endif

/**
 * @probe_client: probe an associated client, must return a cookie that it
 *	later passes to cfg80211_probe_status().
 */
int bl_cfg80211_probe_client(struct wiphy *wiphy, struct net_device *dev,
            const u8 *peer, u64 *cookie)
{
    return 0;
}

/**
 * @mgmt_frame_register: Notify driver that a management frame type was
 *	registered. Note that this callback may not sleep, and cannot run
 *	concurrently with itself.
 */
void bl_cfg80211_mgmt_frame_register(struct wiphy *wiphy,
                   struct wireless_dev *wdev,
                   struct mgmt_frame_regs *upd)
{
}

/**
 * @set_wiphy_params: Notify that wiphy parameters have changed;
 *	@changed bitfield (see &enum wiphy_params_flags) describes which values
 *	have changed. The actual parameter values are available in
 *	struct wiphy. If returning an error, no value should be changed.
 */
static int bl_cfg80211_set_wiphy_params(struct wiphy *wiphy, u32 changed)
{
    return 0;
}


/**
 * @set_tx_power: set the transmit power according to the parameters,
 *	the power passed is in mBm, to get dBm use MBM_TO_DBM(). The
 *	wdev may be %NULL if power was set for the wiphy, and will
 *	always be %NULL unless the driver supports per-vif TX power
 *	(as advertised by the nl80211 feature flag.)
 */
static int bl_cfg80211_set_tx_power(struct wiphy *wiphy,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
                                      struct wireless_dev *wdev,
#endif
                                      enum nl80211_tx_power_setting type, int mbm)
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct bl_vif *vif;
    s8 pwr;
    int res = 0;

    if (type == NL80211_TX_POWER_AUTOMATIC) {
        pwr = 0x7f;
    } else {
        pwr = MBM_TO_DBM(mbm);
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
    if (wdev) {
        vif = container_of(wdev, struct bl_vif, wdev);
        res = bl_send_set_power(bl_hw, vif->vif_index, pwr, NULL);
    } else
#endif
    {
        list_for_each_entry(vif, &bl_hw->vifs, list) {
            res = bl_send_set_power(bl_hw, vif->vif_index, pwr, NULL);
            if (res)
                break;
        }
    }

    return res;
}

static int bl_cfg80211_set_txq_params(struct wiphy *wiphy,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0)
                                      struct net_device *dev,
#endif
                                        struct ieee80211_txq_params *params)
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct bl_vif *bl_vif = netdev_priv(bl_hw->ndev);
    u8 hw_queue, aifs, cwmin, cwmax;
    u32 param;

    BL_DBG(BL_FN_ENTRY_STR);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0)
    hw_queue = bl_ac2hwq[0][params->ac];
#else
    hw_queue = bl_ac2hwq[0][params->queue];
#endif

    aifs  = params->aifs;
    cwmin = fls(params->cwmin);
    cwmax = fls(params->cwmax);

    /* Store queue information in general structure */
    param  = (u32) (aifs << 0);
    param |= (u32) (cwmin << 4);
    param |= (u32) (cwmax << 8);
    param |= (u32) (params->txop) << 12;

    /* Send the MM_SET_EDCA_REQ message to the FW */
    return bl_send_set_edca(bl_hw, hw_queue, param, false, bl_vif->vif_index);
}


/**
 * @remain_on_channel: Request the driver to remain awake on the specified
 *	channel for the specified duration to complete an off-channel
 *	operation (e.g., public action frame exchange). When the driver is
 *	ready on the requested channel, it must indicate this with an event
 *	notification by calling cfg80211_ready_on_channel().
 */
static int
bl_cfg80211_remain_on_channel(struct wiphy *wiphy,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0)
                                struct wireless_dev *wdev,
#else
                                struct net_device *dev,
#endif
                                struct ieee80211_channel *chan,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0)
                                enum nl80211_channel_type channel_type,
#endif
                                unsigned int duration, u64 *cookie)
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0)
    struct bl_vif *bl_vif = netdev_priv(wdev->netdev);
#else
    struct bl_vif *bl_vif = netdev_priv(dev);
#endif
    struct bl_roc_elem *roc_elem;
    int error;

    BL_DBG(BL_FN_ENTRY_STR);

    /* For debug purpose (use ftrace kernel option) */
    trace_roc(bl_vif->vif_index, chan->center_freq, duration);

    /* Check that no other RoC procedure has been launched */
    if (bl_hw->roc_elem) {
        return -EBUSY;
    }

    /* Allocate a temporary RoC element */
    roc_elem = kmalloc(sizeof(struct bl_roc_elem), GFP_KERNEL);

    /* Verify that element has well been allocated */
    if (!roc_elem) {
        return -ENOMEM;
    }

    /* Initialize the RoC information element */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0)
    roc_elem->wdev = wdev;
#else
    roc_elem->wdev = dev->ieee80211_ptr;
#endif
    roc_elem->chan = chan;
    roc_elem->duration = duration;
    roc_elem->mgmt_roc = false;
    roc_elem->on_chan = false;

    /* Initialize the OFFCHAN TX queue to allow off-channel transmissions */
    bl_txq_offchan_init(bl_vif);
    bl_hw->roc_elem = roc_elem;

    /* Forward the information to the FMAC */
    error = bl_send_roc(bl_hw, bl_vif, chan, duration);

    /* If no error, keep all the information for handling of end of procedure */
    if (error == 0) {

        /* Set the cookie value */
        *cookie = (u64)(bl_hw->roc_cookie_cnt);

    } else {
        /* Free the allocated element */
        kfree(roc_elem);
        bl_hw->roc_elem = NULL;
        bl_txq_offchan_deinit(bl_vif);
    }

    return error;
}

/**
 * @cancel_remain_on_channel: Cancel an on-going remain-on-channel operation.
 *	This allows the operation to be terminated prior to timeout based on
 *	the duration value.
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0)
static int bl_cfg80211_cancel_remain_on_channel(struct wiphy *wiphy,
                                                  struct wireless_dev *wdev,
                                                  u64 cookie)
#else
static int bl_cfg80211_cancel_remain_on_channel(struct wiphy *wiphy,
                                                  struct net_device *dev,
                                                  u64 cookie)
#endif
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0)
    struct bl_vif *bl_vif = netdev_priv(wdev->netdev);
#else
    struct bl_vif *bl_vif = netdev_priv(dev);
#endif

    BL_DBG(BL_FN_ENTRY_STR);

    /* For debug purpose (use ftrace kernel option) */
    trace_cancel_roc(bl_vif->vif_index);

    /* Check if a RoC procedure is pending */
    if (!bl_hw->roc_elem) {
        return 0;
    }

    /* Forward the information to the FMAC */
    return bl_send_cancel_roc(bl_hw);
}

/**
 * @dump_survey: get site survey information.
 */
static int bl_cfg80211_dump_survey(struct wiphy *wiphy, struct net_device *netdev,
                                     int idx, struct survey_info *info)
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct ieee80211_supported_band *sband;
    struct bl_survey_info *bl_survey;

    BL_DBG(BL_FN_ENTRY_STR);

    if (idx >= ARRAY_SIZE(bl_hw->survey))
        return -ENOENT;

    bl_survey = &bl_hw->survey[idx];

    // Check if provided index matches with a supported 2.4GHz channel
    sband = wiphy->bands[NL80211_BAND_2GHZ];
    if (sband && idx >= sband->n_channels) {
        idx -= sband->n_channels;
        sband = NULL;
		return -ENOENT;
    }

    // Fill the survey
    info->channel = &sband->channels[idx];
    info->filled = bl_survey->filled;

    if (bl_survey->filled != 0) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0)
        info->time = (u64)bl_survey->chan_time_ms;
        info->time_busy = (u64)bl_survey->chan_time_busy_ms;
#else
        info->channel_time = (u64)bl_survey->chan_time_ms;
        info->channel_time_busy = (u64)bl_survey->chan_time_busy_ms;
#endif
        info->noise = bl_survey->noise_dbm;

        // Set the survey report as not used
        bl_survey->filled = 0;
    }

    return 0;
}

/**
 * @get_channel: Get the current operating channel for the virtual interface.
 *	For monitor interfaces, it should return %NULL unless there's a single
 *	current monitoring channel.
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
static int bl_cfg80211_get_channel(struct wiphy *wiphy,
                                     struct wireless_dev *wdev,
                                     struct cfg80211_chan_def *chandef) {

    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct bl_vif *bl_vif = container_of(wdev, struct bl_vif, wdev);
    struct bl_chanctx *ctxt;

    if (!bl_vif->up ||
        !bl_chanctx_valid(bl_hw, bl_vif->ch_index)) {
        return -ENODATA;
    }

    ctxt = &bl_hw->chanctx_table[bl_vif->ch_index];
    *chandef = ctxt->chan_def;

    return 0;
}
#endif
/**
 * @mgmt_tx: Transmit a management frame.
 */
static int bl_cfg80211_mgmt_tx(struct wiphy *wiphy,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 6, 0)
        struct net_device *dev,
#else
        struct wireless_dev *wdev,
#endif //3.6
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 0)
        struct cfg80211_mgmt_tx_params *params,
#else //3.14
        struct ieee80211_channel *chan, bool offchan,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0)
        enum nl80211_channel_type channel_type,
        bool channel_type_valid,
#endif
        unsigned int wait, const u8 *buf, size_t len,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0)
        bool no_cck,
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 3, 0)
        bool dont_wait_for_ack,
#endif
#endif //3.14
        u64 *cookie)
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 6, 0)
    struct bl_vif *bl_vif = netdev_priv(dev);
#else
    struct bl_vif *bl_vif = netdev_priv(wdev->netdev);
#endif
    struct bl_sta *bl_sta;
    int error = 0;
    bool ap = false;
    bool offchan_i = false;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 0)
    struct ieee80211_channel *chan = params->chan;
    const u8 *buf = params->buf;
    size_t len = params->len;
    bool no_cck = params->no_cck;
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0)
    int n_csa_offsets = params->n_csa_offsets;
    const u16 *csa_offsets = params->csa_offsets;
    #endif
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 2, 0)
    bool no_cck;
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 0)
    struct ieee80211_mgmt *mgmt = (void *)params->buf;
#else
    struct ieee80211_mgmt *mgmt = (void *)buf;
#endif

    do
    {
        /* Check if provided VIF is an AP or a STA one */
        switch (BL_VIF_TYPE(bl_vif)) {
        case NL80211_IFTYPE_AP_VLAN:
            bl_vif = bl_vif->ap_vlan.master;
            /* fall through */
        case NL80211_IFTYPE_AP:
        case NL80211_IFTYPE_P2P_GO:
        case NL80211_IFTYPE_MESH_POINT:
            ap = true;
            break;
        case NL80211_IFTYPE_STATION:
        case NL80211_IFTYPE_P2P_CLIENT:
        default:
            break;
        }

        /* Get STA on which management frame has to be sent */
        bl_sta = bl_retrieve_sta(bl_hw, bl_vif, mgmt->da,
                                     mgmt->frame_control, ap);

        /* For debug purpose (use ftrace kernel option) */
        trace_mgmt_tx(chan ? chan->center_freq : 0,
                      bl_vif->vif_index,
                      (bl_sta) ? bl_sta->sta_idx : 0xFF,
                      mgmt);
	/* Not an AP interface sending frame to unknown STA:
	 * This is allowed for external authentication */
	if ((bl_vif->sta.flags & RWNX_STA_EXT_AUTH) && ieee80211_is_auth(mgmt->frame_control))
		goto send_frame;

        /* If AP, STA have to exist */
        if (!bl_sta) {
            if (!ap) {
                /* ROC is needed, check that channel parameters have been provided */
                if (!chan) {
                    error = -EINVAL;
                    break;
                }

                /* Check that a RoC is already pending */
                if (bl_hw->roc_elem) {
                    /* Get VIF used for current ROC */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 6, 0)
                    struct bl_vif *bl_roc_vif = netdev_priv(dev);
#else
                    struct bl_vif *bl_roc_vif = netdev_priv(bl_hw->roc_elem->wdev->netdev);
#endif

                    /* Check if RoC channel is the same than the required one */
                    if ((bl_hw->roc_elem->chan->center_freq != chan->center_freq)
                            || (bl_vif->vif_index != bl_roc_vif->vif_index)) {
                        error = -EINVAL;
                        break;
                    }
                } else {
                    u64 cookie;

                    /* Start a ROC procedure for 30ms */
                    error = bl_cfg80211_remain_on_channel(wiphy,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 6, 0)
                                dev,
#else
                                wdev,
#endif
                                chan,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0)
                                channel_type,
#endif
                                30, &cookie);

                    if (error) {
                        break;
                    }

                    /*
                     * Need to keep in mind that RoC has been launched internally in order to
                     * avoid to call the cfg80211 callback once expired
                     */
                    bl_hw->roc_elem->mgmt_roc = true;
                }

                offchan_i = true;
            }
        }
		
send_frame:
        /* Push the management frame on the TX path */
        error = bl_start_mgmt_xmit(bl_vif, bl_sta,
                                   buf, len, no_cck,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0)
                                   n_csa_offsets,
                                   csa_offsets,
#endif
                                   offchan_i, cookie); } while (0);

    return error;
}

/**
 * @update_ft_ies: Provide updated Fast BSS Transition information to the
 *	driver. If the SME is in the driver/firmware, this information can be
 *	used in building Authentication and Reassociation Request frames.
 */
static
int bl_cfg80211_update_ft_ies(struct wiphy *wiphy,
                            struct net_device *dev,
                            struct cfg80211_update_ft_ies_params *ftie)
{
    return 0;
}

/**
 * @set_cqm_rssi_config: Configure connection quality monitor RSSI threshold.
 */
static
int bl_cfg80211_set_cqm_rssi_config(struct wiphy *wiphy,
                                  struct net_device *dev,
                                  int32_t rssi_thold, uint32_t rssi_hyst)
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct bl_vif *bl_vif = netdev_priv(dev);

    return bl_send_cfg_rssi_req(bl_hw, bl_vif->vif_index, rssi_thold, rssi_hyst);
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 12, 0)
/**
 *
 * @channel_switch: initiate channel-switch procedure (with CSA). Driver is
 *	responsible for veryfing if the switch is possible. Since this is
 *	inherently tricky driver may decide to disconnect an interface later
 *	with cfg80211_stop_iface(). This doesn't mean driver can accept
 *	everything. It should do it's best to verify requests and reject them
 *	as soon as possible.
 */
int bl_cfg80211_channel_switch(struct wiphy *wiphy,
                                 struct net_device *dev,
                                 struct cfg80211_csa_settings *params)
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    struct bl_vif *vif = netdev_priv(dev);
    struct bl_dma_elem elem;
    struct bl_bcn *bcn, *bcn_after;
    struct bl_csa *csa;
    u16 csa_oft[BCN_MAX_CSA_CPT];
    u8 *buf;
    int i, error = 0;


    if (vif->ap.csa)
        return -EBUSY;


#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0)
    if (params->n_counter_offsets_beacon > BCN_MAX_CSA_CPT)
        return -EINVAL;
#endif

    /* Build the new beacon with CSA IE */
    bcn = &vif->ap.bcn;
    buf = bl_build_bcn(bcn, &params->beacon_csa);
    if (!buf)
        return -ENOMEM;

    memset(csa_oft, 0, sizeof(csa_oft));

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 15, 0)
    csa_oft[0] = params->counter_offset_beacon + bcn->head_len +
            bcn->tim_len;
#else
    for (i = 0; i < params->n_counter_offsets_beacon; i++)
    {
        csa_oft[i] = params->counter_offsets_beacon[i] + bcn->head_len +
            bcn->tim_len;
    }
#endif

    /* If count is set to 0 (i.e anytime after this beacon) force it to 2 */
    if (params->count == 0) {
        params->count = 2;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 15, 0)
        buf[csa_oft[0]] = 2;
#else
        for (i = 0; i < params->n_counter_offsets_beacon; i++)
        {
            buf[csa_oft[i]] = 2;
        }
#endif
    }

    elem.buf = buf;
    elem.len = bcn->len;

    /* Build the beacon to use after CSA. It will only be sent to fw once
       CSA is over. */
    csa = kzalloc(sizeof(struct bl_csa), GFP_KERNEL);
    if (!csa) {
        error = -ENOMEM;
        goto end;
    }


    bcn_after = &csa->bcn;
    buf = bl_build_bcn(bcn_after, &params->beacon_after);
    if (!buf) {
        error = -ENOMEM;
        bl_del_csa(vif);
        goto end;
    }

    vif->ap.csa = csa;
    csa->vif = vif;
    csa->chandef = params->chandef;
    csa->dma.buf = buf;
    csa->dma.len = bcn_after->len;

    /* Send new Beacon. FW will extract channel and count from the beacon */
    error = bl_send_bcn_change(bl_hw, vif->vif_index, elem.buf,
                                 bcn->len, bcn->head_len, bcn->tim_len, csa_oft);

    if (error) {
        bl_del_csa(vif);
        goto end;
    } else {
        INIT_WORK(&csa->work, bl_csa_finish);
        cfg80211_ch_switch_started_notify(dev, &csa->chandef, params->count, false);
    }

  end:
    kfree(elem.buf);

    return error;
}
#endif
/**
 * @change_bss: Modify parameters for a given BSS (mainly for AP mode).
 */
int bl_cfg80211_change_bss(struct wiphy *wiphy, struct net_device *dev,
                             struct bss_parameters *params)
{
    struct bl_vif *bl_vif = netdev_priv(dev);
    int res =  -EOPNOTSUPP;

    if (((BL_VIF_TYPE(bl_vif) == NL80211_IFTYPE_AP) ||
         (BL_VIF_TYPE(bl_vif) == NL80211_IFTYPE_P2P_GO)) &&
        (params->ap_isolate > -1)) {

        if (params->ap_isolate)
            bl_vif->ap.flags |= BL_AP_ISOLATE;
        else
            bl_vif->ap.flags &= ~BL_AP_ISOLATE;

        res = 0;
    }

    return res;
}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 5, 0)
int bl_cfg80211_set_channel(struct wiphy *wiphy, struct net_device *dev,
                   struct ieee80211_channel *chan,
                   enum nl80211_channel_type channel_type)
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);

    BL_DBG("Enter bl_cfg80211_set_channel\n");

    bl_hw->ndev = dev;
    bl_hw->chan = chan;
    bl_hw->channel_type = channel_type;

    return 0;
}
#endif

/* conversion table for legacy rate */
struct bl_legrate {
    s16 idx;
    u16 rate;  // in 100Kbps
};
const struct bl_legrate bl_legrates_lut[] = {
    [0]  = { .idx = 0,  .rate = 10 },
    [1]  = { .idx = 1,  .rate = 20 },
    [2]  = { .idx = 2,  .rate = 55 },
    [3]  = { .idx = 3,  .rate = 110 },
    [4]  = { .idx = -1, .rate = 0 },
    [5]  = { .idx = -1, .rate = 0 },
    [6]  = { .idx = -1, .rate = 0 },
    [7]  = { .idx = -1, .rate = 0 },
    [8]  = { .idx = 10, .rate = 480 },
    [9]  = { .idx = 8,  .rate = 240 },
    [10] = { .idx = 6,  .rate = 120 },
    [11] = { .idx = 4,  .rate = 60 },
    [12] = { .idx = 11, .rate = 540 },
    [13] = { .idx = 9,  .rate = 360 },
    [14] = { .idx = 7,  .rate = 180 },
    [15] = { .idx = 5,  .rate = 90 },
};

static int bl_fill_station_info(struct bl_sta *sta, struct bl_vif *vif,
                                  struct station_info *sinfo)
{
    struct bl_sta_stats *stats = &sta->stats;

    // Generic info
    //sinfo->generation = vif->generation;
    sinfo->signal = stats->last_rx.rssi1;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0)
    switch (stats->last_rx.ch_bw) {
        case PHY_CHNL_BW_20:
            sinfo->rxrate.bw = RATE_INFO_BW_20;
            break;
        case PHY_CHNL_BW_40:
            sinfo->rxrate.bw = RATE_INFO_BW_40;
            break;
        case PHY_CHNL_BW_80:
            sinfo->rxrate.bw = RATE_INFO_BW_80;
            break;
        case PHY_CHNL_BW_160:
            sinfo->rxrate.bw = RATE_INFO_BW_160;
            break;
#if 0
        default:
            sinfo->rxrate.bw = RATE_INFO_BW_HE_RU;
#endif
            break;
    }
#endif

    BL_DBG("format_mod:%d\n", stats->last_rx.format_mod);
    switch (stats->last_rx.format_mod) {
        case FORMATMOD_NON_HT:
        case FORMATMOD_NON_HT_DUP_OFDM:
            sinfo->txrate.flags = 0;
            sinfo->txrate.legacy = bl_legrates_lut[stats->last_rx.leg_rate].rate;
	    BL_DBG("leg_rate:%d, legacy:%d\n", stats->last_rx.leg_rate, sinfo->rxrate.legacy);
            break;
        case FORMATMOD_HT_MF:
        case FORMATMOD_HT_GF:
            sinfo->txrate.flags = RATE_INFO_FLAGS_MCS;
            if (stats->last_rx.short_gi)
                sinfo->txrate.flags |= RATE_INFO_FLAGS_SHORT_GI;
            sinfo->txrate.mcs = stats->last_rx.mcs;
	    BL_DBG("mcs:%d\n", sinfo->rxrate.mcs);
            break;
        case FORMATMOD_VHT:
            //TODO:
            break;
        default :
            return -EINVAL;
    }

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 0, 0)
    sinfo->filled = (STATION_INFO_INACTIVE_TIME |
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 9, 0)
                     STATION_INFO_RX_BYTES |
                     STATION_INFO_TX_BYTES |
#else
                     STATION_INFO_RX_BYTES64 |
                     STATION_INFO_TX_BYTES64 |
#endif
                     STATION_INFO_RX_PACKETS |
                     STATION_INFO_TX_PACKETS |
                     STATION_INFO_SIGNAL |
                     STATION_INFO_RX_BITRATE);
#else
    sinfo->filled = (BIT(NL80211_STA_INFO_INACTIVE_TIME) |
                     BIT(NL80211_STA_INFO_RX_BYTES64)    |
                     BIT(NL80211_STA_INFO_TX_BYTES64)    |
                     BIT(NL80211_STA_INFO_RX_PACKETS)    |
                     BIT(NL80211_STA_INFO_TX_PACKETS)    |
                     BIT(NL80211_STA_INFO_SIGNAL)        |
		     BIT(NL80211_STA_INFO_TX_BITRATE));
#endif

    return 0;
}

/**
 * @get_station: get station information for the station identified by @mac
 */
static int bl_cfg80211_get_station(struct wiphy *wiphy, struct net_device *dev,
                                     const u8 *mac, struct station_info *sinfo)
{
    struct bl_vif *vif = netdev_priv(dev);
    struct bl_sta *sta = NULL;


    if (BL_VIF_TYPE(vif) == NL80211_IFTYPE_MONITOR)
        return -EINVAL;
    else if ((BL_VIF_TYPE(vif) == NL80211_IFTYPE_STATION) ||
             (BL_VIF_TYPE(vif) == NL80211_IFTYPE_P2P_CLIENT)) {
        if (vif->sta.ap && ether_addr_equal(vif->sta.ap->mac_addr, mac))
            sta = vif->sta.ap;
    }
    else
    {
        struct bl_sta *sta_iter;
        list_for_each_entry(sta_iter, &vif->ap.sta_list, list) {
            if (sta_iter->valid && ether_addr_equal(sta_iter->mac_addr, mac)) {
                sta = sta_iter;
                break;
            }
        }
    }

    if (sta)
        return bl_fill_station_info(sta, vif, sinfo);

    return -EINVAL;
}

/**
 * @dump_station: dump station callback -- resume dump at index @idx
 */
static int bl_cfg80211_dump_station(struct wiphy *wiphy, struct net_device *dev,
                                      int idx, u8 *mac, struct station_info *sinfo)
{
    struct bl_vif *vif = netdev_priv(dev);
    struct bl_sta *sta = NULL;

    if (BL_VIF_TYPE(vif) == NL80211_IFTYPE_MONITOR)
        return -EINVAL;
    else if ((BL_VIF_TYPE(vif) == NL80211_IFTYPE_STATION) ||
             (BL_VIF_TYPE(vif) == NL80211_IFTYPE_P2P_CLIENT)) {
        if ((idx == 0) && vif->sta.ap && vif->sta.ap->valid)
            sta = vif->sta.ap;
    } else {
        struct bl_sta *sta_iter;
        int i = 0;
        list_for_each_entry(sta_iter, &vif->ap.sta_list, list) {
            if (i == idx) {
                sta = sta_iter;
                break;
            }
            i++;
        }
    }

    if (sta == NULL)
        return -ENOENT;

    /* Copy peer MAC address */
    memcpy(mac, &sta->mac_addr, ETH_ALEN);

    return bl_fill_station_info(sta, vif, sinfo);
}

static struct cfg80211_ops bl_cfg80211_ops = {
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 5, 0)
    .set_channel = bl_cfg80211_set_channel,
#endif
    .add_virtual_intf = bl_cfg80211_add_iface,
    .del_virtual_intf = bl_cfg80211_del_iface,
    .change_virtual_intf = bl_cfg80211_change_iface,
    .scan = bl_cfg80211_scan,
    .connect = bl_cfg80211_connect,
    .disconnect = bl_cfg80211_disconnect,
    .add_key = bl_cfg80211_add_key,
    .get_key = bl_cfg80211_get_key,
    .del_key = bl_cfg80211_del_key,
    .set_default_key = bl_cfg80211_set_default_key,
    .set_default_mgmt_key = bl_cfg80211_set_default_mgmt_key,
    .add_station = bl_cfg80211_add_station,
    .del_station = bl_cfg80211_del_station,
    .change_station = bl_cfg80211_change_station,
    .mgmt_tx = bl_cfg80211_mgmt_tx,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
    .start_ap = bl_cfg80211_start_ap,
    .change_beacon = bl_cfg80211_change_beacon,
    .stop_ap = bl_cfg80211_stop_ap,
#else
    .add_beacon = bl_cfg80211_start_ap,
    .set_beacon = bl_cfg80211_change_beacon,
    .del_beacon = bl_cfg80211_stop_ap,
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0)
    .set_monitor_channel = bl_cfg80211_set_monitor_channel,
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 3, 0)
    .probe_client = bl_cfg80211_probe_client,
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 8, 0)
    .mgmt_frame_register = bl_cfg80211_mgmt_frame_register,
#else
    .update_mgmt_frame_registrations = bl_cfg80211_mgmt_frame_register,
#endif
    .set_wiphy_params = bl_cfg80211_set_wiphy_params,
    .set_txq_params = bl_cfg80211_set_txq_params,
    .set_tx_power = bl_cfg80211_set_tx_power,
    .remain_on_channel = bl_cfg80211_remain_on_channel,
    .cancel_remain_on_channel = bl_cfg80211_cancel_remain_on_channel,
    .dump_survey = bl_cfg80211_dump_survey,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
    .get_channel = bl_cfg80211_get_channel,
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
    .update_ft_ies = bl_cfg80211_update_ft_ies,
#endif
    .set_cqm_rssi_config = bl_cfg80211_set_cqm_rssi_config,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 12, 0)
    .channel_switch = bl_cfg80211_channel_switch,
#endif
    .change_bss = bl_cfg80211_change_bss,
    .get_station = bl_cfg80211_get_station,
    .dump_station = bl_cfg80211_dump_station,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0) || defined(BL_WPA3_COMPAT)
    .external_auth = bl_cfg80211_external_auth,
#endif
};


/*********************************************************************
 * Init/Exit functions
 *********************************************************************/
static void bl_wdev_unregister(struct bl_hw *bl_hw)
{
    struct bl_vif *bl_vif, *tmp;

    rtnl_lock();
    list_for_each_entry_safe(bl_vif, tmp, &bl_hw->vifs, list) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0)
        bl_cfg80211_del_iface(bl_hw->wiphy, &bl_vif->wdev);
#else
        bl_cfg80211_del_iface(bl_hw->wiphy, bl_vif->wdev.netdev);
#endif
    }
    rtnl_unlock();
}

static void bl_set_vers(struct bl_hw *bl_hw)
{
    u32 vers = bl_hw->version_cfm.version_lmac;

    BL_DBG(BL_FN_ENTRY_STR);

    snprintf(bl_hw->wiphy->fw_version,
             sizeof(bl_hw->wiphy->fw_version), "%d.%d.%d.%d",
             (vers & (0xff << 24)) >> 24, (vers & (0xff << 16)) >> 16,
             (vers & (0xff <<  8)) >>  8, (vers & (0xff <<  0)) >>  0);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 9, 0)
static int bl_reg_notifier(struct wiphy *wiphy, struct regulatory_request *request)
#else
static void bl_reg_notifier(struct wiphy *wiphy, struct regulatory_request *request)
#endif
{
    struct bl_hw *bl_hw = wiphy_priv(wiphy);
    if (bl_mod_params.mp_mode) 
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 9, 0)
        return 0;
#else
        return;
#endif

    // For now trust all initiator
    bl_send_me_chan_config_req(bl_hw);
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 9, 0)
    return 0;
#endif
}

static int remap_pfn_open(struct inode *inode, struct file *file)
{
     struct mm_struct *mm = current->mm;
 
     printk("client: %s (%d)\n", current->comm, current->pid);
     printk("code  section: [0x%lx   0x%lx]\n", mm->start_code, mm->end_code);
     printk("data  section: [0x%lx   0x%lx]\n", mm->start_data, mm->end_data);
     printk("brk   section: s: 0x%lx, c: 0x%lx\n", mm->start_brk, mm->brk);
     printk("mmap  section: s: 0x%lx\n", mm->mmap_base);
     printk("stack section: s: 0x%lx\n", mm->start_stack);
     printk("arg   section: [0x%lx   0x%lx]\n", mm->arg_start, mm->arg_end);
     printk("env   section: [0x%lx   0x%lx]\n", mm->env_start, mm->env_end);
 
     return 0;
}

static int remap_pfn_mmap(struct file *file, struct vm_area_struct *vma)
{
    unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
    unsigned long pfn_start = (virt_to_phys(kbuff) >> PAGE_SHIFT) + vma->vm_pgoff;
    unsigned long virt_start = (unsigned long)kbuff + offset;
    unsigned long size = vma->vm_end - vma->vm_start;
    int ret = 0;

    printk("phy: 0x%lx, offset: 0x%lx, size: 0x%lx\n", pfn_start << PAGE_SHIFT, offset, size);

    ret = remap_pfn_range(vma, vma->vm_start, pfn_start, size, vma->vm_page_prot);
    if (ret)
        printk("%s: remap_pfn_range failed at [0x%lx  0x%lx]\n",
            __func__, vma->vm_start, vma->vm_end);
    else
        printk("%s: map 0x%lx to 0x%lx, size: 0x%lx\n", __func__, virt_start,
            vma->vm_start, size);

    return ret;
}

static const struct file_operations remap_pfn_fops = {
    .owner = THIS_MODULE,
    .open = remap_pfn_open,
    .mmap = remap_pfn_mmap,
};

static struct miscdevice remap_pfn_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "bl_log",
    .fops = &remap_pfn_fops,
};


int bl_logbuf_create(struct bl_hw *bl_hw)
{
	int ret = 0;

	kbuff = kzalloc(LOG_BUF_SIZE, GFP_KERNEL);
	bl_hw->log_buff = kbuff;

	device_create_file(bl_hw->dev, &dev_attr_filter_severity);
	device_create_file(bl_hw->dev, &dev_attr_filter_module);
	
	ret = misc_register(&remap_pfn_misc);

	return ret;
}

void bl_logbuf_destroy(struct bl_hw *bl_hw)
{
	kfree(kbuff);

	device_remove_file(bl_hw->dev, &dev_attr_filter_severity);
	device_remove_file(bl_hw->dev, &dev_attr_filter_module);

    misc_deregister(&remap_pfn_misc);    
}

#define  BL_WD_PEROID   msecs_to_jiffies(50)

static void bl_watchdog_timer(struct bl_hw *bl_hw, bool active)
{
	/* Totally stop the timer */
	if (!active && bl_hw->watchdog_active) {
		del_timer_sync(&bl_hw->timer);
		bl_hw->watchdog_active = false;
		return;
	}

	if (active) {
		if (!bl_hw->watchdog_active) {
			/* Create timer */
			bl_hw->timer.expires = jiffies + BL_WD_PEROID;
			add_timer(&bl_hw->timer);
			bl_hw->watchdog_active = true;
		} else {
			/* Rerun the timer */
			mod_timer(&bl_hw->timer, jiffies + BL_WD_PEROID);
		}
	}
}


static int
bl_watchdog_thread(void *data)
{
    struct bl_hw *bl_hw = (struct bl_hw *)data;
	int wait;
	static int i = 0;
	int tx_cnt, txed_cnt;
	
	allow_signal(SIGTERM);
	while (1) {
		if (kthread_should_stop())
			break;
		BL_DBG("watch dog thread\n");
		wait = wait_for_completion_interruptible(&bl_hw->watchdog_wait);

		if(bl_hw->surprise_removed)
			break;

		// tx statistic for one second
		i++;
		if(i % 20 == 0){
			tx_cnt = bl_hw->tx_cnt - bl_hw->last_tx_cnt;
			txed_cnt = bl_hw->txed_cnt - bl_hw->last_txed_cnt;
			bl_hw->last_tx_cnt = bl_hw->tx_cnt;
			bl_hw->last_txed_cnt = bl_hw->txed_cnt;
			bl_tcp_ack_statistic(bl_hw);
			BL_DBG("one sencond xmit cnt %d txed cnt %d\n", tx_cnt, txed_cnt);
		}

        if (i % 6000 == 0)
            bl_send_heart_beat_detect_req(bl_hw);
		
		// TODO: consider sleep later
        if (!wait) {
         // do some checking here
            if(bl_hw->last_int_cnt == bl_hw->int_cnt){
				bl_get_interrupt_status(bl_hw, 1);

                if(bl_hw->int_status) 
                    bl_queue_main_work(bl_hw);
			}
			bl_hw->last_int_cnt = bl_hw->int_cnt;
			reinit_completion(&bl_hw->watchdog_wait);
		} else
			break;
	}
	return 0;
}


static void
bl_watchdog(struct timer_list *t)
{
    struct bl_hw *bl_hw = from_timer(bl_hw, t, timer);

	if (bl_hw->watchdog_task) {
		complete(&bl_hw->watchdog_wait);
		/* Reschedule the watchdog */
		if (bl_hw->watchdog_active && !bl_hw->surprise_removed)
			mod_timer(&bl_hw->timer,
				  jiffies + BL_WD_PEROID);
	}
}

int bl_cfg80211_init(struct bl_plat *bl_plat, void **platform_data)
{
    struct bl_hw *bl_hw;
    struct bl_conf_file init_conf;
    int ret = 0;
    struct wiphy *wiphy;
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 6, 0)
    struct net_device *ndev;
#else
    struct wireless_dev *wdev;
#endif
    struct vif_params vif_params = {.use_4addr = 1};
    struct bl_iface_tbl nl_iface_table[BL_OPMODE_MAX_NUM] = {
                            {1, NL80211_IFTYPE_STATION, "wlan%d",  NULL},
                            {0, NL80211_IFTYPE_AP,      "ap%d",    NULL} };
    u8_l nl_iface_num = 1;
	u8_l zero_addr[6] =  { 0, 0, 0, 0, 0, 0 };
    int i,j;
    struct netlink_kernel_cfg nl_cfg = {
        .groups = BL_NL_BCAST_GROUP_ID,
    };



    BL_DBG(BL_FN_ENTRY_STR);

    /* create a new wiphy for use with cfg80211 */
    wiphy = wiphy_new(&bl_cfg80211_ops, sizeof(struct bl_hw));

    if (!wiphy) {
        dev_err(bl_platform_get_dev(bl_plat), "Failed to create new wiphy\n");
        ret = -ENOMEM;
        goto err_out;
    }

    bl_hw = wiphy_priv(wiphy);
    bl_hw->wiphy = wiphy;
    bl_hw->plat = bl_plat;
    bl_hw->dev = bl_platform_get_dev(bl_plat);
    bl_hw->mod_params = &bl_mod_params;

    /* set device pointer for wiphy */
    set_wiphy_dev(wiphy, bl_hw->dev);

    /* Create cache to allocate sw_txhdr */
    bl_hw->sw_txhdr_cache = kmem_cache_create("bl_sw_txhdr_cache",
                                                sizeof(struct bl_sw_txhdr),
                                                0, 0, NULL);
    if (!bl_hw->sw_txhdr_cache) {
        wiphy_err(wiphy, "Cannot allocate cache for sw TX header\n");
        ret = -ENOMEM;
        goto err_cache;
    }

	bl_hw->agg_reodr_pkt_cache= kmem_cache_create("bl_agg_reodr_pkt_cache",
                                                sizeof(struct bl_agg_reord_pkt),
                                                0, 0, NULL);
    if (!bl_hw->agg_reodr_pkt_cache) {
        wiphy_err(wiphy, "Cannot allocate cache for agg reorder packet\n");
        ret = -ENOMEM;
        goto err_reodr;
    }

#ifdef CONFIG_BL_MP
    bl_hw->iwp_var.iwpriv_ind = NULL;
    if (bl_mod_params.mp_mode) {
        bl_hw->iwp_var.tx_duty = DEFAULT_TX_DUTY;
        bl_hw->iwp_var.iwpriv_ind = kzalloc(IWPRIV_IND_LEN_MAX+1, GFP_KERNEL);
        bl_hw->iwp_var.iwpriv_ind_len = 0;
        if (!bl_hw->iwp_var.iwpriv_ind) {
            BL_DBG("allocate iwpriv_ind fail \n");
            ret = -ENOMEM;
            goto err_config;
        }
    }
#endif
    bl_hw->mpa_rx_data.buf_ptr = kzalloc(BL_RX_DATA_BUF_SIZE_16K + DMA_ALIGNMENT, GFP_KERNEL|GFP_DMA);
    if(!bl_hw->mpa_rx_data.buf_ptr){
        BL_DBG("allocate rx buf fail \n");
        ret = -ENOMEM;
        goto err_config;
	}
	bl_hw->mpa_rx_data.buf = (u8 *)ALIGN_ADDR(bl_hw->mpa_rx_data.buf_ptr, DMA_ALIGNMENT);

    bl_hw->mpa_tx_data.buf_ptr = kzalloc(BL_TX_DATA_BUF_SIZE_16K + DMA_ALIGNMENT, GFP_KERNEL|GFP_DMA);	
	if(!bl_hw->mpa_tx_data.buf_ptr){
        BL_DBG("allocate tx buf fail \n");
        ret = -ENOMEM;
        goto err_alloc;	
    }
	bl_hw->mpa_tx_data.buf = (u8 *)ALIGN_ADDR(bl_hw->mpa_tx_data.buf_ptr, DMA_ALIGNMENT);
    bl_hw->msg_skb = __netdev_alloc_skb(NULL, 2048, GFP_KERNEL); //dev_alloc_skb(2048);
    if (!bl_hw->msg_skb) {
		/*alloc again*/
        bl_hw->msg_skb = __netdev_alloc_skb(NULL, 2048, GFP_KERNEL);
		if (!bl_hw->msg_skb)
            printk("pre-alloc msg skb fail\n");
	}

#ifdef CONFIG_BL_DNLD_FWBIN
    if ((ret = bl_parse_configfile(bl_hw, BL_CONFIG_FW_NAME, &init_conf))) {
        wiphy_err(wiphy, "bl_parse_configfile failed\n");
    }
#endif
    bl_hw->sec_phy_chan.band = NL80211_BAND_5GHZ;
    bl_hw->sec_phy_chan.type = PHY_CHNL_BW_20;
    bl_hw->sec_phy_chan.prim20_freq = 5500;
    bl_hw->sec_phy_chan.center_freq1 = 5500;
    bl_hw->sec_phy_chan.center_freq2 = 0;
    bl_hw->vif_started = 0;
    bl_hw->adding_sta = false;


    for (i = 0; i < NX_VIRT_DEV_MAX + NX_REMOTE_STA_MAX; i++)
        bl_hw->avail_idx_map |= BIT(i);

    bl_hwq_init(bl_hw);

    for (i = 0; i < NX_NB_TXQ; i++) {
        bl_hw->txq[i].idx = TXQ_INACTIVE;
    }
	
    /* Initialize RoC element pointer to NULL, indicate that RoC can be started */
    bl_hw->roc_elem = NULL;
    /* Cookie can not be 0 */
    bl_hw->roc_cookie_cnt = 1;

    /* set mac addr from config file */
    memcpy(wiphy->perm_addr, init_conf.mac_addr, ETH_ALEN);
    wiphy->mgmt_stypes = bl_default_mgmt_stypes;
    wiphy->bands[NL80211_BAND_2GHZ] = &bl_band_2GHz;
    wiphy->interface_modes =
        BIT(NL80211_IFTYPE_STATION)     |
        BIT(NL80211_IFTYPE_AP)          |
        BIT(NL80211_IFTYPE_AP_VLAN)     |
        BIT(NL80211_IFTYPE_P2P_CLIENT)  |
        BIT(NL80211_IFTYPE_P2P_GO);
    wiphy->flags |= 
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 3, 0)
        WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL |
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 12, 0)
        WIPHY_FLAG_HAS_CHANNEL_SWITCH |
#endif
        WIPHY_FLAG_4ADDR_STATION |
        WIPHY_FLAG_4ADDR_AP;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0)
    wiphy->max_num_csa_counters = BCN_MAX_CSA_CPT;
#endif
    wiphy->max_remain_on_channel_duration = bl_hw->mod_params->roc_dur_max;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 3, 0)
    wiphy->features |= NL80211_FEATURE_SK_TX_STATUS;
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
    wiphy->features |= NL80211_FEATURE_NEED_OBSS_SCAN |
        NL80211_FEATURE_SK_TX_STATUS |
        NL80211_FEATURE_VIF_TXPOWER;
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0)
    wiphy->features |= NL80211_FEATURE_AP_MODE_CHAN_WIDTH_CHANGE;
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0) || defined(BL_WPA3_COMPAT)
    wiphy->features |= NL80211_FEATURE_SAE;
#endif

    wiphy->iface_combinations   = bl_combinations;
    /* -1 not to include combination with radar detection, will be re-added in
       bl_handle_dynparams if supported */
    wiphy->n_iface_combinations = ARRAY_SIZE(bl_combinations) - 1;
    wiphy->reg_notifier = bl_reg_notifier;

    wiphy->signal_type = CFG80211_SIGNAL_TYPE_MBM;

    wiphy->cipher_suites = cipher_suites;
    wiphy->n_cipher_suites = ARRAY_SIZE(cipher_suites) - NB_RESERVED_CIPHER;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)
    bl_hw->ext_capa[0] = WLAN_EXT_CAPA1_EXT_CHANNEL_SWITCHING;
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0)
    bl_hw->ext_capa[7] = WLAN_EXT_CAPA8_OPMODE_NOTIF;

    wiphy->extended_capabilities = bl_hw->ext_capa;
    wiphy->extended_capabilities_mask = bl_hw->ext_capa;
    wiphy->extended_capabilities_len = ARRAY_SIZE(bl_hw->ext_capa);
#endif
	bl_hw->workqueue = alloc_workqueue("BL_WORK_QUEUE", WQ_HIGHPRI | WQ_MEM_RECLAIM | WQ_UNBOUND, 1);
	if (!bl_hw->workqueue) {	
		printk("creat workqueue failed\n");
		goto err_platon;
	}
	INIT_WORK(&bl_hw->main_work, main_wq_hdlr);

	bl_hw->rx_workqueue = alloc_workqueue("BL_RXWORK_QUEUE", WQ_HIGHPRI | WQ_MEM_RECLAIM | WQ_UNBOUND, 1);
	if (!bl_hw->rx_workqueue) {	
		printk("creat rx workqueue failed\n");
		goto err_workqueue;
	}
	INIT_WORK(&bl_hw->rx_work, bl_rx_wq_hdlr);

    INIT_LIST_HEAD(&bl_hw->vifs);

    mutex_init(&bl_hw->dbginfo.mutex);
    spin_lock_init(&bl_hw->tx_lock);
	spin_lock_init(&bl_hw->main_proc_lock);
	spin_lock_init(&bl_hw->int_lock);
	spin_lock_init(&bl_hw->cmd_lock);
	spin_lock_init(&bl_hw->resend_lock);
	spin_lock_init(&bl_hw->txq_lock);
	spin_lock_init(&bl_hw->cb_lock);
	spin_lock_init(&bl_hw->rx_lock);
	spin_lock_init(&bl_hw->rx_process_lock);	
	bl_hw->bl_processing = false;
	bl_hw->more_task_flag = false;
	bl_hw->cmd_sent = false;
	bl_hw->resend = false;
	bl_hw->recovery_flag = false;
	bl_hw->msg_idx = 1;
	bl_hw->data_sent = false;    
    bl_hw->surprise_removed = false;
	bl_hw->rx_processing = false;
	bl_hw->more_rx_task_flag = false;
	bl_hw->rx_pkt_cnt = 0;
	bl_hw->last_hwq = false;

    if (bl_hw->mod_params->tcp_ack_filter) {
        spin_lock_init(&bl_hw->tcp_ack_lock);
        INIT_LIST_HEAD(&bl_hw->tcp_stream_list);
    }

#ifdef BL_RX_REORDER
	for(i = 0; i < (NX_REMOTE_STA_MAX + NX_VIRT_DEV_MAX); i++){
		for(j = 0; j < NX_NB_TID_PER_STA; j++){			
			memset(&bl_hw->rx_reorder[i][j], 0, sizeof(struct rxreorder_list));
			spin_lock_init(&bl_hw->rx_reorder[i][j].rx_lock);
			bl_hw->rx_reorder[i][j].bl_hw = bl_hw;
		}
	}
#else
	for(i = 0; i < (NX_REMOTE_STA_MAX + NX_VIRT_DEV_MAX)*NX_NB_TID_PER_STA; i++){
		INIT_LIST_HEAD(&bl_hw->reorder_list[i]);
	}
#endif
	for(i = 0; i < NX_NB_TXQ; i++)
		skb_queue_head_init(&bl_hw->transmitted_list[i]);

	skb_queue_head_init(&bl_hw->rx_pkt_list);
#ifdef  BL_RX_DEFRAG
	memset(rx_defrag_pool, 0, sizeof(rx_defrag_pool));
#endif

	ret = bl_platform_on(bl_hw);
    if (ret)
        goto err_workqueue1;

	*platform_data = bl_hw;
	sdio_set_drvdata(bl_hw->plat->func, bl_hw);

	//bl_logbuf_create(bl_hw);
    //TODO add magic char 'L' to header file
    BL_DBG("Notify FW to start...\n");
    bl_write_reg(bl_hw, 0x60, 'L'); 


    if (!bl_mod_params.mp_mode)
    {
        /* Reset FW */
        if ((ret = bl_send_reset(bl_hw)))
            goto err_lmac_reqs;
		
		if(bl_hw->mod_params->cal_data_cfg) {
			printk("caldata file : %s\n",bl_hw->mod_params->cal_data_cfg);
			bl_caldata_cfg_file_handle(bl_hw, bl_hw->mod_params->cal_data_cfg);
		}

        if ((ret = bl_send_version_req(bl_hw, &bl_hw->version_cfm)))
            goto err_lmac_reqs;

        /* set mac address reading from efuse */
        if(memcmp(bl_hw->version_cfm.mac,zero_addr,ETH_ALEN))
            memcpy(wiphy->perm_addr, bl_hw->version_cfm.mac, ETH_ALEN);
    }


    /* create fake version_cfm, due to mp_test firmware does not handle this request */
    if (bl_mod_params.mp_mode) {
        bl_hw->version_cfm.version_lmac = 0x5040000;
        bl_hw->version_cfm.version_machw_1 = 0x55fb;
        bl_hw->version_cfm.version_machw_2 = 0x1b3;
        bl_hw->version_cfm.version_phy_1 = 0x822111;
        bl_hw->version_cfm.version_phy_2 = 0x0;
        bl_hw->version_cfm.features = 0x8adf;
    }

    /* set mac address from insmod param */
    if(bl_hw->mod_params->wifi_mac) {
        if (sscanf(bl_hw->mod_params->wifi_mac, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", 
            &zero_addr[0], &zero_addr[1], &zero_addr[2], &zero_addr[3], &zero_addr[4], &zero_addr[5]) == ETH_ALEN)
            memcpy(wiphy->perm_addr, zero_addr, ETH_ALEN);
        else
            printk("module param MAC format wrong, should be wifi_mac=xx:xx:xx:xx:xx:xx\n");
    }
	
	if(bl_hw->mod_params->wifi_mac_cfg &&  \
		bl_parse_mac_config(bl_hw->mod_params->wifi_mac_cfg, &init_conf))			
		memcpy(wiphy->perm_addr, init_conf.mac_addr, ETH_ALEN);

    printk("MAC Address is:\n%pM\n", wiphy->perm_addr);

    bl_set_vers(bl_hw);

    if ((ret = bl_handle_dynparams(bl_hw, bl_hw->wiphy)))
        goto err_lmac_reqs;

    if (!bl_mod_params.mp_mode)
    {
        /* Set parameters to firmware */
        bl_send_me_config_req(bl_hw);
    }

    if ((ret = wiphy_register(wiphy))) {
        wiphy_err(wiphy, "Could not register wiphy device\n");
        goto err_register_wiphy;
    }

    if (!bl_mod_params.mp_mode) 
    {
        /* Set channel parameters to firmware (must be done after WiPHY registration) */
        bl_send_me_chan_config_req(bl_hw);
    }
	
    *platform_data = bl_hw;

	//bl_logbuf_create(bl_hw);
	
#ifdef CONFIG_BL_DEBUGFS
    bl_dbgfs_register(bl_hw, "bl");
#endif

    if (bl_hw->mod_params->opmode == BL_OPMODE_COCURRENT) {
        nl_iface_num++;
        nl_iface_table[BL_OPMODE_AP].valid = 1;
    } else if (bl_hw->mod_params->opmode == BL_OPMODE_REPEATER) {
        nl_iface_num++;
        nl_iface_table[BL_OPMODE_AP].valid = 1;
        nl_iface_table[BL_OPMODE_STA].param = &vif_params;
    }

    for (i = 0; i < nl_iface_num; i++) {
        printk("iface_idx=%d, valid=%d, name=%s, type=%d, param=%p\n", 
            i, nl_iface_table[i].valid, nl_iface_table[i].name, nl_iface_table[i].type, nl_iface_table[i].param);

        rtnl_lock();
        /* Add an initial station interface */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 6, 0)
        ndev = bl_interface_add(bl_hw, nl_iface_table[i].name, nl_iface_table[i].type, nl_iface_table[i].param);
#else
        wdev = bl_interface_add(bl_hw, nl_iface_table[i].name, nl_iface_table[i].type, nl_iface_table[i].param);
#endif
        rtnl_unlock();

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 6, 0)
        if (!ndev) {
#else
        if (!wdev) {
#endif
            wiphy_err(wiphy, "Failed to instantiate a network device\n");
            ret = -ENOMEM;
            goto err_add_interface;
        }

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 6, 0)
        wiphy_info(wiphy, "New interface create %s", ndev->name);
#else
        wiphy_info(wiphy, "New interface create %s", wdev->netdev->name);
#endif
    }

    /* create watch dog thread and timer */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
    timer_setup(&bl_hw->timer, bl_watchdog, 0);
#else
    init_timer(&bl_hw->timer);
    bl_hw->timer.function = bl_watchdog;
    bl_hw->timer.data = (void *)&bl_hw->timer;
#endif

    init_completion(&bl_hw->watchdog_wait);
    bl_hw->watchdog_task = kthread_run(bl_watchdog_thread,
                    bl_hw, "bl_wdog/%s",
                    dev_name(&bl_hw->plat->func->dev));
    if (IS_ERR(bl_hw->watchdog_task)) {
        pr_warn("bl watch thread failed to start\n");
        bl_hw->watchdog_task = NULL;
    }

    bl_watchdog_timer(bl_hw, true);

    bl_hw->netlink_sock_num = BL_NL_SOCKET_NUM;
    bl_hw->netlink_sock = netlink_kernel_create(&init_net, bl_hw->netlink_sock_num, &nl_cfg);
    if (bl_hw->netlink_sock) {
        printk("creat netlink socket num = %d\n", bl_hw->netlink_sock_num);
    }

    return 0;

err_add_interface:
    wiphy_unregister(bl_hw->wiphy);
err_register_wiphy:
err_lmac_reqs:
    bl_platform_off(bl_hw);
err_workqueue:
    if(bl_hw->workqueue)
        destroy_workqueue(bl_hw->workqueue);
err_workqueue1:
    if(bl_hw->rx_workqueue)
        destroy_workqueue(bl_hw->rx_workqueue);
err_platon:
    kfree(bl_hw->mpa_tx_data.buf_ptr);
err_alloc:
    kfree(bl_hw->mpa_rx_data.buf_ptr);
err_config:
    kmem_cache_destroy(bl_hw->agg_reodr_pkt_cache);
err_reodr:
    kmem_cache_destroy(bl_hw->sw_txhdr_cache);
err_cache:
    wiphy_free(wiphy);
err_out:
    if (bl_mod_params.mp_mode) {
        if (bl_hw->iwp_var.iwpriv_ind) {
            kfree(bl_hw->iwp_var.iwpriv_ind);
        }
    }

    return ret;
}

void bl_cfg80211_deinit(struct bl_hw *bl_hw)
{
    struct sk_buff *skb;
    int i = 0;

    BL_DBG(BL_FN_ENTRY_STR);

	/* Stop watchdog task */
	if (bl_hw->watchdog_task) {
		send_sig(SIGTERM, bl_hw->watchdog_task, 1);
		kthread_stop(bl_hw->watchdog_task);
		bl_hw->watchdog_task = NULL;
	}
	bl_watchdog_timer(bl_hw, false);

    if (bl_mod_params.mp_mode) 
        kfree(bl_hw->iwp_var.iwpriv_ind);

	//bl_logbuf_destroy(bl_hw);
    kfree(bl_hw->mpa_rx_data.buf_ptr);
	bl_hw->mpa_rx_data.buf_ptr = NULL;
    bl_hw->mpa_rx_data.buf = NULL;
    kfree(bl_hw->mpa_tx_data.buf_ptr);
    bl_hw->mpa_tx_data.buf_ptr = NULL;
	bl_hw->mpa_tx_data.buf = NULL;
    if (bl_hw->msg_skb)
       dev_kfree_skb_any(bl_hw->msg_skb);
    bl_hw->msg_skb = NULL;

    while (skb_queue_len(&bl_hw->rx_pkt_list)) {
        skb = skb_dequeue(&bl_hw->rx_pkt_list);
        if (skb)
            dev_kfree_skb_any(skb);
    }

    for(i = 0; i < NX_NB_TXQ; i++) {
        while (skb_queue_len(&bl_hw->transmitted_list[i])) {
            skb = skb_dequeue(&bl_hw->transmitted_list[i]);
            if (skb)
                dev_kfree_skb_any(skb);
        }
    }

    if(bl_hw->mod_params->tcp_ack_filter)
        bl_tcp_ack_stream_clear(bl_hw);

#ifdef CONFIG_BL_DEBUGFS
    bl_dbgfs_unregister(bl_hw);
#endif

    bl_wdev_unregister(bl_hw);
    wiphy_unregister(bl_hw->wiphy);

//	msleep(5000);

    if(bl_hw->workqueue){
        flush_workqueue(bl_hw->workqueue);
        destroy_workqueue(bl_hw->workqueue);
        bl_hw->workqueue = NULL;
    }
    if(bl_hw->rx_workqueue){
        flush_workqueue(bl_hw->rx_workqueue);
        destroy_workqueue(bl_hw->rx_workqueue);
        bl_hw->rx_workqueue = NULL;
    }

    netlink_kernel_release(bl_hw->netlink_sock);
    bl_hw->netlink_sock_num = 0;

    bl_platform_off(bl_hw);
    kmem_cache_destroy(bl_hw->sw_txhdr_cache);
	kmem_cache_destroy(bl_hw->agg_reodr_pkt_cache);
    wiphy_free(bl_hw->wiphy);
}

static int __init bl_mod_init(void)
{
    BL_DBG(BL_FN_ENTRY_STR);
    bl_print_version();

    return bl_platform_register_drv();
}

static void __exit bl_mod_exit(void)
{
    BL_DBG(BL_FN_ENTRY_STR);

    bl_platform_unregister_drv();
}

module_init(bl_mod_init);
module_exit(bl_mod_exit);

MODULE_FIRMWARE(BL_CONFIG_FW_NAME);

MODULE_DESCRIPTION(RW_DRV_DESCRIPTION);
MODULE_VERSION(RELEASE_VERSION);
MODULE_AUTHOR(RW_DRV_COPYRIGHT " " RW_DRV_AUTHOR);
MODULE_LICENSE("GPL");
