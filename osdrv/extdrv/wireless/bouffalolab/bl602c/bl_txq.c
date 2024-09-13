/**
 ******************************************************************************
 *
 *  @file bl_txq.c
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

#include <linux/poison.h>

#include "bl_defs.h"
#include "bl_ftrace.h"
#include "bl_sdio.h"
#include "bl_v7.h"


/******************************************************************************
 * Utils inline functions
 *****************************************************************************/
struct bl_txq *bl_txq_sta_get(struct bl_sta *sta, u8 tid, int *idx,
                                  struct bl_hw * bl_hw)
{
    int id;

    if (tid >= NX_NB_TXQ_PER_STA)
        tid = 0;

    if (is_multicast_sta(sta->sta_idx))
        id = (NX_REMOTE_STA_MAX * NX_NB_TXQ_PER_STA) + sta->vif_idx;
    else
        id = (sta->sta_idx * NX_NB_TXQ_PER_STA) + tid;

    if (idx)
        *idx = id;

    return &bl_hw->txq[id];
}

struct bl_txq *bl_txq_vif_get(struct bl_vif *vif, u8 type, int *idx)
{
    int id;

    id = NX_FIRST_VIF_TXQ_IDX + master_vif_idx(vif) +
        (type * NX_VIRT_DEV_MAX);

    if (idx)
        *idx = id;

    return &vif->bl_hw->txq[id];
}

static inline
struct bl_sta *bl_txq_2_sta(struct bl_txq *txq)
{
    return txq->sta;
}

static const u16 bl_1d_to_wmm_queue[8] = { 1, 0, 0, 1, 2, 2, 3, 3 };

/******************************************************************************
 * Init/Deinit functions
 *****************************************************************************/
/**
 * bl_txq_init - Initialize a TX queue
 *
 * @txq: TX queue to be initialized
 * @idx: TX queue index
 * @status: TX queue initial status
 * @hwq: Associated HW queue
 * @ndev: Net device this queue belongs to
 *        (may be null for non netdev txq)
 *
 * Each queue is initialized with the credit of @NX_TXQ_INITIAL_CREDITS.
 */
static void bl_txq_init(struct bl_txq *txq, int idx, u8 status,
                          struct bl_hwq *hwq,
                          struct bl_sta *sta, struct net_device *ndev
                          )
{
    int i;
	BL_DBG(BL_FN_ENTRY_STR);

    txq->idx = idx;
    txq->status = status;
    txq->credits = NX_TXQ_INITIAL_CREDITS;
    txq->pkt_sent = 0;
    skb_queue_head_init(&txq->sk_list);
    txq->last_retry_skb = NULL;
    txq->nb_retry = 0;
    txq->hwq = hwq;
    txq->sta = sta;
    for (i = 0; i < CONFIG_USER_MAX ; i++)
        txq->pkt_pushed[i] = 0;

    txq->ps_id = LEGACY_PS_ID;
    txq->push_limit = 0;
    if (idx < NX_FIRST_VIF_TXQ_IDX) {
        int sta_idx = sta->sta_idx;
        int tid = idx - (sta_idx * NX_NB_TXQ_PER_STA);
        if (tid < NX_NB_TID_PER_STA)
            //txq->ndev_idx = NX_STA_NDEV_IDX(tid, sta_idx);
			txq->ndev_idx = bl_1d_to_wmm_queue[tid];
			//txq->ndev_idx = tid;
        else
            txq->ndev_idx = NDEV_NO_TXQ;
    } else if (idx < NX_FIRST_UNK_TXQ_IDX) {
        txq->ndev_idx = NX_BCMC_TXQ_NDEV_IDX;
    } else {
        txq->ndev_idx = NDEV_NO_TXQ;
    }
    txq->ndev = ndev;
#ifdef CONFIG_BL_AMSDUS_TX
    txq->amsdu = NULL;
    txq->amsdu_len = 0;
#endif /* CONFIG_BL_AMSDUS_TX */
}

/**
 * bl_txq_flush - Flush all buffers queued for a TXQ
 *
 * @bl_hw: main driver data
 * @txq: txq to flush
 */
void bl_txq_flush(struct bl_hw *bl_hw, struct bl_txq *txq)
{
    struct sk_buff *skb;

    while((skb = skb_dequeue(&txq->sk_list)) != NULL) {
        struct bl_sw_txhdr *hdr = ((struct bl_txhdr *)skb->data)->sw_hdr;
#ifdef CONFIG_BL_AMSDUS_TX
        if (hdr->desc.host.packet_cnt > 1) {
            struct bl_amsdu_txhdr *amsdu_txhdr;
            list_for_each_entry(amsdu_txhdr, &hdr->amsdu.hdrs, list) {
                dma_unmap_single(bl_hw->dev, amsdu_txhdr->dma_addr,
                                 amsdu_txhdr->map_len, DMA_TO_DEVICE);
                dev_kfree_skb_any(amsdu_txhdr->skb);
            }
        }
#endif
        kmem_cache_free(bl_hw->sw_txhdr_cache, hdr);
        dev_kfree_skb_any(skb);
    }
}

/**
 * bl_txq_deinit - De-initialize a TX queue
 *
 * @bl_hw: Driver main data
 * @txq: TX queue to be de-initialized
 * Any buffer stuck in a queue will be freed.
 */
