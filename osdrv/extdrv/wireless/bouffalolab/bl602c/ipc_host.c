/**
 ******************************************************************************
 *
 *  @file ipc_host.c
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


/*
 * INCLUDE FILES
 ******************************************************************************
 */
#include <linux/spinlock.h>
#include "bl_defs.h"

#include "ipc_host.h"

/*
 * TYPES DEFINITION
 ******************************************************************************
 */

const int nx_txdesc_cnt[] =
{
    NX_TXDESC_CNT0,
    NX_TXDESC_CNT1,
    NX_TXDESC_CNT2,
    NX_TXDESC_CNT3,
    #if NX_TXQ_CNT == 5
    NX_TXDESC_CNT4,
    #endif
};

const int nx_txdesc_cnt_msk[] =
{
    NX_TXDESC_CNT0 - 1,
    NX_TXDESC_CNT1 - 1,
    NX_TXDESC_CNT2 - 1,
    NX_TXDESC_CNT3 - 1,
    #if NX_TXQ_CNT == 5
    NX_TXDESC_CNT4 - 1,
    #endif
};

const int nx_txuser_cnt[] =
{
    CONFIG_USER_MAX,
    CONFIG_USER_MAX,
    CONFIG_USER_MAX,
    CONFIG_USER_MAX,
    #if NX_TXQ_CNT == 5
    1,
    #endif
};

#if 0
/**
 ******************************************************************************
 */
void *ipc_host_tx_flush(struct ipc_host_env_tag *env, const int queue_idx, const int user_pos)
{
    uint32_t used_idx = env->txdesc_used_idx[queue_idx][user_pos];
    void *host_id = env->tx_host_id[queue_idx][user_pos][used_idx & nx_txdesc_cnt_msk[queue_idx]];

    // call the external function to indicate that a TX packet is freed
    if (host_id != 0)
    {
        // Reset the host id in the array
        env->tx_host_id[queue_idx][user_pos][used_idx & nx_txdesc_cnt_msk[queue_idx]] = 0;

        // Increment the used index
        env->txdesc_used_idx[queue_idx][user_pos]++;
    }

    return (host_id);
}

/**
 ******************************************************************************
 */
void ipc_host_tx_cfm_handler(struct ipc_host_env_tag *env, const int queue_idx, const int user_pos, struct bl_hw_txhdr *hw_hdr, struct bl_txq **txq_saved)
{
	void *host_id = NULL;
	struct sk_buff *skb;
	uint32_t used_idx = env->txdesc_used_idx[queue_idx][user_pos] & nx_txdesc_cnt_msk[queue_idx];
	uint32_t free_idx = env->txdesc_free_idx[queue_idx][user_pos] & nx_txdesc_cnt_msk[queue_idx];
							
	host_id = env->tx_host_id[queue_idx][user_pos][used_idx];
	//ASSERT_ERR(host_id != NULL);
	if(!host_id){
        printk("%s: host id is null \n",__func__);		
		env->txdesc_used_idx[queue_idx][user_pos]++;
        goto exit;
	}
	env->tx_host_id[queue_idx][user_pos][used_idx] = 0;
	skb=host_id;

	BL_DBG("cfm skb=%p in %d of buffer, pkt_sn=%u\n", host_id, used_idx & nx_txdesc_cnt_msk[queue_idx], ((struct bl_txhdr *)(skb->data))->sw_hdr->hdr.reserved);

	if (env->cb.send_data_cfm(env->pthis, host_id, hw_hdr, (void **)txq_saved) != 0) {
		BL_DBG("send_data_cfm!=0, so break, used_idx=%d\n", used_idx);
		env->tx_host_id[queue_idx][user_pos][used_idx] = host_id;
	} else {
		env->txdesc_used_idx[queue_idx][user_pos]++;
	}

exit:
	if ((env->txdesc_used_idx[queue_idx][user_pos]&nx_txdesc_cnt_msk[queue_idx]) == free_idx) {
		BL_DBG("ipc_host_tx_cfm_handler: used_idx=free_idx=%d\n", free_idx);
		env->rb_len[queue_idx] = nx_txdesc_cnt[queue_idx];
	} else {
		BL_DBG("ipc_host_tx_cfm_handler: used_idx=%d, free_idx=%d\n", env->txdesc_used_idx[queue_idx][user_pos]&nx_txdesc_cnt_msk[queue_idx], free_idx);
		env->rb_len[queue_idx] = ((env->txdesc_used_idx[queue_idx][user_pos]&nx_txdesc_cnt_msk[queue_idx])-free_idx + nx_txdesc_cnt[queue_idx]) % nx_txdesc_cnt[queue_idx];
	}
	
	BL_DBG("ipc_host_tx_cfm_handler: env->rb_len[%d]=%d\n", queue_idx, env->rb_len[queue_idx]);
	BL_DBG("used_idx=%d--->%d\n", env->txdesc_used_idx[queue_idx][user_pos]-1, env->txdesc_used_idx[queue_idx][user_pos]);
}
#else
void *ipc_host_tx_flush(struct ipc_host_env_tag *env, const int queue_idx, const int user_pos)
{
	struct bl_hw * bl_hw = NULL;

	if((bl_hw = (struct bl_hw *)env->pthis) == NULL)
		return NULL;

	return (void *)skb_dequeue(&bl_hw->transmitted_list[queue_idx]);
}

