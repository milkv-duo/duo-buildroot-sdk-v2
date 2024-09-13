/**
 ******************************************************************************
 *
 *  @file bl_rx.c
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
#include <linux/dma-mapping.h>
#include <linux/ieee80211.h>
#include <linux/etherdevice.h>

#include "bl_defs.h"
#include "bl_rx.h"
#include "bl_tx.h"
#include "bl_ftrace.h"
#include "bl_compat.h"

const u8 legrates_lut[] = {
    0,                          /* 0 */
    1,                          /* 1 */
    2,                          /* 2 */
    3,                          /* 3 */
    -1,                         /* 4 */
    -1,                         /* 5 */
    -1,                         /* 6 */
    -1,                         /* 7 */
    10,                         /* 8 */
    8,                          /* 9 */
    6,                          /* 10 */
    4,                          /* 11 */
    11,                         /* 12 */
    9,                          /* 13 */
    7,                          /* 14 */
    5                           /* 15 */
};


/**
 * bl_rx_statistic - save some statistics about received frames
 *
 * @bl_hw: main driver data.
 * @hw_rxhdr: Rx Hardware descriptor of the received frame.
 * @sta: STA that sent the frame.
 */
static void bl_rx_statistic(struct bl_hw *bl_hw, struct hw_rxhdr *hw_rxhdr,
                              struct bl_sta *sta)
{
#ifdef CONFIG_BL_DEBUGFS
    struct bl_stats *stats = &bl_hw->stats;
    struct bl_rx_rate_stats *rate_stats = &sta->stats.rx_rate;
    struct hw_vect *hwvect = &hw_rxhdr->hwvect;
    int mpdu, ampdu, mpdu_prev, rate_idx;


    /* update ampdu rx stats */
    mpdu = hwvect->mpdu_cnt;
    ampdu = hwvect->ampdu_cnt;
    mpdu_prev = stats->ampdus_rx_map[ampdu];

    if (mpdu_prev < mpdu ) {
        stats->ampdus_rx_miss += mpdu - mpdu_prev - 1;
    } else {
        stats->ampdus_rx[mpdu_prev]++;
    }
    stats->ampdus_rx_map[ampdu] = mpdu;

    /* update rx rate statistic */
    if (hwvect->format_mod > FORMATMOD_NON_HT_DUP_OFDM) {
        int mcs = hwvect->mcs;
        int bw = hwvect->ch_bw;
        int sgi = hwvect->short_gi;
        int nss;
        if (hwvect->format_mod <= FORMATMOD_HT_GF) {
            nss = hwvect->stbc ? hwvect->stbc : hwvect->n_sts;
            rate_idx = 16 + nss * 32 + mcs * 4 +  bw * 2 + sgi;
        } else {
            nss = hwvect->stbc ? hwvect->stbc/2 : hwvect->n_sts;
            rate_idx = 144 + nss * 80 + mcs * 8 + bw * 2 + sgi;
        }
    } else {
        int idx = legrates_lut[hwvect->leg_rate];
        if (idx < 4) {
            rate_idx = idx * 2 + hwvect->pre_type;
        } else {
            rate_idx = 8 + idx - 4;
        }
    }
    if (rate_idx < rate_stats->size) {
        rate_stats->table[rate_idx]++;
        rate_stats->cpt++;
    } else {
        BL_DBG("Invalid index conversion => %d / %d\n", rate_idx, rate_stats->size);
    }
#endif

    /* save complete hwvect */
    sta->stats.last_rx = hw_rxhdr->hwvect;

}
							  
  static const u16 bl_1d_to_wmm_queue[8] = { 1, 0, 0, 1, 2, 2, 3, 3 };

u16 bl_recal_priority_netdevidx(struct bl_vif *bl_vif, struct sk_buff *skb)
{
    struct bl_hw *bl_hw = bl_vif->bl_hw;
    struct wireless_dev *wdev = &bl_vif->wdev;
    struct bl_sta *sta = NULL;
    struct bl_txq *txq;
    u16 netdev_queue;
    bool tdls_mgmgt_frame = false;

	BL_DBG(BL_FN_ENTRY_STR);

    switch (wdev->iftype) {
    case NL80211_IFTYPE_STATION:
    {
        struct ethhdr *eth;
        eth = (struct ethhdr *)skb->data;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0)
        if (eth->h_proto == cpu_to_be16(ETH_P_TDLS)) {
            tdls_mgmgt_frame = true;
        }
#endif
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
    case NL80211_IFTYPE_AP:
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
        skb->priority = 0xAA;
        netdev_queue = NX_BCMC_TXQ_NDEV_IDX;
    }

    BUG_ON(netdev_queue >= NX_NB_NDEV_TXQ);
	
    if(skb->priority < 8)
        return bl_1d_to_wmm_queue[skb->priority];
	else
        return 0;

//    return netdev_queue;
}

/**
 * bl_rx_data_skb - Process one data frame
 *
 * @bl_hw: main driver data
 * @bl_vif: vif that received the buffer
 * @skb: skb received
 * @rxhdr: HW rx descriptor
 * @return: true if buffer has been forwarded to upper layer
 *
 * If buffer is amsdu , it is first split into a list of skb.
 * Then each skb may be:
 * - forwarded to upper layer
 * - resent on wireless interface
 *
 * When vif is a STA interface, every skb is only forwarded to upper layer.
 * When vif is an AP interface, multicast skb are forwarded and resent, whereas
 * skb for other BSS's STA are only resent.
 */
static bool bl_rx_data_skb(struct bl_hw *bl_hw, struct bl_vif *bl_vif,
                             struct sk_buff *skb,  struct hw_rxhdr *rxhdr)
{
    struct sk_buff_head list;
    struct sk_buff *rx_skb;
    struct bl_sta *sta = NULL;
    u16 netdev_queue;
    bool amsdu = rxhdr->flags_is_amsdu;
    bool resend = false, forward = true;

	if(!bl_vif)
        return false;
	
    skb->dev = bl_vif->ndev;

    __skb_queue_head_init(&list);

    if (bl_hw->mod_params->pn_check && rxhdr->flags_is_pn_check) {
        if (bl_vif->rx_pn && (bl_vif->rx_pn + 1 != rxhdr->pn)) {
            printk("pn_check miss: stid%d,sn%d,fn%d,tid%d. pn:%lld~%lld\n", rxhdr->flags_sta_idx, rxhdr->sn, rxhdr->fn, rxhdr->tid, rxhdr->pn, bl_vif->rx_pn);
            bl_vif->rx_pn = rxhdr->pn;
            return false;
        } else {
            bl_vif->rx_pn = rxhdr->pn;
        }
    }

    if (amsdu) {
        int count;
        struct ethhdr *eth;
        unsigned char rfc1042_header[] = { 0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00 };

        /* For FFD 5.2.1/5.8.1. mitigate A-MSDU aggregation injection attacks */
        eth = (struct ethhdr *)(skb->data);
        if (ether_addr_equal(eth->h_dest, rfc1042_header)) {
            BL_DBG("%s drop amsdu with llc head\n", __func__);
            return false;
        }

        ieee80211_amsdu_to_8023s(skb, &list, bl_vif->ndev->dev_addr,
                                 BL_VIF_TYPE(bl_vif), 0, NULL, NULL);

        count = skb_queue_len(&list);
        if (count > ARRAY_SIZE(bl_hw->stats.amsdus_rx))
            count = ARRAY_SIZE(bl_hw->stats.amsdus_rx);
        if (count >=1 && count <= ARRAY_SIZE(bl_hw->stats.amsdus_rx))
            bl_hw->stats.amsdus_rx[count - 1]++;
        else			
            bl_hw->stats.amsdus_rx[0]++;
    } else {
        bl_hw->stats.amsdus_rx[0]++;
        __skb_queue_head(&list, skb);
    }