static void bl_txq_deinit(struct bl_hw *bl_hw, struct bl_txq *txq)
{
    spin_lock_bh(&bl_hw->txq_lock);
    bl_txq_del_from_hw_list(txq);
    txq->idx = TXQ_INACTIVE;
    spin_unlock_bh(&bl_hw->txq_lock);

    bl_txq_flush(bl_hw, txq);
}

/**
 * bl_txq_vif_init - Initialize all TXQ linked to a vif
 *
 * @bl_hw: main driver data
 * @bl_vif: Pointer on VIF
 * @status: Intial txq status
 *
 * Softmac : 1 VIF TXQ per HWQ
 *
 * Fullmac : 1 VIF TXQ for BC/MC
 *           1 VIF TXQ for MGMT to unknown STA
 */
void bl_txq_vif_init(struct bl_hw *bl_hw, struct bl_vif *bl_vif,
                       u8 status)
{
    struct bl_txq *txq;
    int idx;

	BL_DBG(BL_FN_ENTRY_STR);

    txq = bl_txq_vif_get(bl_vif, NX_BCMC_TXQ_TYPE, &idx);
    bl_txq_init(txq, idx, status, &bl_hw->hwq[BL_HWQ_BE],
                  &bl_hw->sta_table[bl_vif->ap.bcmc_index], bl_vif->ndev);

    txq = bl_txq_vif_get(bl_vif, NX_UNK_TXQ_TYPE, &idx);
    bl_txq_init(txq, idx, status, &bl_hw->hwq[BL_HWQ_VO],
                  NULL, bl_vif->ndev);
}

/**
 * bl_txq_vif_deinit - Deinitialize all TXQ linked to a vif
 *
 * @bl_hw: main driver data
 * @bl_vif: Pointer on VIF
 */
void bl_txq_vif_deinit(struct bl_hw * bl_hw, struct bl_vif *bl_vif)
{
    struct bl_txq *txq;

    txq = bl_txq_vif_get(bl_vif, NX_BCMC_TXQ_TYPE, NULL);
    bl_txq_deinit(bl_hw, txq);

    txq = bl_txq_vif_get(bl_vif, NX_UNK_TXQ_TYPE, NULL);
    bl_txq_deinit(bl_hw, txq);
}


/**
 * bl_txq_sta_init - Initialize TX queues for a STA
 *
 * @bl_hw: Main driver data
 * @bl_sta: STA for which tx queues need to be initialized
 * @status: Intial txq status
 *
 * This function initialize all the TXQ associated to a STA.
 * Softmac : 1 TXQ per TID
 *
 * Fullmac : 1 TXQ per TID (limited to 8)
 *           1 TXQ for MGMT
 */
void bl_txq_sta_init(struct bl_hw * bl_hw, struct bl_sta *bl_sta,
                       u8 status)
{
    struct bl_txq *txq;
    int tid, idx;
	struct bl_vif *bl_vif = NULL;
	BL_DBG(BL_FN_ENTRY_STR);

    bl_vif = bl_hw->vif_table[bl_sta->vif_idx];

    txq = bl_txq_sta_get(bl_sta, 0, &idx, bl_hw);
    for (tid = 0; tid < NX_NB_TXQ_PER_STA; tid++, txq++, idx++) {
        bl_txq_init(txq, idx, status, &bl_hw->hwq[bl_tid2hwq[tid]],
                      bl_sta, bl_vif->ndev);
        txq->ps_id = bl_sta->uapsd_tids & (1 << tid) ? UAPSD_ID : LEGACY_PS_ID;
    }
}

/**
 * bl_txq_sta_deinit - Deinitialize TX queues for a STA
 *
 * @bl_hw: Main driver data
 * @bl_sta: STA for which tx queues need to be deinitialized
 */
void bl_txq_sta_deinit(struct bl_hw *bl_hw, struct bl_sta *bl_sta)
{
    struct bl_txq *txq;
    int i;

    txq = bl_txq_sta_get(bl_sta, 0, NULL, bl_hw);

    for (i = 0; i < NX_NB_TXQ_PER_STA; i++, txq++) {
        bl_txq_deinit(bl_hw, txq);
    }
}

/**
 * bl_txq_flush_all - flush all txq
 *
 * @bl_hw: Main driver data
 *
 */
 void bl_txq_flush_all(struct bl_hw *bl_hw)
{
    struct bl_txq *txq;
    int i;

    for(i = 0; i < NX_NB_TXQ; i++){
        txq = &bl_hw->txq[i];
        bl_txq_deinit(bl_hw, txq);
    }

}
#ifdef CONFIG_BL_FULLMAC
/**
 * bl_txq_unk_vif_init - Initialize TXQ for unknown STA linked to a vif
 *
 * @bl_vif: Pointer on VIF
 */
void bl_txq_unk_vif_init(struct bl_vif *bl_vif)
{
    struct bl_hw *bl_hw = bl_vif->bl_hw;
    struct bl_txq *txq;
    int idx;

    txq = bl_txq_vif_get(bl_vif, NX_UNK_TXQ_TYPE, &idx);
//    idx = bl_txq_vif_idx(bl_vif, NX_UNK_TXQ_TYPE);
    bl_txq_init(txq, idx, 0, &bl_hw->hwq[BL_HWQ_VO], NULL, bl_vif->ndev);
}

