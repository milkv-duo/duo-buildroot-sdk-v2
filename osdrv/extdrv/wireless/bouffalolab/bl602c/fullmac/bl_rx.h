/**
 ******************************************************************************
 *
 *  @file bl_rx.h
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

#ifndef _BL_RX_H_
#define _BL_RX_H_

#define RX_WIN_SIZE 64

#ifdef  BL_RX_REORDER
struct rxreorder_list {
	struct bl_hw *bl_hw;
	u8 flag;
	u8 check_start_win;
	u8 is_timer_set;
	u8 flush;
	u8 del_ba;
	u16 record_start_win;
	spinlock_t rx_lock;
	struct timer_list timer;
    u16 start_win;
    u16 end_win;
    u16 last_seq;
	u16 win_size;
	u16 start_win_index;
	u16 cnt;
    void *reorder_pkt[RX_WIN_SIZE];
};
#endif

#ifdef  BL_RX_DEFRAG
#define DEFRAG_SKB_CNT       (16)
#ifndef DEFRAG_LIST
/* Size of the pool containing Reassembly structure */
#define BL_RX_DEFRAG_POOL_SIZE       (8)
#endif

struct rxdefrag_list {
	struct bl_hw *bl_hw;
    /** Pointer to the next element in the queue */
    struct list_head list;
    /** Station Index */
    u8 sta_idx;
    /** Traffic Index */
    u8 tid;
    /** Next Expected FN */
    u8 next_fn;
    /** Sequence Number */
    u16 sn;
    /** Length to be copied */
    u16 cpy_len;	
    /** Total Frame Length */
    u16 frame_len;
    /** fragment skb buffer	pointer array */
	void *pkt[DEFRAG_SKB_CNT];
    /** lock */	
	spinlock_t rx_lock;
    /** timer used for flushing the fragments if the packet is not complete */
	bool is_timer_set;	
	struct timer_list timer;
};
#endif
enum rx_status_bits
{
    /// The buffer can be forwarded to the networking stack
    RX_STAT_FORWARD = 1 << 0,
    /// A new buffer has to be allocated
    RX_STAT_ALLOC = 1 << 1,
    /// The buffer has to be deleted
    RX_STAT_DELETE = 1 << 2,
    /// The length of the buffer has to be updated
    RX_STAT_LEN_UPDATE = 1 << 3,
    /// The length in the Ethernet header has to be updated
    RX_STAT_ETH_LEN_UPDATE = 1 << 4,
    /// Simple copy
    RX_STAT_COPY = 1 << 5,
};


/*
 * Decryption status subfields.
 * {
 */
#define BL_RX_HD_DECR_UNENC           0 // Frame unencrypted
#define BL_RX_HD_DECR_ICVFAIL         1 // WEP/TKIP ICV failure
#define BL_RX_HD_DECR_CCMPFAIL        2 // CCMP failure
#define BL_RX_HD_DECR_AMSDUDISCARD    3 // A-MSDU discarded at HW
#define BL_RX_HD_DECR_NULLKEY         4 // NULL key found
#define BL_RX_HD_DECR_WEPSUCCESS      5 // Security type WEP
#define BL_RX_HD_DECR_TKIPSUCCESS     6 // Security type TKIP
#define BL_RX_HD_DECR_CCMPSUCCESS     7 // Security type CCMP
// @}

struct hw_vect {
    /** Total length for the MPDU transfer */
    u32 len                   :16;

    u32 reserved              : 8;

    /** AMPDU Status Information */
    u32 mpdu_cnt              : 6;
    u32 ampdu_cnt             : 2;


    /** TSF Low */
    __le32 tsf_lo;
    /** TSF High */
    __le32 tsf_hi;

    /** Receive Vector 1a */
    u32    leg_length         :12;
    u32    leg_rate           : 4;
    u32    ht_length          :16;

    /** Receive Vector 1b */
    u32    _ht_length         : 4; // FIXME
    u32    short_gi           : 1;
    u32    stbc               : 2;
    u32    smoothing          : 1;
    u32    mcs                : 7;
    u32    pre_type           : 1;
    u32    format_mod         : 3;
    u32    ch_bw              : 2;
    u32    n_sts              : 3;
    u32    lsig_valid         : 1;
    u32    sounding           : 1;
    u32    num_extn_ss        : 2;
    u32    aggregation        : 1;
    u32    fec_coding         : 1;
    u32    dyn_bw             : 1;
    u32    doze_not_allowed   : 1;