    if (((BL_VIF_TYPE(bl_vif) == NL80211_IFTYPE_AP) ||
         (BL_VIF_TYPE(bl_vif) == NL80211_IFTYPE_AP_VLAN) ||
         (BL_VIF_TYPE(bl_vif) == NL80211_IFTYPE_P2P_GO)) &&
        !(bl_vif->ap.flags & BL_AP_ISOLATE)) {
        const struct ethhdr *eth;
        rx_skb = skb_peek(&list);
        if(rx_skb == NULL)
            printk("rx_skb=NULL!!!\n");
		/*go through mac_header, after mac_header is eth header*/
        skb_reset_mac_header(rx_skb);
        eth = eth_hdr(rx_skb);

        if (unlikely(is_multicast_ether_addr(eth->h_dest))) {
            /* broadcast pkt need to be forwared to upper layer and resent
               on wireless interface */
			BL_DBG("this is a broadcast packet, forwared true and resend true!\n");
            resend = true;
        } else {
            /* unicast pkt for STA inside the BSS, no need to forward to upper
               layer simply resend on wireless interface */
            if (rxhdr->flags_dst_idx != 0xFF)
            {
				BL_DBG("unicast pkt for STA inside the BSS, and forward false and resend true\n");
                sta = &bl_hw->sta_table[rxhdr->flags_dst_idx];
                if (sta->valid && (sta->vlan_idx == bl_vif->vif_index))
                {
                    forward = false;
                    resend = true;
                }
            }
        }
    } else if (BL_VIF_TYPE(bl_vif) == NL80211_IFTYPE_MESH_POINT) {
        const struct ethhdr *eth;
        rx_skb = skb_peek(&list);
        skb_reset_mac_header(rx_skb);
        eth = eth_hdr(rx_skb);

        if (!is_multicast_ether_addr(eth->h_dest)) {
            /* unicast pkt for STA inside the BSS, no need to forward to upper
               layer simply resend on wireless interface */
            if (rxhdr->flags_dst_idx != 0xFF)
            {
                forward = false;
                resend = true;
            }
        }
    }

    /* Repeater mode, drop Rx broadcast loopback data from STA iface, to avoid confuse to kernel bridge fdb table */
    if ((bl_hw->mod_params->opmode == BL_OPMODE_REPEATER) && (BL_VIF_TYPE(bl_vif) == NL80211_IFTYPE_STATION)) {
        int i = 0;
        struct ethhdr *eth = NULL;
        rx_skb = skb_peek(&list);
        if(rx_skb != NULL) {
            skb_reset_mac_header(rx_skb);
            eth = eth_hdr(rx_skb);
            if (is_multicast_ether_addr(eth->h_dest) || is_broadcast_ether_addr(eth->h_dest)) {
                for (i = 0; i < NX_REMOTE_STA_MAX; i++) {
                    sta = &bl_hw->sta_table[i];
                    if (sta->valid && (BL_VIF_TYPE(bl_hw->vif_table[sta->vif_idx]) == NL80211_IFTYPE_AP)
                            && (memcmp(&eth->h_source, &sta->mac_addr, 6) == 0)) {
                        forward = false;
                        BL_DBG("Repeater mode: STA inface drop data which src_addr is AP connected STA's\n");
                        break;
                    }
                }
            }
        } else {
            printk("rx_skb=NULL!\n");
        }
    }

    /* STA mode, drop frame with SA=self */
    if ((bl_hw->mod_params->opmode == BL_OPMODE_STA) && (BL_VIF_TYPE(bl_vif) == NL80211_IFTYPE_STATION)) {
        struct ethhdr *eth = NULL;
        u16 eth_type = 0;
        rx_skb = skb_peek(&list);
        if(rx_skb != NULL) {
            skb_reset_mac_header(rx_skb);
            eth = eth_hdr(rx_skb);
            eth_type = ntohs(((struct ethhdr *)eth)->h_proto);

            if ((eth_type == ETH_P_ARP || eth_type == ETH_P_IPV6)
                && (memcmp(&eth->h_source, bl_hw->wiphy->perm_addr, 6) == 0)) {
                forward = false;
                BL_DBG("%s drop loopback 0x%x\r\n", __func__, eth_type);
            }
        }
    }

    while (!skb_queue_empty(&list)) {
        rx_skb = __skb_dequeue(&list);
		BL_DBG("resend rx_skb->priority=%d, ndev_idx=%d\n", rx_skb->priority, skb_get_queue_mapping(rx_skb));

        /* resend pkt on wireless interface */
        if (resend) {
            struct sk_buff *skb_copy;
            /* always need to copy buffer when forward=0 to get enough headrom for tsdesc */
            skb_copy = skb_copy_expand(rx_skb, sizeof(struct bl_txhdr) + sizeof(struct txdesc_api) + sizeof(struct sdio_hdr) +
                                       BL_SWTXHDR_ALIGN_SZ, 0, GFP_ATOMIC);
            if (skb_copy) {
                int res;
                skb_copy->protocol = htons(ETH_P_802_3);
                skb_reset_network_header(skb_copy);
                skb_reset_mac_header(skb_copy);

				BL_DBG("resend skb_copy=%p, skb len=0x%02x, skb->priority=%d, ndev_idx=%d\n", skb_copy, skb_copy->len, skb_copy->priority, skb_get_queue_mapping(skb_copy));

                bl_vif->is_resending = true;

				//netdev_queue = bl_recal_priority_netdevidx(bl_vif, skb_copy);

				//skb_set_queue_mapping(skb_copy, netdev_queue);

				BL_DBG("after recal: skb->priority=%d, netdev_queue=%d\n", skb_copy->priority, netdev_queue);

				//bl_hw->resend = true;
                res = dev_queue_xmit(skb_copy);
                //res = bl_requeue_multicast_skb(skb_copy, bl_vif);
                bl_vif->is_resending = false;
                /* note: buffer is always consummed by dev_queue_xmit */
                if (res == NET_XMIT_DROP) {
                    bl_vif->net_stats.rx_dropped++;
                    bl_vif->net_stats.tx_dropped++;
                } else if (res != NET_XMIT_SUCCESS) {
                    netdev_err(bl_vif->ndev,
                               "Failed to re-send buffer to driver (res=%d)",
                               res);
                    bl_vif->net_stats.tx_errors++;
                }
            } else {
                netdev_err(bl_vif->ndev, "Failed to copy skb");
            }
        }

        /* forward pkt to upper layer */
        if (forward) {
            rx_skb->protocol = eth_type_trans(rx_skb, bl_vif->ndev);
            memset(rx_skb->cb, 0, sizeof(rx_skb->cb));
/*
	if (in_interrupt())
	    netif_rx(rx_skb);
	else
	    netif_rx_ni(rx_skb);*/

            local_bh_disable();
            netif_receive_skb(rx_skb);
            local_bh_enable();


            /* Update statistics */
            bl_vif->net_stats.rx_packets++;
            bl_vif->net_stats.rx_bytes += rx_skb->len;
			BL_DBG("forward this packet to network layer success\n");
        }
    }

    return forward;
}

