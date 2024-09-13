/**
 ******************************************************************************
 *
 *  @file bl_cmds.c
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


#include <linux/list.h>

#include "bl_cmds.h"
#include "bl_defs.h"
#include "bl_msg_tx.h"
#include "bl_strs.h"
#define CREATE_TRACE_POINTS
#include "bl_ftrace.h"
#include "bl_debugfs.h"
#include "bl_sdio.h"
#include "bl_v7.h"
#include "bl_irqs.h"
#include "bl_main.h"
#include "bl_nl_events.h"

static void cmd_mgr_print(struct bl_cmd_mgr *cmd_mgr);

void cmd_dump(const struct bl_cmd *cmd)
{
    printk("tkn[%d]  flags:%04x  result:%3d  cmd:%4d-%-24s - reqcfm(%4d-%-s)\n",
           cmd->tkn, cmd->flags, cmd->result, cmd->id, BL_ID2STR(cmd->id),
           cmd->reqid, cmd->reqid != (lmac_msg_id_t)-1 ? BL_ID2STR(cmd->reqid) : "none");
}

void dump_fw_criticals(struct bl_hw *bl_hw) {
    int i = 0;
    uint8_t ind_byte = 3;
    uint32_t c_inv_addr = 0xffffffff;
    uint32_t c_inv_value = 0xffffffff;
    uint32_t wr_mask = 0x80;
    uint32_t rd_mask = 0x7f;
    uint32_t scratch_reg = 0x60;
    const int dump_cnt = 8;
    uint8_t dump_data[8];
    uint8_t dump_buf[50];
    uint32_t value = 0;
    uint32_t addr = 0;
    // uint32_t read_cnt = 0;
    uint32_t read_cnt_max = 5000; //ms
    unsigned long jiffies_end = jiffies;
    uint32_t cnt_print = 0;
    static uint8_t dump_fw_started = 0;
	struct file *p_dump_file = NULL;
	unsigned char file_name[64];
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
	mm_segment_t fs;
#endif
	uint32_t len = 0;

    //multiple cmd timeout, may trigger multiple dump
    if (dump_fw_started) {
        printk(KERN_CRIT"dump fw already started\n");
        return;
    }
    
    dump_fw_started = 1;

    printk(KERN_CRIT"dump fw memory\n");
    
	for (i=0; i<4; i++) {
		bl_write_reg(bl_hw, scratch_reg+i, 0x00);
	}

    i = 0;
    while (i++ < read_cnt_max) {
        int j = 0;
        msleep_interruptible(1);
        for (j=0; j<dump_cnt; j++) {
            bl_read_reg(bl_hw, scratch_reg+j, dump_data+j);
        }
        if (*(uint32_t *)dump_data == c_inv_addr && *(uint32_t *)(dump_data + 4) == c_inv_value) {
            printk(KERN_CRIT"dump fw start\n");
            bl_write_reg(bl_hw, scratch_reg + ind_byte, dump_data[ind_byte]&rd_mask);
            break;
        }
    }
    if (i >= read_cnt_max) {
        printk(KERN_CRIT"dump fw, wait starter timeout.\n");
        return;
    }

    do {
        if (bl_hw->mod_params->dump_dir != NULL) {
            memset(file_name, 0, sizeof(file_name));
            sprintf(file_name, "%s/%s_%ld.txt", bl_hw->mod_params->dump_dir, "bl_fw_dump", jiffies_end);
            p_dump_file = filp_open(file_name, O_CREAT | O_RDWR, 0644);
            if (IS_ERR(p_dump_file)) {
                printk(KERN_CRIT"dump fw, create file %s error, try other folder", file_name);
            } else {
                printk(KERN_CRIT"dump fw data in %s\n", file_name);
                break;
            }
        }

        memset(file_name, 0, sizeof(file_name));
        sprintf(file_name, "%s/%s_%ld.txt", "/data", "bl_fw_dump", jiffies_end);
        p_dump_file = filp_open(file_name, O_CREAT | O_RDWR, 0644);
        if (IS_ERR(p_dump_file)) {
            printk(KERN_CRIT"dump fw, create file %s error, try create /var/log/bl_fw_dump.txt", file_name);
        } else {
            printk(KERN_CRIT"dump fw data in %s\n", file_name);
            break;
        }

        memset(file_name, 0, sizeof(file_name));
        sprintf(file_name, "%s/%s_%ld.txt", "/var/log", "bl_fw_dump", jiffies_end);
        p_dump_file = filp_open(file_name, O_CREAT | O_RDWR, 0644);
        if (IS_ERR(p_dump_file)) {
            printk(KERN_CRIT"dump fw, create file %s failed, no file to dump, only printk to dmesg\n", file_name);
            p_dump_file = NULL;
        } else {
            printk(KERN_CRIT"dump fw data in %s\n", file_name);
            break;
        }
    } while(0);

    if (p_dump_file != NULL) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
        fs = get_fs();
        set_fs(KERNEL_DS);
#endif
    }

    memset(dump_data, 0, dump_cnt);
    jiffies_end = jiffies + HZ*read_cnt_max/1000;
    while ((*(uint32_t *)dump_data) != c_inv_addr) {
        // if ((read_cnt++%10) == 0) {
        //     msleep_interruptible(1);
        // }

        //Avoid fw/board is gone, while becomes dead loop
        if (time_after(jiffies, jiffies_end)) {
            printk(KERN_CRIT"dump fw, timeout when dumping.\n");
            break;
        }

        for (i=0; i<dump_cnt; i++) {
            bl_read_reg(bl_hw, scratch_reg+i, dump_data+i);
        }

        //Receive indication flag that fw is done to write.
        if ((dump_data[ind_byte]&wr_mask) > 0) {
            for (i=0; i<dump_cnt; i++) {
                bl_read_reg(bl_hw, scratch_reg+i, dump_data+i);
            }
            
            //Send indiation to fw that it is OK to read
            bl_write_reg(bl_hw, scratch_reg+ind_byte, dump_data[ind_byte]&rd_mask);
            
            addr = ((*(uint32_t *)dump_data) & 0x7fffffff);
            value = *(uint32_t *)(dump_data + 4);
            
            if ((cnt_print++ % 1000) == 0)
                printk(KERN_CRIT"dump addr:0x%08x=0x%08x:value\n", addr, value);

            if (p_dump_file != NULL) {
                len = sprintf(dump_buf, "dump addr:0x%08x=0x%08x:value\n", addr, value);
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
                vfs_write(p_dump_file, dump_buf, len, &p_dump_file->f_pos);
#else
                kernel_write(p_dump_file, dump_buf, len, &p_dump_file->f_pos);
#endif
            }

            jiffies_end = jiffies + HZ*read_cnt_max/1000;
        }
    }

    if (p_dump_file != NULL) {
        printk(KERN_CRIT"dump fw successfully to file %s\n", file_name);
    	filp_close(p_dump_file, NULL);
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
	    set_fs(fs);
#endif
    }

    printk(KERN_CRIT"dump fw end\n");

    return;
}

void dump_info(struct bl_hw *bl_hw)
{
	struct bl_cmd *cmd = &bl_hw->last_cmd;
	struct bl_plat *bl_plat;
    int i = 0;
    int trapDataNum = 8;
    u8 trapDbgData;

	bl_plat = bl_hw->plat;

	for(i=0; i< trapDataNum; i++){
		bl_read_reg(bl_hw, 0x60 + i, &trapDbgData);
		printk("cmd timeout %d epc/tval 0x%2x \n", i, trapDbgData);
	}
	bl_read_data_sync(bl_hw, bl_hw->plat->mp_regs, 64, REG_PORT);
	printk("SDIO reg hostintstatus val:%x \n", bl_hw->plat->mp_regs[3]);			
	printk("SDIO reg rdbitmap val:%x \n", bl_hw->plat->mp_regs[4]|(bl_hw->plat->mp_regs[5]<<8));			
	printk("SDIO reg wrbitmap val:%x \n", bl_hw->plat->mp_regs[6]|(bl_hw->plat->mp_regs[7]<<8));			
	printk("SDIO reg CardIntStatus val:%x \n", bl_hw->plat->mp_regs[0x38]);
	bl_read_fun0_reg(bl_hw, 0x04, &trapDbgData);
	printk("sdio fun0 reg0x4 %x\n", trapDbgData);
	bl_read_fun0_reg(bl_hw, 0x05, &trapDbgData);
	printk("sdio fun0 reg0x5 %x\n", trapDbgData);

	printk("mp_rb=0x%x, cur_rp=%d, mp_wb=0x%x, cur_wp=%d\n", bl_plat->mp_rd_bitmap, bl_hw->plat->curr_rd_port, bl_plat->mp_wr_bitmap, bl_hw->plat->curr_wr_port);

    printk("last cmd: tkn[%d]  flags:%04x  result:%3d  cmd:%4d-%-24s - reqcfm(%4d-%-s)\n",
           cmd->tkn, cmd->flags, cmd->result, cmd->id, BL_ID2STR(cmd->id),
           cmd->reqid, cmd->reqid != (lmac_msg_id_t)-1 ? BL_ID2STR(cmd->reqid) : "none");
    printk("last cmd index %d \n", (bl_hw->last_cmd_idx % 8 -1));
    for (i = 0; i < 8; i++) {
       cmd = &bl_hw->saved_cmd[i];
       printk("cmd[%d]: tkn[%d]  flags:%04x  result:%3d  cmd:%4d-%-24s - reqcfm(%4d-%-s)\n",
		  i, cmd->tkn, cmd->flags, cmd->result, cmd->id, BL_ID2STR(cmd->id),
		  cmd->reqid, cmd->reqid != (lmac_msg_id_t)-1 ? BL_ID2STR(cmd->reqid) : "none");

   }
#ifdef CONFIG_BL_DUMP_FW 
	dump_fw_criticals(bl_hw);
#endif

}

static void cmd_complete(struct bl_cmd_mgr *cmd_mgr, struct bl_cmd *cmd)
{
    lockdep_assert_held(&cmd_mgr->lock);

    list_del(&cmd->list);
    cmd_mgr->queue_sz--;

    cmd->flags |= BL_CMD_FLAG_DONE;
    if (cmd->flags & BL_CMD_FLAG_NONBLOCK) {
        kfree(cmd);
    } else {
        if (BL_CMD_WAIT_COMPLETE(cmd->flags)) {
            cmd->result = 0;
            complete(&cmd->complete);
        }
    }
}

static int cmd_mgr_queue(struct bl_cmd_mgr *cmd_mgr, struct bl_cmd *cmd)
{
    struct bl_hw *bl_hw = container_of(cmd_mgr, struct bl_hw, cmd_mgr);
    struct bl_cmd *last;
    unsigned long tout;
    bool defer_push = false;
    u16 flags;

    BL_DBG(BL_FN_ENTRY_STR);
    trace_msg_send(cmd->id);
	
	if(bl_hw->surprise_removed)
		return 0;

    spin_lock_bh(&cmd_mgr->lock);

    if (cmd_mgr->state == BL_CMD_MGR_STATE_CRASHED) {
        printk(KERN_CRIT"cmd queue crashed\n");
        cmd->result = -EPIPE;
        spin_unlock_bh(&cmd_mgr->lock);
        //dump_info(bl_hw);
        return -EPIPE;
    }

    if (!list_empty(&cmd_mgr->cmds)) {
        if (cmd_mgr->queue_sz == cmd_mgr->max_queue_sz) {
            printk(KERN_CRIT"Too many cmds (%d) already queued\n",
                   cmd_mgr->max_queue_sz);
            cmd->result = -ENOMEM;
            spin_unlock_bh(&cmd_mgr->lock);
            cmd_mgr_print(cmd_mgr);
            return -ENOMEM;
        }
        last = list_entry(cmd_mgr->cmds.prev, struct bl_cmd, list);
        if (last->flags & (BL_CMD_FLAG_WAIT_ACK | BL_CMD_FLAG_WAIT_PUSH)) {
            cmd->flags |= BL_CMD_FLAG_WAIT_PUSH;
            defer_push = true;
        }
    }

    cmd->flags |= BL_CMD_FLAG_WAIT_ACK;
    if (cmd->flags & BL_CMD_FLAG_REQ_CFM)
        cmd->flags |= BL_CMD_FLAG_WAIT_CFM;
	
    flags = cmd->flags;
    cmd->tkn    = cmd_mgr->next_tkn++;
    cmd->result = -EINTR;

    if (!(flags & BL_CMD_FLAG_NONBLOCK))
        init_completion(&cmd->complete);

    list_add_tail(&cmd->list, &cmd_mgr->cmds);
    cmd_mgr->queue_sz++;
    tout = msecs_to_jiffies(BL_80211_CMD_TIMEOUT_MS + BL_80211_CMD_TIMEOUT_MS * ((cmd_mgr->queue_sz > 0) ? (cmd_mgr->queue_sz - 1) : 0));
    spin_unlock_bh(&cmd_mgr->lock);

    if (!defer_push) {
		ASSERT_ERR(!(bl_hw->ipc_env->msga2e_hostid));
		bl_hw->ipc_env->msga2e_hostid = (void *)cmd;
		spin_lock_bh(&bl_hw->cmd_lock);
		bl_hw->cmd_sent = true;
		spin_unlock_bh(&bl_hw->cmd_lock);
		bl_queue_main_work(bl_hw);
    }

    BL_DBG("send: cmd:%4d-%-24s\n", cmd->id, BL_ID2STR(cmd->id));
	

    if (!(flags & BL_CMD_FLAG_NONBLOCK)) {
        if (!wait_for_completion_timeout(&cmd->complete, tout)) {
			printk("wait for cmd cfm timeout (%d)!\n", tout);
            cmd_dump(cmd);
            bl_hw->last_cmd = *cmd;
            //If multi cmd timeout, may trigger multi dump_info.
            dump_info(bl_hw);
			
            spin_lock_bh(&cmd_mgr->lock);
            if (!(cmd->flags & BL_CMD_FLAG_DONE)) {				
				cmd_mgr->state = BL_CMD_MGR_STATE_CRASHED;
                cmd->result = -ETIMEDOUT;
                cmd_complete(cmd_mgr, cmd);
            }
            spin_unlock_bh(&cmd_mgr->lock);			
            bl_nl_broadcast_event(bl_hw, BL_EVENT_ID_RESET, NULL, 0);
        }else {
            bl_hw->last_cmd = *cmd;
        }
    } else {
        cmd->result = 0;		
        bl_hw->last_cmd = *cmd;
    }
    // update cmd queue
    bl_hw->saved_cmd[bl_hw->last_cmd_idx % 8] = *cmd;
    bl_hw->last_cmd_idx ++;

    return 0;
}

static int cmd_mgr_llind(struct bl_cmd_mgr *cmd_mgr, struct bl_cmd *cmd)
{
    struct bl_cmd *cur, *acked = NULL, *next = NULL;
	struct bl_hw *bl_hw = container_of(cmd_mgr, struct bl_hw, cmd_mgr);
	
    BL_DBG(BL_FN_ENTRY_STR);

    spin_lock(&cmd_mgr->lock);
    if (cmd) {
        //cmd_dump(cmd);
    } else {
        printk("%s, cmd null\r\n", __func__);
    }

    list_for_each_entry(cur, &cmd_mgr->cmds, list) {
        if (!acked) {
            if (cmd && cur->tkn == cmd->tkn) {
                if (WARN_ON_ONCE(cur != cmd)) {
					cmd_mgr_print(cmd_mgr);
                    //cmd_dump(cmd);
                }
                acked = cur;
                continue;
            }
        }
        if (cur->flags & BL_CMD_FLAG_WAIT_PUSH) {
                next = cur;
                break;
        }
    }
    if (!acked) {
        if (cmd)
            printk(KERN_CRIT "%s Error: acked cmd(%d) not found, next:0x%x\n",__func__,cmd->id, next);
        else
            printk(KERN_CRIT "%s Error: acked cmd NULL, next:0x%x\n",__func__,next);
    } else {
        cmd->flags &= ~BL_CMD_FLAG_WAIT_ACK;
        if (BL_CMD_WAIT_COMPLETE(cmd->flags)) {
            cmd_complete(cmd_mgr, cmd);
		}
    }
    if (next) {
		ASSERT_ERR(!(bl_hw->ipc_env->msga2e_hostid));
		bl_hw->ipc_env->msga2e_hostid = (void *)next;
		spin_lock_bh(&bl_hw->cmd_lock);
		bl_hw->cmd_sent = true;
		spin_unlock_bh(&bl_hw->cmd_lock);
		bl_queue_main_work(bl_hw);
    }
    spin_unlock(&cmd_mgr->lock);

    return 0;
}

static int cmd_mgr_run_callback(struct bl_hw *bl_hw, struct bl_cmd *cmd,
                                struct ipc_e2a_msg *msg, msg_cb_fct cb)
{
    int res;

    if (! cb)
        return 0;

    spin_lock(&bl_hw->cb_lock);
    res = cb(bl_hw, cmd, msg);
    spin_unlock(&bl_hw->cb_lock);

    return res;
}

static int cmd_mgr_msgind(struct bl_cmd_mgr *cmd_mgr, struct ipc_e2a_msg *msg, msg_cb_fct cb)
{
    struct bl_hw *bl_hw = container_of(cmd_mgr, struct bl_hw, cmd_mgr);
    struct bl_cmd *cmd;
    bool found = false;

    BL_DBG(BL_FN_ENTRY_STR);
    trace_msg_recv(msg->id);

    spin_lock(&cmd_mgr->lock);
    list_for_each_entry(cmd, &cmd_mgr->cmds, list) {
        if (cmd->reqid == msg->id &&
            (cmd->flags & BL_CMD_FLAG_WAIT_CFM)) {
            if (!cmd_mgr_run_callback(bl_hw, cmd, msg, cb)) {
                found = true;
                cmd->flags &= ~(BL_CMD_FLAG_WAIT_CFM | BL_CMD_FLAG_WAIT_ACK);

                if (cmd->e2a_msg && msg->param_len)
                    memcpy(cmd->e2a_msg, &msg->param, msg->param_len);

                if (BL_CMD_WAIT_COMPLETE(cmd->flags)){
                    cmd_complete(cmd_mgr, cmd);
				}

                break;
            }
        }
    }
    spin_unlock(&cmd_mgr->lock);

    if (!found)
	{
        cmd_mgr_run_callback(bl_hw, NULL, msg, cb);
	}

    return 0;
}

static void cmd_mgr_print(struct bl_cmd_mgr *cmd_mgr)
{
    struct bl_cmd *cur;

    spin_lock_bh(&cmd_mgr->lock);
    BL_DBG("q_sz/max: %2d / %2d - next tkn: %d\n",
             cmd_mgr->queue_sz, cmd_mgr->max_queue_sz,
             cmd_mgr->next_tkn);
    list_for_each_entry(cur, &cmd_mgr->cmds, list) {
        cmd_dump(cur);
    }
    spin_unlock_bh(&cmd_mgr->lock);
}

static void cmd_mgr_drain(struct bl_cmd_mgr *cmd_mgr)
{
    struct bl_cmd *cur, *nxt;

    BL_DBG(BL_FN_ENTRY_STR);

    spin_lock_bh(&cmd_mgr->lock);
    list_for_each_entry_safe(cur, nxt, &cmd_mgr->cmds, list) {
        list_del(&cur->list);
        cmd_mgr->queue_sz--;
        if (!(cur->flags & BL_CMD_FLAG_NONBLOCK))
            complete(&cur->complete);
    }
    spin_unlock_bh(&cmd_mgr->lock);
}

void bl_cmd_mgr_init(struct bl_cmd_mgr *cmd_mgr)
{
    BL_DBG(BL_FN_ENTRY_STR);

    INIT_LIST_HEAD(&cmd_mgr->cmds);
    spin_lock_init(&cmd_mgr->lock);
    cmd_mgr->max_queue_sz = BL_CMD_MAX_QUEUED;
    cmd_mgr->queue  = &cmd_mgr_queue;
    cmd_mgr->print  = &cmd_mgr_print;
    cmd_mgr->drain  = &cmd_mgr_drain;
    cmd_mgr->llind  = &cmd_mgr_llind;
    cmd_mgr->msgind = &cmd_mgr_msgind;
}

void bl_cmd_mgr_deinit(struct bl_cmd_mgr *cmd_mgr)
{
    cmd_mgr->print(cmd_mgr);
    cmd_mgr->drain(cmd_mgr);
    cmd_mgr->print(cmd_mgr);
    memset(cmd_mgr, 0, sizeof(*cmd_mgr));
}