/**
 * bl_txq_unk_vif_deinit - Deinitialize TXQ for unknown STA linked to a vif
 *
 * @bl_vif: Pointer on VIF
 */
void bl_txq_unk_vif_deinit(struct bl_vif *bl_vif)
{
    struct bl_txq *txq;

    txq = bl_txq_vif_get(bl_vif, NX_UNK_TXQ_TYPE, NULL);
    bl_txq_deinit(bl_vif->bl_hw, txq);
}

/**
 * bl_init_unk_txq - Initialize TX queue for the transmission on a offchannel
 *
 * @vif: Interface for which the queue has to be initialized
 *
 * NOTE: Offchannel txq is only active for the duration of the ROC
 */
void bl_txq_offchan_init(struct bl_vif *bl_vif)
{
    struct bl_hw *bl_hw = bl_vif->bl_hw;
    struct bl_txq *txq;
	BL_DBG(BL_FN_ENTRY_STR);

    txq = &bl_hw->txq[NX_OFF_CHAN_TXQ_IDX];
    bl_txq_init(txq, NX_OFF_CHAN_TXQ_IDX, BL_TXQ_STOP_CHAN,
                  &bl_hw->hwq[BL_HWQ_VO], NULL, bl_vif->ndev);
}

/**
 * bl_deinit_offchan_txq - Deinitialize TX queue for offchannel
 *
 * @vif: Interface that manages the STA
 *
 * This function deintialize txq for one STA.
 * Any buffer stuck in a queue will be freed.
 */
void bl_txq_offchan_deinit(struct bl_vif *bl_vif)
{
    struct bl_txq *txq;

    txq = &bl_vif->bl_hw->txq[NX_OFF_CHAN_TXQ_IDX];
    bl_txq_deinit(bl_vif->bl_hw, txq);
}

#endif

/******************************************************************************
 * Start/Stop functions
 *****************************************************************************/
/**
 * bl_txq_add_to_hw_list - Add TX queue to a HW queue schedule list.
 *
 * @txq: TX queue to add
 *
 * Add the TX queue if not already present in the HW queue list.
 * To be called with tx_lock hold
 */
void bl_txq_add_to_hw_list(struct bl_txq *txq)
{
    if (!(txq->status & BL_TXQ_IN_HWQ_LIST)) {
        trace_txq_add_to_hw(txq);
        txq->status |= BL_TXQ_IN_HWQ_LIST;
        list_add_tail(&txq->sched_list, &txq->hwq->list);
    } else {
		BL_DBG("txq is not scheduled for transmission, txq->status=0x%x\n", txq->status);
    }
}

/**
 * bl_txq_del_from_hw_list - Delete TX queue from a HW queue schedule list.
 *
 * @txq: TX queue to delete
 *
 * Remove the TX queue from the HW queue list if present.
 * To be called with tx_lock hold
 */
void bl_txq_del_from_hw_list(struct bl_txq *txq)
{
    if (txq->status & BL_TXQ_IN_HWQ_LIST) {
        trace_txq_del_from_hw(txq);
        txq->status &= ~BL_TXQ_IN_HWQ_LIST;
        list_del(&txq->sched_list);
	if(txq->hwq->curr_list == &txq->sched_list)
		txq->hwq->curr_list = &txq->hwq->list;
    }
}

/**
 * bl_txq_start - Try to Start one TX queue
 *
 * @txq: TX queue to start
 * @reason: reason why the TX queue is started (among BL_TXQ_STOP_xxx)
 *
 * Re-start the TX queue for one reason.
 * If after this the txq is no longer stopped and some buffers are ready,
 * the TX queue is also added to HW queue list.
 * To be called with tx_lock hold
 */
void bl_txq_start(struct bl_txq *txq, u16 reason)
{
    BUG_ON(txq==NULL);
    if (txq->idx != TXQ_INACTIVE && (txq->status & reason))
    {
        trace_txq_start(txq, reason);
        txq->status &= ~reason;
        if (!bl_txq_is_stopped(txq) &&
            !skb_queue_empty(&txq->sk_list)) {
            bl_txq_add_to_hw_list(txq);
        }
    }
}

/**
 * bl_txq_stop - Stop one TX queue
 *
 * @txq: TX queue to stop
 * @reason: reason why the TX queue is stopped (among BL_TXQ_STOP_xxx)
 *
 * Stop the TX queue. It will remove the TX queue from HW queue list
 * To be called with tx_lock hold
 */
void bl_txq_stop(struct bl_txq *txq, u16 reason)
{
    BUG_ON(txq==NULL);
    if (txq->idx != TXQ_INACTIVE)
    {
        trace_txq_stop(txq, reason);
        txq->status |= reason;
        bl_txq_del_from_hw_list(txq);
    }
}


/**
 * bl_txq_sta_start - Start all the TX queue linked to a STA
 *
 * @sta: STA whose TX queues must be re-started
 * @reason: Reason why the TX queue are restarted (among BL_TXQ_STOP_xxx)
 * @bl_hw: Driver main data
 *
 * This function will re-start all the TX queues of the STA for the reason
 * specified. It can be :
 * - BL_TXQ_STOP_STA_PS: the STA is no longer in power save mode
 * - BL_TXQ_STOP_VIF_PS: the VIF is in power save mode (p2p absence)
 * - BL_TXQ_STOP_CHAN: the STA's VIF is now on the current active channel
 *
 * Any TX queue with buffer ready and not Stopped for other reasons, will be
 * added to the HW queue list
 * To be called with tx_lock hold
 */