/**
 * bl_rx_mgmt - Process one 802.11 management frame
 *
 * @bl_hw: main driver data
 * @bl_vif: vif that received the buffer
 * @skb: skb received
 * @rxhdr: HW rx descriptor
 *
 * Process the management frame and free the corresponding skb
 */
static void bl_rx_mgmt(struct bl_hw *bl_hw, struct bl_vif *bl_vif,
                         struct sk_buff *skb,  struct hw_rxhdr *hw_rxhdr)
{
    bool handled = 0;
    struct ieee80211_mgmt *mgmt = (struct ieee80211_mgmt *)skb->data;

    trace_mgmt_rx(hw_rxhdr->phy_prim20_freq, hw_rxhdr->flags_vif_idx,
                  hw_rxhdr->flags_sta_idx, mgmt);

    if (ieee80211_is_beacon(mgmt->frame_control)) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 3, 0)
        cfg80211_report_obss_beacon(bl_hw->wiphy, skb->data, skb->len,
                                    hw_rxhdr->phy_prim20_freq,
                                    hw_rxhdr->hwvect.rssi1, GFP_ATOMIC);
        handled = 1;
#endif
    }

    if ((ieee80211_is_deauth(mgmt->frame_control) ||
                ieee80211_is_disassoc(mgmt->frame_control)) &&
               (mgmt->u.deauth.reason_code == WLAN_REASON_CLASS2_FRAME_FROM_NONAUTH_STA ||
                mgmt->u.deauth.reason_code == WLAN_REASON_CLASS3_FRAME_FROM_NONASSOC_STA)) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 11, 0)
        cfg80211_rx_unprot_mlme_mgmt(bl_vif->ndev, skb->data, skb->len);
#else
        if (ieee80211_is_deauth(mgmt->frame_control))
            cfg80211_send_unprot_deauth(bl_vif->ndev, skb->data, skb->len);
        else
            cfg80211_send_unprot_disassoc(bl_vif->ndev, skb->data, skb->len);
#endif
        handled = 1;
    }

    if ((BL_VIF_TYPE(bl_vif) == NL80211_IFTYPE_STATION) &&
               (ieee80211_is_action(mgmt->frame_control) &&
                (mgmt->u.action.category == 6))) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
        struct cfg80211_ft_event_params ft_event;
        ft_event.target_ap = (uint8_t *)&mgmt->u.action + ETH_ALEN + 2;
        ft_event.ies = (uint8_t *)&mgmt->u.action + ETH_ALEN*2 + 2;
        ft_event.ies_len = hw_rxhdr->hwvect.len - (ft_event.ies - (uint8_t *)mgmt);
        ft_event.ric_ies = NULL;
        ft_event.ric_ies_len = 0;
        cfg80211_ft_event(bl_vif->ndev, &ft_event);
        handled = 1;
#endif
    }

    if (!handled) {
        cfg80211_rx_mgmt(
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 6, 0)
            &bl_vif->wdev,
#else
            bl_vif->wdev.netdev,
#endif
            hw_rxhdr->phy_prim20_freq,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
            hw_rxhdr->hwvect.rssi1,
#endif
            skb->data, skb->len, 
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 0)
            0);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3, 12, 0)
            0, GFP_ATOMIC);
#else
            GFP_ATOMIC);
#endif
    }

    dev_kfree_skb(skb);
}

/**
 * bl_rx_get_vif - Return pointer to the destination vif
 *
 * @bl_hw: main driver data
 * @vif_idx: vif index present in rx descriptor
 *
 * Select the vif that should receive this frame returns NULL if the destination
 * vif is not active.
 * If no vif is specified, this is probably a mgmt broadcast frame, so select
 * the first active interface
 */
static inline
struct bl_vif *bl_rx_get_vif(struct bl_hw * bl_hw, int vif_idx)
{
    struct bl_vif *bl_vif = NULL;

    if (vif_idx == 0xFF) {
        list_for_each_entry(bl_vif, &bl_hw->vifs, list) {
            if (bl_vif->up)
                return bl_vif;
        }
        return NULL;
    } else if (vif_idx < NX_VIRT_DEV_MAX) {
        bl_vif = bl_hw->vif_table[vif_idx];
        if (!bl_vif || !bl_vif->up)
            return NULL;
    }

    return bl_vif;
}

/**
 * bl_rx_packet_dispatch - dispatch the packet
 *
 * @bl_hw: Pointer to bl_hw
 * @skb: Pointer to the skb
 *
 * This function is called for rx packet dispatch
 *
 */
u8 bl_rx_packet_dispatch(struct bl_hw *bl_hw, struct sk_buff *skb)
{
    struct hw_rxhdr *hw_rxhdr;
    struct bl_vif *bl_vif;

    int msdu_offset = sizeof(struct hw_rxhdr);

	hw_rxhdr = (struct hw_rxhdr *)skb->data;
	bl_vif = bl_rx_get_vif(bl_hw, hw_rxhdr->flags_vif_idx);
	if (!bl_vif) {
		dev_err(bl_hw->dev, "Frame received but no active vif (%d)",
				hw_rxhdr->flags_vif_idx);
		dev_kfree_skb(skb);
		goto end;
	}
	BL_DBG("enter %s SN %d\n", __func__, hw_rxhdr->sn);

	/*Now, skb->data pointed to the real payload*/
	skb_pull(skb, msdu_offset);
			
    if ((hw_rxhdr->flags_sta_idx != 0xff) && (hw_rxhdr->flags_sta_idx < (NX_REMOTE_STA_MAX + NX_VIRT_DEV_MAX))) {
        struct bl_sta *sta= &bl_hw->sta_table[hw_rxhdr->flags_sta_idx];

        bl_rx_statistic(bl_hw, hw_rxhdr, sta);

        if (sta->vlan_idx != bl_vif->vif_index) {
            bl_vif = bl_hw->vif_table[sta->vlan_idx];
            if (!bl_vif) {
				printk("cannot find the vif\n");
                dev_kfree_skb(skb);
                goto end;
                }
           }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 3, 0)
           if (hw_rxhdr->flags_is_4addr && !bl_vif->use_4addr) {
                    cfg80211_rx_unexpected_4addr_frame(bl_vif->ndev,
                                                       sta->mac_addr, GFP_ATOMIC);
            }
#endif
        }

		BL_DBG("original skb->priority=%d\n", skb->priority);
        skb->priority = 256 + hw_rxhdr->flags_user_prio;
		BL_DBG("hw_rxhdr->flags_user_prio=%d, skb->priority=%d, ndev_idx=%d\n", hw_rxhdr->flags_user_prio, skb->priority, skb_get_queue_mapping(skb));
        if (!bl_rx_data_skb(bl_hw, bl_vif, skb, hw_rxhdr)) {
            dev_kfree_skb(skb);
        }
    
    end:

    return 0;
}

#ifdef  BL_RX_REORDER