    /** Receive Vector 1c */
    u32    antenna_set        : 8;
    u32    partial_aid        : 9;
    u32    group_id           : 6;
    u32    reserved_1c        : 1;
    s32    rssi1              : 8;

    /** Receive Vector 1d */
    s32    rssi2              : 8;
    s32    rssi3              : 8;
    s32    rssi4              : 8;
    u32    reserved_1d        : 8;

    /** Receive Vector 2a */
    u32    rcpi               : 8;
    u32    evm1               : 8;
    u32    evm2               : 8;
    u32    evm3               : 8;

    /** Receive Vector 2b */
    u32    evm4               : 8;
    u32    reserved2b_1       : 8;
    u32    reserved2b_2       : 8;
    u32    reserved2b_3       : 8;

    /** Status **/
    u32    rx_vect2_valid     : 1;
    u32    resp_frame         : 1;
    /** Decryption Status */
    u32    decr_status        : 3;
    u32    rx_fifo_oflow      : 1;

    /** Frame Unsuccessful */
    u32    undef_err          : 1;
    u32    phy_err            : 1;
    u32    fcs_err            : 1;
    u32    addr_mismatch      : 1;
    u32    ga_frame           : 1;
    u32    current_ac         : 2;

    u32    frm_successful_rx  : 1;
    /** Descriptor Done  */
    u32    desc_done_rx       : 1;
    /** Key Storage RAM Index */
    u32    key_sram_index     : 10;
    /** Key Storage RAM Index Valid */
    u32    key_sram_v         : 1;
    u32    type               : 2;
    u32    subtype            : 4;
};

struct hw_rxhdr {
    /** RX vector */
    struct hw_vect hwvect;

    /** PHY channel information 1 */
    u32    phy_band           : 8;
    u32    phy_channel_type   : 8;
    u32    phy_prim20_freq    : 16;
    /** PHY channel information 2 */
    u32    phy_center1_freq   : 16;
    u32    phy_center2_freq   : 16;
    /** RX flags */
    u32    flags_is_amsdu     : 1;
    u32    flags_is_80211_mpdu: 1;
    u32    flags_is_4addr     : 1;
    u32    flags_new_peer     : 1;
    u32    flags_user_prio    : 3;
    u32    flags_is_bar        : 1;
    u32    flags_vif_idx      : 8;    // 0xFF if invalid VIF index
    u32    flags_sta_idx      : 8;    // 0xFF if invalid STA index
    u32    flags_dst_idx      : 8;    // 0xFF if unknown destination STA
    u16    sn;
    u16	   tid;

    /** User flags  */
    u16   flags_mf          : 1;     // more frag
    u16   flags_is_pn_check : 1;     // need driver check pn
    u16   flags_rsved       : 14;    // rsved

    /// frag number
    u16   fn;
    /// rx pn
    u64   pn;
}__packed;

struct bl_agg_reodr_msg {
    u16 sn;
    u8 sta_idx;
    u8 tid;
    u8 status;
	u8 num;
}__packed;

extern const u8 legrates_lut[];
#define BL_FC_GET_TYPE(fc)	(((fc) & 0x000c) >> 2)
#define BL_FC_GET_STYPE(fc)	(((fc) & 0x00f0) >> 4)

#define AGG_REORD_MSG_MAX_NUM 4096

#ifdef  BL_RX_REORDER
u8 bl_rx_packet_dispatch(struct bl_hw *bl_hw, struct sk_buff *skb);
void bl_rx_reorder_flush(struct timer_list *t);
#endif
u8 bl_rxdataind(void *pthis, void *hostid);
void bl_rx_wq_hdlr(struct work_struct *work);
#ifdef  BL_RX_DEFRAG
void bl_rx_defrag_clean(struct bl_hw *bl_hw);
struct sk_buff * bl_rx_defrag_check(struct bl_hw *bl_hw, struct sk_buff *skb, int payload_offset);
#endif

#endif /* _BL_RX_H_ */