void bl_txq_sta_start(struct bl_sta *bl_sta, u16 reason
#ifdef CONFIG_BL_FULLMAC
                        , struct bl_hw *bl_hw
#endif
                        )
{
    struct bl_txq *txq;
    int i;
    int nb_txq;

    if (!bl_sta)
        return;

    trace_txq_sta_start(bl_sta->sta_idx);

    txq = bl_txq_sta_get(bl_sta, 0, NULL, bl_hw);

    if (is_multicast_sta(bl_sta->sta_idx))
        nb_txq = 1;
    else
        nb_txq = NX_NB_TXQ_PER_STA;

    for (i = 0; i < nb_txq; i++, txq++)
        bl_txq_start(txq, reason);
}


/**
 * bl_stop_sta_txq - Stop all the TX queue linked to a STA
 *
 * @sta: STA whose TX queues must be stopped
 * @reason: Reason why the TX queue are stopped (among BL_TX_STOP_xxx)
 * @bl_hw: Driver main data
 *
 * This function will stop all the TX queues of the STA for the reason
 * specified. It can be :
 * - BL_TXQ_STOP_STA_PS: the STA is in power save mode
 * - BL_TXQ_STOP_VIF_PS: the VIF is in power save mode (p2p absence)
 * - BL_TXQ_STOP_CHAN: the STA's VIF is not on the current active channel
 *
 * Any TX queue present in a HW queue list will be removed from this list.
 * To be called with tx_lock hold
 */
void bl_txq_sta_stop(struct bl_sta *bl_sta, u16 reason
#ifdef CONFIG_BL_FULLMAC
                       , struct bl_hw *bl_hw
#endif
                       )
{
    struct bl_txq *txq;
    int i;
#ifdef CONFIG_BL_FULLMAC
    int nb_txq;
#endif

    if (!bl_sta)
        return;

    trace_txq_sta_stop(bl_sta->sta_idx);
    txq = bl_txq_sta_get(bl_sta, 0, NULL, bl_hw);

    if (is_multicast_sta(bl_sta->sta_idx))
        nb_txq = 1;
    else
        nb_txq = NX_NB_TXQ_PER_STA;

    for (i = 0; i < nb_txq; i++, txq++)
        bl_txq_stop(txq, reason);

}

#ifdef CONFIG_BL_FULLMAC
static inline
void bl_txq_vif_for_each_sta(struct bl_hw *bl_hw, struct bl_vif *bl_vif,
                               void (*f)(struct bl_sta *, u16, struct bl_hw *),
                               u16 reason)
{

    switch (BL_VIF_TYPE(bl_vif)) {
    case NL80211_IFTYPE_STATION:
    case NL80211_IFTYPE_P2P_CLIENT:
    {
        if(!WARN_ON(bl_vif->sta.ap == NULL))
            f(bl_vif->sta.ap, reason, bl_hw);
        break;
    }
    case NL80211_IFTYPE_AP_VLAN:
        bl_vif = bl_vif->ap_vlan.master;
    case NL80211_IFTYPE_AP:
    case NL80211_IFTYPE_P2P_GO:
    {
        struct bl_sta *sta;
        list_for_each_entry(sta, &bl_vif->ap.sta_list, list) {
            f(sta, reason, bl_hw);
        }
        break;
    }
    default:
        BUG();
        break;
    }
}

#endif

/**
 * bl_txq_vif_start - START TX queues of all STA associated to the vif
 *                      and vif's TXQ
 *
 * @vif: Interface to start
 * @reason: Start reason (BL_TXQ_STOP_CHAN or BL_TXQ_STOP_VIF_PS)
 * @bl_hw: Driver main data
 *
 * Iterate over all the STA associated to the vif and re-start them for the
 * reason @reason
 * Take tx_lock
 */
void bl_txq_vif_start(struct bl_vif *bl_vif, u16 reason,
                        struct bl_hw *bl_hw)
{
    struct bl_txq *txq;

    trace_txq_vif_start(bl_vif->vif_index);

    spin_lock_bh(&bl_hw->txq_lock);
    bl_txq_vif_for_each_sta(bl_hw, bl_vif, bl_txq_sta_start, reason);

    txq = bl_txq_vif_get(bl_vif, NX_BCMC_TXQ_TYPE, NULL);
    bl_txq_start(txq, reason);
    txq = bl_txq_vif_get(bl_vif, NX_UNK_TXQ_TYPE, NULL);
    bl_txq_start(txq, reason);

    spin_unlock_bh(&bl_hw->txq_lock);
}



/**
 * bl_txq_vif_stop - STOP TX queues of all STA associated to the vif
 *
 * @vif: Interface to stop
 * @arg: Stop reason (BL_TXQ_STOP_CHAN or BL_TXQ_STOP_VIF_PS)
 * @bl_hw: Driver main data
 *
 * Iterate over all the STA associated to the vif and stop them for the
 * reason BL_TXQ_STOP_CHAN or BL_TXQ_STOP_VIF_PS
 * Take tx_lock
 */