/**
 * bl_rx_reorder_dispatch_until_start_win - dispatch the reorder list until the start win
 *
 * @bl_hw: Pointer to struct bl_hw
 * @reorder_list: Pointer to the reorder list
 * @start_win: new start win
 *
 * This function is called for rx reorder packet dispatch
 *
 */
 void bl_rx_reorder_dispatch_until_start_win(struct bl_hw *bl_hw, struct rxreorder_list *reorder_list, u16 start_win)
{
    struct sk_buff *skb;
    u8 index;
	u16 count_to_send;

	if(start_win >= reorder_list->start_win)
		count_to_send = (start_win - reorder_list->start_win);
	else
		count_to_send = (start_win + MAX_SEQ_VALUE - reorder_list->start_win)%MAX_SEQ_VALUE;

    if(count_to_send > RX_WIN_SIZE)
		count_to_send = RX_WIN_SIZE;
	
    for (index = 0; index < count_to_send; index++){
		//spin_lock_irq(&reorder_list->rx_lock);
        skb = (struct sk_buff *)reorder_list->reorder_pkt[(index + reorder_list->start_win_index)%RX_WIN_SIZE];
        reorder_list->reorder_pkt[(index + reorder_list->start_win_index)%RX_WIN_SIZE] = NULL;		
		//spin_unlock_irq(&reorder_list->rx_lock);

		if(skb)
            bl_rx_packet_dispatch(bl_hw, skb);
	}

	//update start_win and index
	reorder_list->start_win = start_win % MAX_SEQ_VALUE;
	reorder_list->start_win_index =(reorder_list->start_win_index + count_to_send)% RX_WIN_SIZE;	
	reorder_list->end_win = (reorder_list->start_win + RX_WIN_SIZE -1) % MAX_SEQ_VALUE;
}


/**
 * bl_rx_reorder_dispatch_until_start_win - dispatch the reorder list until the start win
 *
 * @bl_hw: Pointer to struct bl_hw
 * @reorder_list: Pointer to the reorder list
 *
 * This function is called for rx reorder packet dispatch
 *
 */
 void bl_rx_reorder_scan_and_dispatch(struct bl_hw *bl_hw, struct rxreorder_list *reorder_list)
{
    struct sk_buff *skb;
    u8 index;

	BL_DBG("enter %s\n", __func__);

    for (index = 0; index < RX_WIN_SIZE; index++){		
		//spin_lock_irq(&reorder_list->rx_lock);
        skb = (struct sk_buff *)reorder_list->reorder_pkt[(index + reorder_list->start_win_index)%RX_WIN_SIZE];
        reorder_list->reorder_pkt[(index + reorder_list->start_win_index)%RX_WIN_SIZE] = NULL;			
		//spin_unlock_irq(&reorder_list->rx_lock);
        if(skb)
            bl_rx_packet_dispatch(bl_hw, skb);
        else
            break;
	}

	//update start_win and index
	reorder_list->start_win = (reorder_list->start_win + index) % MAX_SEQ_VALUE;
	reorder_list->start_win_index =(reorder_list->start_win_index + index)% RX_WIN_SIZE;	
	reorder_list->end_win = (reorder_list->start_win + RX_WIN_SIZE -1) % MAX_SEQ_VALUE;
}

/**
 * bl_rx_flush_all - flush all the packet
 *
 * @bl_hw: Pointer to struct bl_hw
 * @reorder_list: Pointer to the reorder list
 *
 * This function is called for rx reorder packet dispatch
 *
 */
 void bl_rx_flush_all(struct bl_hw *bl_hw, struct rxreorder_list *reorder_list)
 {
     struct sk_buff *skb;
     u8 i;
 	/* flush all rx packet */
	for(i=0; i< RX_WIN_SIZE; i++){
		//spin_lock_irq(&rxreorder_list->rx_lock);
         skb = reorder_list->reorder_pkt[(reorder_list->start_win_index+i)%RX_WIN_SIZE];
		 reorder_list->reorder_pkt[(reorder_list->start_win_index+i)%RX_WIN_SIZE] = NULL;		 
		// spin_unlock_irq(&rxreorder_list->rx_lock);
		 if(skb)
		 	bl_rx_packet_dispatch(bl_hw, skb);
	}
 }
/**
 * bl_rx_reorder_flush - callback function of reorder timer
 *
 * @t: Pointer to struct timer_list
 * 
 *
 * This function is callback function of reorder timer
 *
 */
void bl_rx_reorder_flush(struct timer_list *t)
{
    struct rxreorder_list *reorder_list = from_timer(reorder_list, t, timer);

	if(reorder_list->flag)
		reorder_list->flush = true;
	reorder_list->is_timer_set = false;
	reorder_list->bl_hw->flush_rx = true;	
	queue_work(reorder_list->bl_hw->rx_workqueue, &reorder_list->bl_hw->rx_work);
}

#define  BL_REORDER_PEROID   msecs_to_jiffies(60)
/**
 * bl_rx_reorder_timer_set - set reorder timer
 *
 * @reorder_list: Pointer to struct rxreorder_list
 * 
 *
 * This function is to set reorder timer
 *
 */
void bl_rx_reorder_timer_set(struct rxreorder_list *reorder_list)
{
    if(reorder_list->is_timer_set){
        if(in_irq() || in_atomic() || irqs_disabled())
		del_timer(&reorder_list->timer);
		else    
        del_timer_sync(&reorder_list->timer);
    }

	mod_timer(&reorder_list->timer, jiffies + BL_REORDER_PEROID);

	reorder_list->is_timer_set = true;
}
#endif
void bl_rx_wq_hdlr(struct work_struct *work)
{
	struct bl_hw *bl_hw = container_of(work, struct bl_hw, rx_work);
	struct sk_buff *skb = NULL;
	int i, j, k;

	BL_DBG(BL_FN_ENTRY_STR);
	if(bl_hw->surprise_removed)
		goto exit;
	
	spin_lock_irq(&bl_hw->rx_process_lock);
	if(bl_hw->rx_processing){
		bl_hw->more_rx_task_flag = true;
	    spin_unlock_irq(&bl_hw->rx_process_lock);
		goto exit;
	} else {
		bl_hw->rx_processing = true;		
		spin_unlock_irq(&bl_hw->rx_process_lock);
	}

start:
	while (true){
#ifdef  BL_RX_REORDER
		if(bl_hw->flush_rx){			
			for(i = 0; i < (NX_REMOTE_STA_MAX + NX_VIRT_DEV_MAX); i++){
				for(j = 0; j < NX_NB_TID_PER_STA; j++) {
					if(bl_hw->rx_reorder[i][j].flush) {
						struct rxreorder_list *reorder_list = &bl_hw->rx_reorder[i][j];
						reorder_list->flush = false;
						reorder_list->last_seq = reorder_list->start_win;
						for(k = RX_WIN_SIZE -1; k > 0 ; k--)
							if(reorder_list->reorder_pkt[(reorder_list->start_win_index + k)%RX_WIN_SIZE]){
								reorder_list->last_seq = (reorder_list->start_win + k) % MAX_SEQ_VALUE;
								break;
							}							
						if(reorder_list->last_seq != reorder_list->start_win)
						bl_rx_reorder_dispatch_until_start_win(bl_hw, reorder_list, reorder_list->last_seq);
					}
					if(bl_hw->rx_reorder[i][j].del_ba) {
						bl_hw->rx_reorder[i][j].del_ba = false;
						bl_rx_flush_all(bl_hw, &bl_hw->rx_reorder[i][j]);
					}
				}
			}
			bl_hw->flush_rx = false;
		}
#endif
		skb = NULL;
		//spin_lock_irq(&bl_hw->rx_lock);
		if(skb_queue_len(&bl_hw->rx_pkt_list)){
			skb = skb_dequeue(&bl_hw->rx_pkt_list);
		}
		//spin_unlock_irq(&bl_hw->rx_lock);
		if(skb)
			bl_rxdataind(bl_hw, skb);
		else
			break;
	}
	
	spin_lock_irq(&bl_hw->rx_process_lock);
	if (bl_hw->more_rx_task_flag) {
		bl_hw->more_rx_task_flag = false;
	    spin_unlock_irq(&bl_hw->rx_process_lock);
		goto start;
	}
	bl_hw->rx_processing = false;
	spin_unlock_irq(&bl_hw->rx_process_lock);


exit:		
	return;
}

