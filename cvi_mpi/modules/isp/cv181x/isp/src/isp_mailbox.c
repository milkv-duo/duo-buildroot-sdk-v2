/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_mailbox.c
 * Description:
 *
 */
#include "stdio.h"
#include "stdlib.h"
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "isp_mailbox.h"
#include "isp_debug.h"
#include "isp_defines.h"

#include "cvi_sys.h"

#include "rtos_isp_cmd.h"
#include "rtos_cmdqu.h"
#include "isp_mgr_buf.h"

#define CMDQ_DEV_NAME "/dev/cvi-rtos-cmdqu"

static CVI_U8 init_cnt;
static CVI_S32 mailbox_fd;

CVI_S32 isp_mailbox_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");

	CVI_S32 ret = CVI_SUCCESS;

	init_cnt++;

	if (mailbox_fd > 0) {
		return ret;
	}

	mailbox_fd = open(CMDQ_DEV_NAME, O_RDWR);

	if (mailbox_fd < 0) {
		ISP_LOG_ERR("ViPipe %d open %s failed\n", ViPipe, CMDQ_DEV_NAME);
		ret = CVI_FAILURE;
	}

	return ret;
}

CVI_S32 isp_mailbox_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");

	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	init_cnt--;

	if (init_cnt == 0) {
		close(mailbox_fd);
		mailbox_fd = -1;
	}

	return ret;
}

//static CVI_S32 isp_mailbox_send(VI_PIPE ViPipe, CVI_U8 cmd_id, CVI_U64 param_ptr)
//{
//	ISP_LOG_DEBUG("+\n");
//
//	CVI_S32 ret = CVI_SUCCESS;
//	struct cmdqu_t cmd = {0};
//
//	UNUSED(ViPipe);
//
//	cmd.ip_id = IP_ISP;
//	cmd.cmd_id = cmd_id;
//	cmd.block = 1;
//	cmd.resv.mstime = 0;
//	cmd.param_ptr = (CVI_U32) param_ptr;
//
//	if (ioctl(mailbox_fd, RTOS_CMDQU_SEND, &cmd) < 0) {
//		ISP_LOG_ERR("cmd(%d) param(%p) failed\n", cmd_id, ISP_PTR_CAST_U8(param_ptr));
//		ret = CVI_FAILURE;
//	}
//
//	return ret;
//}

static CVI_S32 isp_mailbox_send_wait(VI_PIPE ViPipe, CVI_U8 cmd_id, CVI_U64 param_ptr, CVI_U16 timeout_ms)
{
	ISP_LOG_DEBUG("+\n");

	CVI_S32 ret = CVI_SUCCESS;
	struct cmdqu_t cmd = {0};

	UNUSED(ViPipe);

	cmd.ip_id = IP_ISP;
	cmd.cmd_id = cmd_id;
	cmd.block = 1;
	cmd.resv.mstime = timeout_ms;
	cmd.param_ptr = (CVI_U32) param_ptr;

	if (ioctl(mailbox_fd, RTOS_CMDQU_SEND_WAIT, &cmd) < 0) {
		ISP_LOG_ERR("cmd(%d) param(%p) failed\n", cmd_id, ISP_PTR_CAST_U8(param_ptr));
		ret = CVI_FAILURE;
	}

	return ret;
}

CVI_S32 isp_mailbox_send_cmd_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");

	CVI_S32 ret = CVI_SUCCESS;
	CVI_U64 paddr = 0;
	CVI_U32 magic = 0;
	struct rtos_isp_cmd_event *ev;

	ret = isp_mgr_buf_get_event_addr(ViPipe, &paddr, (CVI_VOID *) &ev);

	ev->ViPipe = ViPipe;
	magic = ev->frame_idx;

	isp_mgr_buf_flush_cache(ViPipe);
	ret = isp_mailbox_send_wait(ViPipe, RTOS_ISP_CMD_INIT, paddr, 50);

	if (ret != CVI_SUCCESS) {
		ISP_LOG_ERR("ViPipe(%d), send RTOS_ISP_CMD_INIT timeout.\n", ViPipe);
	}

	isp_mgr_buf_invalid_cache(ViPipe);

	if (magic != ev->frame_idx) {
		printf("\n[ERROR] isp init 906l check magic(%d vs %d) fail!\n\n", magic, ev->frame_idx);
	}

	return ret;
}

CVI_S32 isp_mailbox_send_cmd_deinit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");

	CVI_S32 ret = CVI_SUCCESS;
	CVI_U64 paddr = 0;
	struct rtos_isp_cmd_event *ev;

	ret = isp_mgr_buf_get_event_addr(ViPipe, &paddr, (CVI_VOID *) &ev);

	ev->ViPipe = ViPipe;

	CVI_SYS_IonFlushCache(paddr, (CVI_VOID *) ev, sizeof(struct rtos_isp_cmd_event));
	ret = isp_mailbox_send_wait(ViPipe, RTOS_ISP_CMD_DEINIT, paddr, 50);

	if (ret != CVI_SUCCESS) {
		ISP_LOG_ERR("ViPipe(%d), send RTOS_ISP_CMD_DEINIT timeout.\n", ViPipe);
	}

	return ret;
}

CVI_S32 isp_mailbox_send_cmd_event(VI_PIPE ViPipe, enum rtos_isp_cmd event, CVI_U32 frame_idx)
{
	ISP_LOG_DEBUG("+\n");

	CVI_S32 ret = CVI_SUCCESS;
	CVI_U64 paddr = 0;
	struct rtos_isp_cmd_event *ev;

	ret = isp_mgr_buf_get_event_addr(ViPipe, &paddr, (CVI_VOID *) &ev);

	ev->ViPipe = ViPipe;
	ev->frame_idx = frame_idx;
	ev->is_slave_done = CVI_FALSE;

	isp_mgr_buf_flush_cache(ViPipe);
	ret = isp_mailbox_send_wait(ViPipe, event, paddr, 50);

	if (ret != CVI_SUCCESS) {
		ISP_LOG_ERR("ViPipe(%d), send event: %d timeout.\n", ViPipe, event);
	}

	isp_mgr_buf_invalid_cache(ViPipe);

	return ret;
}

//CVI_S32 isp_mailbox_send_cmd_sync_post_done(VI_PIPE ViPipe, CVI_U32 frame_idx)
//{
//	ISP_LOG_DEBUG("+\n");
//
//	CVI_S32 ret = CVI_SUCCESS;
//	CVI_U64 paddr = 0;
//	struct rtos_isp_cmd_event *ev;
//
//	ret = isp_mgr_buf_get_event_addr(ViPipe, &paddr, (CVI_VOID *) &ev);
//
//	ev->ViPipe = ViPipe;
//
//	CVI_SYS_IonFlushCache(paddr, (CVI_VOID *) ev, sizeof(struct rtos_isp_cmd_event));
//	ret = isp_mailbox_send_wait(ViPipe, RTOS_ISP_CMD_POST_DONE_SYNC, paddr, 65535);
//
//	if (ret != CVI_SUCCESS) {
//		ISP_LOG_ERR("ViPipe(%d), send RTOS_ISP_CMD_POST_DONE_SYNC timeout.\n", ViPipe);
//	}
//
//	CVI_SYS_IonInvalidateCache(paddr, (CVI_VOID *) ev, sizeof(struct rtos_isp_cmd_event));
//
//	if (ev->frame_idx == frame_idx && ev->is_slave_done) {
//		ret = CVI_SUCCESS;
//	} else {
//		ret = CVI_FAILURE;
//	}
//
//	return ret;
//}