void bl_txq_vif_stop(struct bl_vif *bl_vif, u16 reason,
                       struct bl_hw *bl_hw)
{
    struct bl_txq *txq;

    trace_txq_vif_stop(bl_vif->vif_index);

    spin_lock_bh(&bl_hw->txq_lock);

    bl_txq_vif_for_each_sta(bl_hw, bl_vif, bl_txq_sta_stop, reason);

    txq = bl_txq_vif_get(bl_vif, NX_BCMC_TXQ_TYPE, NULL);
    bl_txq_stop(txq, reason);
    txq = bl_txq_vif_get(bl_vif, NX_UNK_TXQ_TYPE, NULL);
    bl_txq_stop(txq, reason);

    spin_unlock_bh(&bl_hw->txq_lock);
}

#ifdef CONFIG_BL_FULLMAC

/**
 * bl_start_offchan_txq - START TX queue for offchannel frame
 *
 * @bl_hw: Driver main data
 */
void bl_txq_offchan_start(struct bl_hw *bl_hw)
{
    struct bl_txq *txq;

    txq = &bl_hw->txq[NX_OFF_CHAN_TXQ_IDX];
    spin_lock_bh(&bl_hw->txq_lock);
    bl_txq_start(txq, BL_TXQ_STOP_CHAN);
    spin_unlock_bh(&bl_hw->txq_lock);
}

/**
 * bl_switch_vif_sta_txq - Associate TXQ linked to a STA to a new vif
 *
 * @sta: STA whose txq must be switched
 * @old_vif: Vif currently associated to the STA (may no longer be active)
 * @new_vif: vif which should be associated to the STA for now on
 *
 * This function will switch the vif (i.e. the netdev) associated to all STA's
 * TXQ. This is used when AP_VLAN interface are created.
 * If one STA is associated to an AP_vlan vif, it will be moved from the master
 * AP vif to the AP_vlan vif.
 * If an AP_vlan vif is removed, then STA will be moved back to mastert AP vif.
 *
 */
void bl_txq_sta_switch_vif(struct bl_sta *sta, struct bl_vif *old_vif,
                             struct bl_vif *new_vif)
{
    struct bl_hw *bl_hw = new_vif->bl_hw;
    struct bl_txq *txq;
    int i;

    /* start TXQ on the new interface, and update ndev field in txq */
    if (!netif_carrier_ok(new_vif->ndev))
        netif_carrier_on(new_vif->ndev);
    txq = bl_txq_sta_get(sta, 0, NULL, bl_hw);
    for (i = 0; i < NX_NB_TID_PER_STA; i++, txq++) {
        txq->ndev = new_vif->ndev;
        netif_wake_subqueue(txq->ndev, txq->ndev_idx);
    }
}
#endif /* CONFIG_BL_FULLMAC */

/******************************************************************************
 * TXQ queue/schedule functions
 *****************************************************************************/
/**
 * bl_txq_queue_skb - Queue a buffer in a TX queue
 *
 * @skb: Buffer to queue
 * @txq: TX Queue in which the buffer must be added
 * @bl_hw: Driver main data
 * @retry: Should it be queued in the retry list
 *
 * @return: Retrun 1 if txq has been added to hwq list, 0 otherwise
 *
 * Add a buffer in the buffer list of the TX queue
 * and add this TX queue in the HW queue list if the txq is not stopped.
 * If this is a retry packet it is added after the last retry packet or at the
 * beginning if there is no retry packet queued.
 *
 * If the STA is in PS mode and this is the first packet queued for this txq
 * update TIM.
 *
 * To be called with tx_lock hold
 */
int bl_txq_queue_skb(struct sk_buff *skb, struct bl_txq *txq,
                       struct bl_hw *bl_hw,  bool retry)
{

#ifdef CONFIG_BL_FULLMAC
    if (unlikely(txq->sta && txq->sta->ps.active)) {
        txq->sta->ps.pkt_ready[txq->ps_id]++;
        trace_ps_queue(txq->sta);

        if (txq->sta->ps.pkt_ready[txq->ps_id] == 1) {
			BL_DBG("unlikely, sta was in ps mode!\n, txq->ps_id=%d\n", txq->ps_id);
            bl_set_traffic_status(bl_hw, txq->sta, true, txq->ps_id);
        }
    }
#endif

    spin_lock_irq(&bl_hw->txq_lock);

    if (!retry) {
        /* add buffer in the sk_list */
        skb_queue_tail(&txq->sk_list, skb);
    } else {
		if(skb==NULL)
			printk("skb = NULL\n");
		if(&txq->sk_list == NULL)
			printk("txq->sk_list NULL\n");

        if (txq->last_retry_skb) {
			BL_DBG("append last retry skb\n");
            skb_append(txq->last_retry_skb, skb, &txq->sk_list);
		}else{
			BL_DBG("append retry skb to head\n");
            skb_queue_head(&txq->sk_list, skb);
		}

        txq->last_retry_skb = skb;
        txq->nb_retry++;
    }

    trace_txq_queue_skb(skb, txq, retry);