#ifdef  BL_RX_DEFRAG
/* Maximum time we can wait for a fragment (in ms) */
#define BL_DEFRAG_MAX_WAIT        (msecs_to_jiffies(100))

/* Pool of re-defrag structures */
struct rxdefrag_list rx_defrag_pool[BL_RX_DEFRAG_POOL_SIZE];

/**
 * bl_rx_defrag_timer_set - set defrag timer
 *
 * @reorder_list: Pointer to struct rxdefrag_list
 * 
 *
 * This function is to set defrag timer
 *
 */
void bl_rx_defrag_timer_set(struct rxdefrag_list *p_defrag)
{
	BL_DBG("%s \n", __func__);

    if(p_defrag->is_timer_set){
        if(in_irq() || in_atomic() || irqs_disabled())
		del_timer(&p_defrag->timer);
		else    
        del_timer_sync(&p_defrag->timer);
    }

	mod_timer(&p_defrag->timer, jiffies + BL_DEFRAG_MAX_WAIT);

	p_defrag->is_timer_set = true;
}

void bl_rx_defrag_clean(struct bl_hw * bl_hw)
{
    u8_l defrag_pool_cnt = 0, defrag_skb_cnt = 0;
    struct rxdefrag_list * p_defrag;
    while (defrag_pool_cnt < BL_RX_DEFRAG_POOL_SIZE) {
        defrag_skb_cnt = 0;
        p_defrag = &rx_defrag_pool[defrag_pool_cnt];
        if (p_defrag->pkt[0] && p_defrag->bl_hw == bl_hw)
            while ((defrag_skb_cnt < p_defrag->next_fn)
                    && (defrag_skb_cnt < DEFRAG_SKB_CNT)) {
                if(p_defrag->pkt[defrag_skb_cnt]) {
                    dev_kfree_skb((struct sk_buff *)p_defrag->pkt[defrag_skb_cnt]);
                    p_defrag->pkt[defrag_skb_cnt] = NULL;
                }
                defrag_skb_cnt++;
            }
        defrag_pool_cnt++;
    }
    memset(rx_defrag_pool, 0, sizeof(rx_defrag_pool));
}

/**
 * bl_rx_defrag_timeout_cb - callback function of rxdefrag timer
 *
 * @t: Pointer to struct timer_list
 * 
 *
 * This function is callback function of rxdefrag timer
 *
 */
static void bl_rx_defrag_timeout_cb(struct timer_list *t)
{
	struct bl_hw *bl_hw;
	struct sk_buff *skb;
    struct rxdefrag_list *p_defrag = from_timer(p_defrag, t, timer);

	BL_DBG("%s \n", __func__);

	if (!p_defrag)
		return;

	bl_hw = p_defrag->bl_hw;
	skb = (struct sk_buff *)p_defrag->pkt[0];
	/* directly dispatch */
	if (skb)
		bl_rx_packet_dispatch(bl_hw, skb);
}

/**
 * bl_rx_expand_pkt - allocate a large skb to store de-fraged frames
 *
 * @bl_hw: Pointer to struct bl_hw 
 * @p_defrag: Pointer to struct rxdefrag_list
 * 
 *
 * This function is to expand the first fragment skb to a larger one
 *
 */
static void bl_rx_expand_pkt(struct bl_hw *bl_hw, struct rxdefrag_list *p_defrag)
{
	u16 alloc_sz;
	struct sk_buff *skb, *new_skb;
	u8	*data_ptr;

	if (!p_defrag)
		return;

	/* get first fragment */
	skb = (struct sk_buff *)p_defrag->pkt[0];

	/* alloc new skb */
	alloc_sz = p_defrag->frame_len;

	new_skb = dev_alloc_skb(alloc_sz);
	if (!new_skb) {
		printk("%s: failed to alloc skb", __func__);
		return;
	} else {
        //printk("alloc skb: %p\n", skb);
    }

	/* copy first fragment to new skb */
	data_ptr = (u8 *)skb_put(new_skb, skb->len);
	memcpy(data_ptr, skb->data, skb->len);

	/* free original first fragment */
	if(skb)
		dev_kfree_skb_any(skb);	

	/* attach new skb to defrag list */
	p_defrag->pkt[0] = new_skb;	

}
/**
 * bl_rx_defrag_pkt - de-frag all the fragment frames
 *
 * @bl_hw: Pointer to struct bl_hw 
 * @p_defrag: Pointer to struct rxdefrag_list
 * @payload_offset: the offset of ethernet payload in the skb data buffer 
 *
 * This function is to defrag all fragment frames into the new larger skb buffer
 *
 */
static int bl_rx_defrag_pkt(struct bl_hw *bl_hw, struct rxdefrag_list *p_defrag, int payload_offset)
{
	struct sk_buff *skb;	
	u8	cur_fn = 0;
	u8	*data_ptr;
    int msdu_offset = payload_offset;	

	if (!p_defrag)
		return -1;

	/* expand first fragment to a large memory */
	bl_rx_expand_pkt(bl_hw, p_defrag);

	/* skip eth hdr len */
	msdu_offset += sizeof(struct ethhdr);
	
	while (cur_fn < (p_defrag->next_fn-1)) {

		cur_fn++;
		
		skb = (struct sk_buff *)p_defrag->pkt[cur_fn];

		/* copy the 2nd~n fragment frame's payload to the first fragment */
		skb_pull(skb, msdu_offset);	/* skip msdu offset*/
		
		/* copy to tail of first fragment */
		data_ptr = (u8 *)skb_put((struct sk_buff *)p_defrag->pkt[0], skb->len);

		if (data_ptr == NULL) {
			break;
		}

		memcpy(data_ptr, skb->data, skb->len);

		/* recover original fragment skb and free it */
		skb_push(skb, msdu_offset);		
		if (skb)
			dev_kfree_skb_any(skb);	

	};

	return 0;
}
/**
 * bl_rx_defrag_alloc - allocate a new defrag list header
 *
 * @bl_hw: Pointer to struct bl_hw 
 * @sn: skb sequence number
 *
 * This function is to allocate a new defrag list header according to skb sn
 *
 */
static struct rxdefrag_list *bl_rx_defrag_alloc(struct bl_hw *bl_hw, u16 sn)
{
    /* Get the first element of the list of used Reassembly structures */
    struct rxdefrag_list *p_defrag = NULL;

#ifdef DEFRAG_LIST
    if (!p_defrag)
    {
        /* Get the first element of the list of used Reassembly structures */
        p_defrag = (struct rxdefrag_list *)(&bl_hw->rxu_defrag_used);

    }
#else
	p_defrag = &rx_defrag_pool[sn%8];
#endif
    /* Return the allocated element */
    return (p_defrag);
}
/**
 * bl_rx_defrag_get - get a defrag list header
 *
 * @bl_hw: Pointer to struct bl_hw
 * @sta_idx: sta index 
 * @sn: skb sequence number
 * @tid: traffic index 
 *
 * This function is to get a defrag list header according to sta_idx&sn&tid
 *
 */