void ipc_host_tx_cfm_handler(struct ipc_host_env_tag *env, const int queue_idx, const int user_pos, struct bl_hw_txcfm *hw_hdr, struct bl_txq **txq_saved)
{
    u8_l staid = 0, tid = 0, vif = 0, txq_num = 0;
    struct sk_buff *skb = NULL;
    struct bl_hw * bl_hw = NULL;

    if((bl_hw = (struct bl_hw *)env->pthis) == NULL)
        return;

    staid = hw_hdr->staid;
    tid = hw_hdr->tid;

    /* when staid = 10/11/12/13, it is None-STA interface, we use TXQ[90] for TX*/
    if(staid >= NX_REMOTE_STA_MAX)
        tid = 0;

    staid = min(staid, NX_REMOTE_STA_MAX);
    tid = min(tid, NX_NB_TXQ_PER_STA - 1);
    vif = min(vif, NX_VIRT_DEV_MAX - 1);

    txq_num = staid * NX_NB_TXQ_PER_STA + tid + vif;

    skb = skb_dequeue(&bl_hw->transmitted_list[txq_num]);
    if(!skb){
        BL_DBG("%s: host id is null qlen=%d\n", skb_queue_len(&bl_hw->transmitted_list[txq_num]));
        return;
    }

    if (env->cb.send_data_cfm(env->pthis, skb, hw_hdr, (void **)txq_saved) != 0) {
        BL_DBG("send_data_cfm!=0, so break, skb_wait_len=%d\n", skb_queue_len(&bl_hw->transmitted_list[txq_num]));
    }
}
#endif

/**
 ******************************************************************************
 */
void ipc_host_init(struct ipc_host_env_tag *env,
                  struct ipc_host_cb_tag *cb,
                  void *pthis)
{
    unsigned int i;
    // Reset the IPC Host environment
    memset(env, 0, sizeof(struct ipc_host_env_tag));

    // Save the callbacks in our own environment
    env->cb = *cb;

    // Save the pointer to the register base
    env->pthis = pthis;

}

#if 0
/**
 ******************************************************************************
 */
void ipc_host_txdesc_push(struct ipc_host_env_tag *env, const int queue_idx,
                          const int user_pos, void *host_id)
{
#ifdef CONFIG_BL_DBG
    struct sk_buff *skb = host_id;
#endif
    uint32_t free_idx = env->txdesc_free_idx[queue_idx][user_pos] & nx_txdesc_cnt_msk[queue_idx];
	uint32_t used_idx = env->txdesc_used_idx[queue_idx][user_pos] & nx_txdesc_cnt_msk[queue_idx];

	BL_DBG("save skb=%p in %d of buffer, pkt_sn=%u\n", host_id, free_idx, ((struct bl_txhdr *)((struct sk_buff *)skb->data))->sw_hdr->hdr.reserved);

    // Save the host id in the environment
    env->tx_host_id[queue_idx][user_pos][free_idx] = host_id;

    if((free_idx + 1) % nx_txdesc_cnt[queue_idx] == used_idx) {
		BL_DBG("queue is full: free_idx=%d, used_idx=%d\n", free_idx, used_idx);
		env->txdesc_free_idx[queue_idx][user_pos]++;
		env->rb_len[queue_idx] = 0;
    } else {
	   	env->txdesc_free_idx[queue_idx][user_pos]++;
		BL_DBG("ipc_host_txdesc_push: used_idx=%d, free_idx=%d\n", used_idx, env->txdesc_free_idx[queue_idx][user_pos]&nx_txdesc_cnt_msk[queue_idx]);
		env->rb_len[queue_idx] = (used_idx-(env->txdesc_free_idx[queue_idx][user_pos]&nx_txdesc_cnt_msk[queue_idx]) + nx_txdesc_cnt[queue_idx]) % nx_txdesc_cnt[queue_idx];
    }

	BL_DBG("ipc_host_txdesc_push: env->rb_len[%d]=%d\n", queue_idx, env->rb_len[queue_idx]);
	BL_DBG("queue_idx[%d], free_idx: %d--->%d\n", queue_idx, env->txdesc_free_idx[queue_idx][user_pos] - 1, env->txdesc_free_idx[queue_idx][user_pos]);
}
#else
int ipc_host_txdesc_push(struct ipc_host_env_tag *env, const int queue_idx,
                          const int user_pos, void *host_id)
{
    u8_l staid = 0, tid = 0, vif = 0, txq_num = 0;
    struct sk_buff *skb = host_id;
    struct bl_hw * bl_hw = (struct bl_hw *)env->pthis;
    struct bl_txhdr * txhdr = (struct bl_txhdr *)skb->data;

    if(!skb)
        return -1;

    staid = txhdr->sw_hdr->desc.host.staid;
    tid = txhdr->sw_hdr->desc.host.tid;
    /* current TXCFM not include vif, use 0 for all vif */
    vif = 0;

    /* when staid = 10/11/12/13, it is None-STA interface, we use TXQ[90] for TX*/
    if(staid >= NX_REMOTE_STA_MAX)
        tid = 0;

    staid = min(staid, NX_REMOTE_STA_MAX);
    tid = min(tid, NX_NB_TXQ_PER_STA - 1);
    vif = min(vif, NX_VIRT_DEV_MAX - 1);

    txq_num = staid * NX_NB_TXQ_PER_STA + tid + vif;

    skb_queue_tail(&bl_hw->transmitted_list[txq_num], skb);

    return txq_num;
}
#endif