    /* Flowctrl corresponding netdev queue if needed */
    /* If too many buffer are queued for this TXQ stop netdev queue */
    if ((txq->ndev_idx != NDEV_NO_TXQ) &&
        (skb_queue_len(&txq->sk_list) > BL_NDEV_FLOW_CTRL_STOP)) {
		//printk("stop: txq[%d], txq_netdev_idx[%d], skb_len[%d], txq->ndev->num_tx_queues[%d]\n", txq->idx, txq->ndev_idx, skb_queue_len(&txq->sk_list), txq->ndev->num_tx_queues);
        txq->status |= BL_TXQ_NDEV_FLOW_CTRL;
        netif_stop_subqueue(txq->ndev, txq->ndev_idx);
		//netif_tx_stop_all_queues(txq->ndev);
        trace_txq_flowctrl_stop(txq);
    }

    /* add it in the hwq list if not stopped and not yet present */
    if (!bl_txq_is_stopped(txq)) {
        bl_txq_add_to_hw_list(txq);
        spin_unlock_irq(&bl_hw->txq_lock);
        return 1;
    } else {
		BL_DBG("txq stopped, txq->status=0x%x\n", txq->status);
    }

    spin_unlock_irq(&bl_hw->txq_lock);

    return 0;
}

/**
 * bl_txq_confirm_any - Process buffer confirmed by fw
 *
 * @bl_hw: Driver main data
 * @txq: TX Queue
 * @hwq: HW Queue
 * @sw_txhdr: software descriptor of the confirmed packet
 *
 * Process a buffer returned by the fw. It doesn't check buffer status
 * and only does systematic counter update:
 * - hw credit
 * - buffer pushed to fw
 *
 * To be called with tx_lock hold
 */
void bl_txq_confirm_any(struct bl_hw *bl_hw, struct bl_txq *txq,
                          struct bl_hwq *hwq, struct bl_sw_txhdr *sw_txhdr)
{
    int user = 0;
	
    if (txq->pkt_pushed[user])
        txq->pkt_pushed[user]--;

    hwq->credits[user]++;
    bl_hw->stats.cfm_balance[hwq->id]--;
}

/******************************************************************************
 * HWQ processing
 *****************************************************************************/
static inline
s8 bl_txq_get_credits(struct bl_txq *txq)
{
    s8 cred = txq->credits;
#ifdef CONFIG_BL_FULLMAC
    /* if destination is in PS mode, push_limit indicates the maximum
       number of packet that can be pushed on this txq. */
    if (txq->push_limit && (cred > txq->push_limit)) {
        cred = txq->push_limit;
    }
#endif
    return cred;
}

/**
 * bl_txq_get_skb_to_push - Get list of buffer to push for one txq
 *
 * @bl_hw: main driver data
 * @hwq: HWQ on wich buffers will be pushed
 * @txq: TXQ to get buffers from
 * @user: user postion to use
 * @sk_list_push: list to update
 *
 *
 * This function will returned a list of buffer to push for one txq.
 * It will take into account the number of credit of the HWQ for this user
 * position and TXQ (and push_limit for fullmac).
 * This allow to get a list that can be pushed without having to test for
 * hwq/txq status after each push
 *
 * If a MU group has been selected for this txq, it will also update the
 * counter for the group
 *
 * @return true if txq no longer have buffer ready after the ones returned.
 *         false otherwise
 */
static
bool bl_txq_get_skb_to_push(struct bl_hw *bl_hw, struct bl_hwq *hwq,
                              struct bl_txq *txq, int user,
                              struct sk_buff_head *sk_list_push)
{
    int nb_ready = skb_queue_len(&txq->sk_list);
    int credits;
	int hwq_credits;
	int txq_credits;
	int i = 0, unlink_cnt = 0;
    bool res = false;
	int avail_port_num = 0;
	u32 wr_bitmap = bl_hw->plat->mp_wr_bitmap;
	struct bl_device *bl_device = (struct bl_device *)(bl_hw->plat->priv);
	struct sk_buff *skb = NULL, *tmp = NULL;

    skb_queue_head_init(sk_list_push);

	wr_bitmap &= 0xFFFFFFFE;
	while(wr_bitmap) {
		wr_bitmap = wr_bitmap & (wr_bitmap-1);
		avail_port_num++;
	}

    if (bl_hw->mod_params->tcp_ack_filter) {
        skb_queue_walk_safe(&txq->sk_list, skb, tmp) {
            if (bl_tcp_ack_process(bl_hw, skb, txq)) {
                __skb_unlink(skb, &txq->sk_list);
                unlink_cnt++;
            }
        }

        if(nb_ready == unlink_cnt)
            return true;
        else
            nb_ready -= unlink_cnt;
    }

	if(!(bl_hw->plat->mp_wr_bitmap & (1 << bl_hw->plat->curr_wr_port)) && (bl_hw->plat->mp_wr_bitmap & 0xFFFE)) {
		BL_DBG("bl_hw->plat->mp_wr_bitmap %x and curr_wr_port %d out of sync \n",bl_hw->plat->mp_wr_bitmap,bl_hw->plat->curr_wr_port);
		do{
			if (++(bl_hw->plat->curr_wr_port) == MAX_PORT_NUM)
				bl_hw->plat->curr_wr_port = bl_device->reg->start_wr_port;
		}while(!(bl_hw->plat->mp_wr_bitmap & (1 << bl_hw->plat->curr_wr_port)));
	}

	//hwq_credits = bl_hw->ipc_env->rb_len[hwq->id];
	txq_credits = bl_txq_get_credits(txq);
//	credits = min3(avail_port_num, txq_credits, hwq_credits);
	credits = min(avail_port_num, 8);