static struct rxdefrag_list *bl_rx_defrag_get(struct bl_hw *bl_hw, u8 sta_idx, u16 sn, u8 tid)
{
    /* Get the first element of the list of used Reassembly structures */
    struct rxdefrag_list *p_defrag = NULL;
	u8 cnt = 0;

#ifdef DEFRAG_LIST
    list_for_each_entry(p_defrag, &bl_hw->rxu_defrag_used, list) {

	    if (p_defrag)
	    {
	        /* Compare Station Id and TID */
	        if ((p_defrag->sta_idx == sta_idx)
	                && (p_defrag->tid == tid)
	                && (p_defrag->sn == sn))
	        {
	            /* We found a matching structure, escape from the loop */
	            BL_DBG("%s found match!\n", __func__);
	            break;
	        }

	        p_defrag = (struct rxdefrag_list *)p_defrag->list.next;
	    }

	}
#else
	while (cnt < BL_RX_DEFRAG_POOL_SIZE) {
			p_defrag = &rx_defrag_pool[cnt];
	        /* Compare Station Id and TID */	
	        if ((p_defrag->sta_idx == sta_idx)
	                && (p_defrag->tid == tid)
	                && (p_defrag->sn == sn))
	        {
	            /* We found a matching structure, escape from the loop */
	            BL_DBG("%s found match, stid %d, tid %d, sn %d!\n", __func__, sta_idx, tid, sn);
	            break;
	        }
			cnt++;
	}
	//the last item not match, return NULL
	if (cnt >= BL_RX_DEFRAG_POOL_SIZE) {
		p_defrag = NULL;
    }
#endif
    /* Return found element */    
    return (p_defrag);
}
/**
 * @brief Check if the received frame shall be reassembled.
 *
 * @bl_hw: Pointer to bl_hw
 * @skb: pointer to skb
 * @payload_offset: the offset of ethernet payload in the skb data buffer
 *
 * @return Whether the frame shall be defrag or not
 *
 */
struct sk_buff * bl_rx_defrag_check(struct bl_hw *bl_hw, struct sk_buff *skb, int payload_offset)
{
    struct hw_rxhdr *hw_rxhdr;
    struct bl_vif *bl_vif;
	struct sk_buff *skb_ret = skb;	
    struct rxdefrag_list *p_defrag;
    int sta_idx = 0, tid = 0;
    u16 sn = 0, mf = 0;
    u8 fn = 0;

	hw_rxhdr = (struct hw_rxhdr *)skb->data;
	bl_vif = bl_rx_get_vif(bl_hw, hw_rxhdr->flags_vif_idx);
	if (!bl_vif) {
		dev_err(bl_hw->dev, "Frame received but no active vif (%d)",
				hw_rxhdr->flags_vif_idx);
		dev_kfree_skb(skb);
		return NULL;
	}

	sn = hw_rxhdr->sn;
	mf = hw_rxhdr->flags_mf;
	fn = hw_rxhdr->fn;
	tid = hw_rxhdr->tid;
	sta_idx = hw_rxhdr->flags_sta_idx;

    do
    {

		if (!mf && !fn)
		{
			/* isn't a fragment frame directly return original skb */
			break;
		}

		/* Check if a reassembly procedure is in progress */
		p_defrag = bl_rx_defrag_get(bl_hw, sta_idx, sn, tid);

		BL_DBG("p_defrag=%p, stid=%d, tid=%d, sn=%d, fn=%d, pn=%lld\n",
			p_defrag, sta_idx, tid, sn, hw_rxhdr->fn, hw_rxhdr->pn);

		if (!p_defrag)
		{

			if (fn) {
				/* If not first fragment, we can reject the packet */				
				dev_kfree_skb(skb);
				BL_DBG("%s drop as fn=%d but p_defrag=NULL\n", __func__, fn);
				skb_ret = NULL;
				break;
			}
		
			/* Allocate a Reassembly structure */
			p_defrag = bl_rx_defrag_alloc(bl_hw, sn);
		
			/* Fullfil the Reassembly structure */
			p_defrag->bl_hw		= bl_hw;	
			p_defrag->sta_idx	  = sta_idx;
			p_defrag->tid		  = tid;
			p_defrag->sn		  = sn;
			p_defrag->next_fn	  = 1;

			/* Get Fragment Length */
			p_defrag->cpy_len	  = skb->len;
			/* Reset total received length */
			p_defrag->frame_len   = skb->len;
            /* Record the frame to defrag list */
            p_defrag->pkt[fn] = skb;
			
#ifdef DEFRAG_TO			
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
			timer_setup(&p_defrag->timer, bl_rx_defrag_timeout_cb, 0);
#else
			init_timer(&p_defrag->timer);
			p_defrag->timer.function = bl_rx_defrag_timeout_cb;
			p_defrag->timer.data = (void *)&p_defrag->timer;
#endif		
#endif

#ifdef DEFRAG_LIST
			/* Push the reassembly structure at the end of the list of used structures */
			list_add_tail(&bl_hw->rxu_defrag_used, &p_defrag->list);
#endif
			/* Indicate to jump dispatch and will be freed in defrag processing */
			skb_ret = NULL;
		}
		else
		{
		
			/* Check the fragment is the one we are waiting for */			
			if (p_defrag->next_fn != fn) {
				/* Packet has already been received */
				BL_DBG("%s drop as nexfn:%d != fn:%d\n",__func__, p_defrag->next_fn, fn);
				dev_kfree_skb(skb);
				skb_ret = NULL;
				break;
			}
			/* Get payload length of fragment */
			if (fn == 0) {
				/* Fullfil the Reassembly structure */
				p_defrag->sta_idx	  = sta_idx;
				p_defrag->tid		  = tid;
				p_defrag->sn		  = sn;
				p_defrag->cpy_len	  = skb->len;

#ifdef DEFRAG_TO			
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
			timer_setup(&p_defrag->timer, bl_rx_defrag_timeout_cb, 0);
#else
			init_timer(&p_defrag->timer);
			p_defrag->timer.function = bl_rx_defrag_timeout_cb;
			p_defrag->timer.data = (void *)&p_defrag->timer;
#endif		
#endif
			}
			else {
				p_defrag->cpy_len = (skb->len - payload_offset- sizeof(struct ethhdr));
			}
			p_defrag->frame_len += p_defrag->cpy_len;
		
			/* Update number of received fragment */
			p_defrag->next_fn++;
			p_defrag->pkt[fn] = skb;

			/* Indicate to jump dispatch and will be freed in defrag processing */
			skb_ret = NULL;
			
			if (!mf)
			{/* the last fragment */

				/* Indicate that the packet can now be defraged */
				bl_rx_defrag_pkt(bl_hw, p_defrag, payload_offset);
				
				skb_ret = (struct sk_buff *)p_defrag->pkt[0];
#ifdef DEFRAG_TO		
				/* Clear the reassembly timer */
				if(in_irq() || in_atomic() || irqs_disabled())
					del_timer(&p_defrag->timer);
				else	
					del_timer_sync(&p_defrag->timer);
#endif				
#ifdef DEFRAG_LIST		
				/* Free the Reassembly structure */
				list_add_tail(&bl_hw->rxu_defrag_free, &p_defrag->list);
#else
				memset(p_defrag, 0, sizeof(struct rxdefrag_list));
#endif
				/* Forward the total de-fraged frame to upper layer */

			}
		
		}

#ifdef DEFRAG_TO		
		/* Set defrag timer */
		if(mf && !p_defrag->is_timer_set){
			bl_rx_defrag_timer_set(p_defrag);
		}
#endif
	} while (0);
	
	return skb_ret;
}
#endif

#ifdef  BL_RX_REORDER
/**
 * bl_rx_reorder_pkt - reorder the rx packet
 *
 * @bl_hw: Pointer to bl_hw
 * @reorder_list: Pointer to the struct rxreorder_list
 * @sn: packet sequence
 * @is_bar: is BAR packet or not
 * @skb: pointer to skb
 *
 * This function is called for rx packet reorder
 *
 */
 void bl_rx_reorder_pkt(struct bl_hw *bl_hw, struct rxreorder_list *reorder_list, u16 sn, u8 is_bar,struct sk_buff *skb)
{
    u8 offset, index, drop = 0;
	int prev_start_win, start_win, end_win, win_size;

    if(is_bar)
		BL_DBG("%s BAR \n", __func__);

    if(reorder_list->check_start_win){
        if(sn == reorder_list->start_win) {
		    reorder_list->check_start_win = false;
			BL_DBG("end start win checking sn = %d \n", sn);
		}
		else{
            reorder_list->cnt ++;
            if(reorder_list->cnt < RX_WIN_SIZE/2){
				BL_DBG("checking sn %d \n", sn);
			    reorder_list->last_seq = sn;
			    if(!is_bar)
				    bl_rx_packet_dispatch(bl_hw,skb);
			    goto end;
			}
            reorder_list->check_start_win = false;
            if (sn != reorder_list->start_win) {
                end_win = (reorder_list->start_win + RX_WIN_SIZE - 1) & (MAX_SEQ_VALUE -1);
                if (((end_win > reorder_list->start_win)  &&
					     (reorder_list->last_seq >= reorder_list->start_win) &&
					     (reorder_list->last_seq < end_win)) ||
					    ((end_win < reorder_list->start_win) &&
					     ((reorder_list->last_seq >= reorder_list->start_win) ||
					      (reorder_list->last_seq < end_win)))) {
						BL_DBG("%s Update start_win: last_seq=%d, start_win=%d seq_num=%d\n", __func__,
						       reorder_list->last_seq, reorder_list->start_win,sn);
						reorder_list->start_win = reorder_list->last_seq + 1;
					} else if ((sn < reorder_list->start_win) && (sn > reorder_list->last_seq)) {
						BL_DBG("%s Update start_win: last_seq=%d, start_win=%d seq_num=%d\n", __func__,
						       reorder_list->last_seq, reorder_list->start_win,sn);

						reorder_list->start_win = reorder_list->last_seq + 1;
					}
				}			
		}			
	}

   prev_start_win = start_win = reorder_list->start_win;
   win_size = RX_WIN_SIZE;
   end_win = ((start_win + win_size) - 1) & (MAX_SEQ_VALUE- 1);

   //1. sn  less than start_win, drop it
	//BL_DBG("%s :#1 start win %d end win %d win size %d sn %d \n",__func__,start_win, end_win, win_size, sn);
	if ((start_win + TWOPOW11) > (MAX_SEQ_VALUE - 1)) {
		if (sn >= ((start_win + (TWOPOW11)) &
				(MAX_SEQ_VALUE - 1)) &&
			(sn < start_win)) {
           drop = 1;
		   goto drop;
		}
	} else if ((sn < start_win) ||
		   (sn >= (start_win + (TWOPOW11)))) {
        drop = 1;
		goto drop;
	}

	
	/* BAR: update win start = sn in case #2*/
	if (is_bar)
		sn = ((sn + win_size) - 1) & (MAX_SEQ_VALUE - 1);

    // 2. sn larger than end win, out of buffering, free previous rx packet, and move the start win
    if (((end_win < start_win) && (sn < start_win) && (sn > end_win)) ||
		    ((end_win > start_win) && ((sn > end_win) || (sn < start_win)))) {
		BL_DBG("%s :#2(out of win) start win %d end win %d win size %d sn %d \n",__func__,start_win, end_win, win_size, sn);  
        end_win = sn;
        if (((sn  - win_size) + 1) >= 0)
            start_win = (end_win - win_size) + 1;
        else
            start_win =	(MAX_SEQ_VALUE - (win_size -sn)) + 1;
        //flush util new start win
        bl_rx_reorder_dispatch_until_start_win(bl_hw, reorder_list, start_win);
    }

	//3. now sn in the buffer window
	if(!is_bar) {
	    BL_DBG("%s :#3 start win %d end win %d win size %d sn %d \n",__func__,start_win, end_win, win_size, sn);	
        if (sn >= reorder_list->start_win)
            offset = sn - reorder_list->start_win;
        else // wrap case
            offset = (sn + MAX_SEQ_VALUE - reorder_list->start_win)%MAX_SEQ_VALUE;

        index = (reorder_list->start_win_index + offset) % RX_WIN_SIZE;
       if (reorder_list->reorder_pkt[index]) {
            BL_DBG("Drop Duplicate Pkt sn %d\n", sn);
            drop = 1;
            goto drop;
        }
        reorder_list->last_seq = sn;	
        reorder_list->reorder_pkt[index] = skb;
	} else{
		// free BAR
	    dev_kfree_skb(skb);
	}

    // dispatch packet from start win util one hole is hit ,and update start win
    bl_rx_reorder_scan_and_dispatch(bl_hw, reorder_list);
    if(prev_start_win != reorder_list->start_win || !reorder_list->is_timer_set){
           bl_rx_reorder_timer_set(reorder_list);
	}	
    return;
drop:
    if(drop){		
		BL_DBG("%s :#1(drop) start win %d end win %d win size %d sn %d \n",__func__,start_win, end_win, win_size, sn);
        dev_kfree_skb(skb);
    }

end:
   return;
}
#endif

/**
 * bl_rxdataind - Process rx buffer
 *
 * @pthis: Pointer to the object attached to the IPC structure
 *         (points to struct bl_hw is this case)
 * @hostid: Address of the RX descriptor
 *
 * This function is called for each buffer received by the fw
 *
 */