	//BL_DBG("bl_txq_get_skb_to_push: env->rb_len[%d]=%d\n", hwq->id, bl_hw->ipc_env->rb_len[hwq->id]);

	BL_DBG("nb_ready=%d, txq->idx=%d, hwq->idx=%d, txq->status=0x%x, txq->credits=%d, avail_port_num=%d, hwq_credits=%d, credits=%d\n", 
					nb_ready, txq->idx, hwq->id, txq->status, bl_txq_get_credits(txq), avail_port_num, hwq_credits, credits);


	if(credits == 0) {
		BL_DBG("credits=0, no availe port\n");
		return true;
	}

     if(bl_hw->plat->curr_wr_port + credits > 16)
         credits -= 1;

#if 0
    if (credits >= nb_ready) {
        skb_queue_splice_init(&txq->sk_list, sk_list_push);
        credits = nb_ready;
        res = true;
    } else {
        skb_queue_extract(&txq->sk_list, sk_list_push, credits);
#endif

    if(credits >= nb_ready) {
        skb_queue_extract(&txq->sk_list, sk_list_push, nb_ready);
        credits = nb_ready;
        res = true;
    } else {
        skb_queue_extract(&txq->sk_list, sk_list_push, credits);

#ifdef CONFIG_BL_FULLMAC
        /* When processing PS service period (i.e. push_limit != 0), no longer
           process this txq if this is a legacy PS service period (even if no
           packet is pushed) or the SP is complete for this txq */
        if (txq->push_limit &&
            ((txq->ps_id == LEGACY_PS_ID) ||
             (credits >= txq->push_limit)))
            res = true;
#endif
    }

    return res;
}


/**
 * bl_hwq_process - Process one HW queue list
 *
 * @bl_hw: Driver main data
 * @hw_queue: HW queue index to process
 *
 * The function will iterate over all the TX queues linked in this HW queue
 * list. For each TX queue, push as many buffers as possible in the HW queue.
 * (NB: TX queue have at least 1 buffer, otherwise it wouldn't be in the list)
 * - If TX queue no longer have buffer, remove it from the list and check next
 *   TX queue
 * - If TX queue no longer have credits or has a push_limit (PS mode) and it
 *   is reached , remove it from the list and check next TX queue
 * - If HW queue is full, update list head to start with the next TX queue on
 *   next call if current TX queue already pushed "too many" pkt in a row, and
 *   return
 *
 * To be called when HW queue list is modified:
 * - when a buffer is pushed on a TX queue
 * - when new credits are received
 * - when a STA returns from Power Save mode or receives traffic request.
 * - when Channel context change
 *
 * To be called with tx_lock hold
 */
#define ALL_HWQ_MASK  ((1 << CONFIG_USER_MAX) - 1)

void bl_hwq_process(struct bl_hw *bl_hw, struct bl_hwq *hwq)
{
    struct bl_txq *txq;	
    struct list_head *head, *next, *temp;
    int user = 0;

    trace_process_hw_queue(hwq);

    bl_hw->used_id = hwq->id;
    spin_lock_bh(&bl_hw->txq_lock);
	head = hwq->curr_list;
	next = head->next;
	
    // list_for_each_entry_safe(txq, next, /*&hwq->list*/head, sched_list) 
    do {
        struct sk_buff_head sk_list_push;
        bool txq_empty;

        trace_process_txq(txq);

        if(next == head) {
       	    BL_DBG("exit bl hwq process hwq id %d list empty, or polling done\n", hwq->id);		   	
    	    hwq->curr_list = head;
            break;
        }
		
        if(next == &hwq->list)
	   	    next = hwq->list.next;
	   	    
        if(next == &hwq->list) {
	        hwq->curr_list = &hwq->list;
	   
	        BL_DBG("exit bl hwq process as next still hwq list head\n");			
	        break;
        }

	    temp = next->next;
	    txq = container_of(next, typeof(*txq), sched_list);
		
		BL_DBG("---> txq->idx=%d, hwq->idx=%d, txq->status=0x%x, txq->credits=%d\n", txq->idx, hwq->id, txq->status, txq->credits);
        /* sanity check for debug */
		if(!(txq->status & BL_TXQ_IN_HWQ_LIST) && txq->idx == TXQ_INACTIVE) {
			if (txq->sched_list.prev == (struct list_head *)LIST_POISON2 || txq->sched_list.next == (struct list_head *)LIST_POISON1 || 
			    txq->sched_list.prev == NULL || txq->sched_list.next == NULL) 
			{
                //
            } else {
    			printk("txq was not in hwq, but why we probe this txq! txq->idx %x txq->status %x hwq->id %d\n",txq->idx,txq->status, hwq->id);
    			printk("curr %p hwq list %p txq list %p\n", hwq->curr_list, &hwq->list, &txq->sched_list);
    			list_del(&txq->sched_list);
    			hwq->curr_list = &hwq->list;
            }
		    break;
		}
		
        //BUG_ON(txq->idx == TXQ_INACTIVE);
        //BUG_ON(txq->credits <= 0);
		if(skb_queue_len(&txq->sk_list) == 0) {
			printk("there is no skb in txq...\n");
			//continue;
			goto end;
		}
		
        #ifdef BL_FW_RETRY
        do {
        #endif
            txq_empty = true;
            skb_queue_head_init(&sk_list_push);
            //if wlan0 down, another loop call bl_close->bl_txq_deinit, will inactivate the txq and flush skb. Check to avoid exception
            if ((txq->idx != TXQ_INACTIVE)) {
                txq_empty = bl_txq_get_skb_to_push(bl_hw, hwq, txq, user, &sk_list_push);		
            }
            spin_unlock_bh(&bl_hw->txq_lock);
            
            if(skb_queue_len(&sk_list_push) != 0) {
                bl_tx_multi_pkt_push(bl_hw,  &sk_list_push);
            }
            
            spin_lock_bh(&bl_hw->txq_lock);
        #ifdef BL_FW_RETRY
        } while(!txq_empty && (bl_hw->plat->mp_wr_bitmap & 0xfffe));
        #endif

        #ifdef BL_FW_RETRY
        bl_hw->data_sent = false;
        #endif

        if (txq_empty) {
			if(skb_queue_len(&txq->sk_list) == 0) {
				BL_DBG("txq sk_list empty, delete it\n");
            	bl_txq_del_from_hw_list(txq);
            	txq->pkt_sent = 0;
			} else {
				BL_DBG("txq sk_list is not empty!\n");
			}
        } else {
            BL_DBG("txq_empty=false, txq->credits=%d\n", txq->credits);
            //if(txq->credits != 0) {
            //    break;
            //}
        }

        #ifdef CONFIG_BL_FULLMAC
        /* Unable to complete PS traffic request because of hwq credit */
        if (txq->push_limit && txq->sta) {
			BL_DBG("Unable to complete PS traffic request because of hwq credit\n");
            if (txq->ps_id == LEGACY_PS_ID) {
                /* for legacy PS abort SP and wait next ps-poll */
				BL_DBG("for legacy PS abort SP and wait next ps-poll\n");
                txq->sta->ps.sp_cnt[txq->ps_id] -= txq->push_limit;
                txq->push_limit = 0;
            }
            /* for u-apsd need to complete the SP to send EOSP frame */
        }

        /* restart netdev queue if number of queued buffer is below threshold */
        if (unlikely(txq->status & BL_TXQ_NDEV_FLOW_CTRL) &&
            skb_queue_len(&txq->sk_list) < BL_NDEV_FLOW_CTRL_RESTART) {
			//BL_DBG("restart netdev queue if number of queued buffer is below threshold\n");
			//printk("restart:txq[%d], txq_netdev_idx[%d], skb_len[%d]\n", txq->idx, txq->ndev_idx, skb_queue_len(&txq->sk_list));
            txq->status &= ~BL_TXQ_NDEV_FLOW_CTRL;
            netif_wake_subqueue(txq->ndev, txq->ndev_idx);
			//netif_tx_wake_all_queues(txq->ndev);
            trace_txq_flowctrl_restart(txq);
        }
        #endif /* CONFIG_BL_FULLMAC */

		if (!(bl_hw->plat->mp_wr_bitmap & 0xfffe)) {
            if(txq->status & BL_TXQ_IN_HWQ_LIST)
                hwq->curr_list = next;
            else
                hwq->curr_list = &hwq->list;
                
            break;
        }
end:		
        next = temp;
        hwq->curr_list = temp;
    }while(next != head);
    
    spin_unlock_bh(&bl_hw->txq_lock);
}

/**
 * bl_hwq_process_all - Process all HW queue list
 *
 * @bl_hw: Driver main data
 *
 * Loop over all HWQ, and process them if needed
 * To be called with tx_lock hold
 */
void bl_hwq_process_all(struct bl_hw *bl_hw)
{
    int id;
    int start_id;
    int last_id;
    int actual_id;

    if(!(bl_hw->plat->mp_wr_bitmap & 0xfffe))
        return;

    if(bl_hw->last_hwq) {
        start_id = bl_hw->used_id + 1;
        last_id = bl_hw->used_id;

        do {
            actual_id = start_id % NX_TXQ_CNT;
            bl_hwq_process(bl_hw, &bl_hw->hwq[actual_id]);
            start_id++;
            if(!(bl_hw->plat->mp_wr_bitmap & 0xfffe))
                break;			
        }while(actual_id != last_id);
    } else {
        for (id = ARRAY_SIZE(bl_hw->hwq) - 1; id >= 0 ; id--) {
           	bl_hwq_process(bl_hw, &bl_hw->hwq[id]);
            if(!(bl_hw->plat->mp_wr_bitmap & 0xfffe))
                break;
        }
    }
}


/**
 * bl_hwq_init - Initialize all hwq structures
 *
 * @bl_hw: Driver main data
 *
 */
void bl_hwq_init(struct bl_hw *bl_hw)
{
    int i, j;

    for (i = 0; i < ARRAY_SIZE(bl_hw->hwq); i++) {
        struct bl_hwq *hwq = &bl_hw->hwq[i];

        for (j = 0 ; j < CONFIG_USER_MAX; j++)
            hwq->credits[j] = nx_txdesc_cnt[i];
        hwq->id = i;
        hwq->size = nx_txdesc_cnt[i];
        INIT_LIST_HEAD(&hwq->list);		
        hwq->curr_list = &hwq->list;
    }
}