u8 bl_rxdataind(void *pthis, void *hostid)
{
    u32 status = 0;
    int sta_idx = 0;
    int tid = 0;
    u16 sn = 0;
    u16 mf = 0;
    u8 fn = 0;

    struct bl_hw *bl_hw = (struct bl_hw *)pthis;
    struct hw_rxhdr *hw_rxhdr;
    struct bl_vif *bl_vif;
    struct sk_buff *skb = (struct sk_buff *)hostid;
#ifndef  BL_RX_REORDER
    bool found = false;
    struct bl_agg_reord_pkt *reord_pkt = NULL;
    struct bl_agg_reord_pkt *reord_pkt_inst = NULL;
    struct ieee80211_hdr *hdr;
#else
    struct rxreorder_list *reorder_list;
    const struct ethhdr *eth;
#endif
    int msdu_offset = sizeof(struct hw_rxhdr);

    if(!skb)
        return -1;
	/*status occupy 4 bytes*/
    memcpy(&status, skb->data, 4);
    skb_pull(skb, 4);

    /* check that frame is completely uploaded */
    if (!status) {
	printk("receive status error, status=0x%x\n", status);
        return -1;
    }

	BL_DBG("status---->:0x%x\n", status);

	if(status & RX_STAT_DELETE) {
		BL_DBG("staus->delete, just free skb\n");
		skb_push(skb, sizeof(struct sdio_hdr) + 4);
		dev_kfree_skb(skb);
		goto end;
	}
	
	hw_rxhdr = (struct hw_rxhdr *)skb->data;
	sta_idx = hw_rxhdr->flags_sta_idx;
	sn = hw_rxhdr->sn;
	tid = hw_rxhdr->tid;
	bl_vif = bl_rx_get_vif(bl_hw, hw_rxhdr->flags_vif_idx);
	if (!bl_vif) {
		dev_err(bl_hw->dev, "Frame received but no active vif (%d) sn %d",
				hw_rxhdr->flags_vif_idx, sn);
		dev_kfree_skb(skb);
		goto end;
	}
	
	if((hw_rxhdr->flags_vif_idx != 0xFF && hw_rxhdr->flags_vif_idx > NX_VIRT_DEV_MAX) 
		|| (hw_rxhdr->flags_sta_idx != 0xFF && hw_rxhdr->flags_sta_idx > (NX_REMOTE_STA_MAX + NX_VIRT_DEV_MAX))) {
		printk("free broken data, vif_idx=%d, sta_idx=%d sn %d\n", hw_rxhdr->flags_vif_idx,  hw_rxhdr->flags_sta_idx, sn);
		dev_kfree_skb(skb);
		goto end;
	}
		
	/*Now, skb->data pointed to the real payload*/			
	skb_pull(skb, msdu_offset);

	if (hw_rxhdr->flags_is_80211_mpdu) {	
		BL_DBG("receive mgmt packet: type=0x%x, subtype=0x%x\n", 
			BL_FC_GET_TYPE(((struct ieee80211_mgmt *)skb->data)->frame_control), BL_FC_GET_STYPE(((struct ieee80211_mgmt *)skb->data)->frame_control));

		bl_rx_mgmt(bl_hw, bl_vif, skb, hw_rxhdr);
		goto end;
	} 

#ifdef BL_RX_DEFRAG
	mf = hw_rxhdr->flags_mf;
	fn = hw_rxhdr->fn;

	if (mf || fn) {
		/* defrag needs hw_rxhdr info */
		skb_push(skb, msdu_offset);

		skb = bl_rx_defrag_check(bl_hw, skb, msdu_offset);

		if (NULL == skb)
			goto end;

		bl_rx_packet_dispatch(bl_hw, skb);
		goto end;
	}
#endif

#ifdef  BL_RX_REORDER
	if((tid < 8) && (sta_idx < (NX_REMOTE_STA_MAX + NX_VIRT_DEV_MAX)))
	    reorder_list = &bl_hw->rx_reorder[sta_idx][tid];

	
	eth = (struct ethhdr *)(skb->data);

	skb_push(skb, msdu_offset);

    if(reorder_list && reorder_list->flag && ((!is_multicast_ether_addr(eth->h_dest))||(hw_rxhdr->flags_is_bar))){
	    bl_rx_reorder_pkt(bl_hw, reorder_list, sn, hw_rxhdr->flags_is_bar, skb);
        goto end;
	}
	bl_rx_packet_dispatch(bl_hw, skb);
#else

    /* Check if we need to forward the buffer */
    if (status & RX_STAT_FORWARD) {
		BL_DBG("RX_STATE_FORWARD: sn=%u, tid=%d\n", sn, tid);

        	BL_DBG("receive data packet: type=0x%x, subtype=0x%x\n", 
					BL_FC_GET_TYPE(*((u8 *)(skb->data))), BL_FC_GET_STYPE(*((u8 *)(skb->data))));
			
            if ((hw_rxhdr->flags_sta_idx != 0xff) && (hw_rxhdr->flags_sta_idx < (NX_REMOTE_STA_MAX + NX_VIRT_DEV_MAX))) {
                struct bl_sta *sta= &bl_hw->sta_table[hw_rxhdr->flags_sta_idx];

                bl_rx_statistic(bl_hw, hw_rxhdr, sta);

				//printk("sta->vlan_idx=%d, bl_vif->vif_index=%d\n", sta->vlan_idx, bl_vif->vif_index);
                if (sta->vlan_idx != bl_vif->vif_index) {
                    bl_vif = bl_hw->vif_table[sta->vlan_idx];
                    if (!bl_vif) {
						printk("cannot find the vif\n");
                        dev_kfree_skb(skb);
                        goto end;
                    }
                }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 3, 0)
                if (hw_rxhdr->flags_is_4addr && !bl_vif->use_4addr) {
                    cfg80211_rx_unexpected_4addr_frame(bl_vif->ndev,
                                                       sta->mac_addr, GFP_ATOMIC);
                }
#endif
            }

			BL_DBG("original skb->priority=%d\n", skb->priority);
            skb->priority = 256 + hw_rxhdr->flags_user_prio;
			BL_DBG("hw_rxhdr->flags_user_prio=%d, skb->priority=%d, ndev_idx=%d\n", hw_rxhdr->flags_user_prio, skb->priority, skb_get_queue_mapping(skb));
            if (!bl_rx_data_skb(bl_hw, bl_vif, skb, hw_rxhdr)) {
                dev_kfree_skb(skb);
			}        
    }
	if (status & RX_STAT_ALLOC) {
		/*put skb into right list according sta_idx & tid*/
		//hw_rxhdr = (struct hw_rxhdr *)skb->data;

		skb_push(skb, msdu_offset);

		BL_DBG("RX_STATE_ALLOC: sta_idx=%d, sn=%u, tid=%d\n", sta_idx, sn, tid);
		/*check sta id */
		if(sta_idx >= (NX_REMOTE_STA_MAX + NX_VIRT_DEV_MAX) || tid > NX_NB_TID_PER_STA){
		   printk("%s  tid%d sta_idx %d larger than max support %d \n",__func__, tid, sta_idx, (NX_REMOTE_STA_MAX + NX_VIRT_DEV_MAX));
		   dev_kfree_skb(skb);
		   return -1;
		}
		//restore the skb and add into the tail of reorder list
		skb_push(skb, 4);

		/*construct the reorder pkt*/
		reord_pkt = kmem_cache_alloc(bl_hw->agg_reodr_pkt_cache, GFP_ATOMIC);
		if(reord_pkt == NULL) {
			printk("KMEM CACHE ALLOC failed\n");
			return -1;
		}

		reord_pkt->skb = skb;
		reord_pkt->sn = sn;

		if (!list_empty(&bl_hw->reorder_list[sta_idx*NX_NB_TID_PER_STA + tid])) {
				list_for_each_entry(reord_pkt_inst, &bl_hw->reorder_list[sta_idx*NX_NB_TID_PER_STA + tid], list) {
					BL_DBG("reord_pkt_inst->sn=%u, sn=%u\n", reord_pkt_inst->sn, sn);
					if(reord_pkt_inst->sn > sn) {
						BL_DBG("Add sn %u before sn %u\n", sn, reord_pkt_inst->sn);
						found = true;
						list_add(&(reord_pkt->list), reord_pkt_inst->list.prev);
						break;
					}
				}

				if(!found) {
					BL_DBG("not found, add sn %u to tail\n", sn);
					list_add_tail(&(reord_pkt->list), &bl_hw->reorder_list[sta_idx*NX_NB_TID_PER_STA+tid]);		
				}
		} else {
			BL_DBG("list is empty, add sn %u in tail\n", sn);
			list_add_tail(&(reord_pkt->list), &bl_hw->reorder_list[sta_idx*NX_NB_TID_PER_STA+tid]);		
		}
	}
#endif
    end:

    return 0;
}

