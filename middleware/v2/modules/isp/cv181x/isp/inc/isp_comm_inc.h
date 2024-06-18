/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: include/isp_comm_inc.h
 * Description:
 */

#ifndef __ISP_COMM_INC_H__
#define __ISP_COMM_INC_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include "stddef.h"
#include "stdint.h"
#include <sys/time.h>

#if defined(ARCH_CV183X) || defined(ARCH_CV182X)
#include <cvi_common.h>
#include <cvi_comm_video.h>
#include <cvi_defines.h>
#include <cvi_math.h>
#include <linux/cvi_vip_isp.h>
#include <linux/cvi_vip_tun_cfg.h>
#include <linux/cvi_vip.h>
#include <linux/cvi_vip_snsr.h>
#elif defined(__CV181X__) || defined(__CV180X__)
#include <linux/cvi_common.h>
#include <linux/cvi_comm_vi.h>
#include <linux/cvi_comm_video.h>
#include <linux/cvi_defines.h>
#include <linux/cvi_math.h>
#include <linux/vi_isp.h>
#include <linux/vi_tun_cfg.h>
#include <linux/vi_uapi.h>
#include <linux/vi_snsr.h>
#elif defined(ARCH_RTOS_CV181X)
#include <cvi_common.h>
#include <cvi_comm_vi.h>
#include <cvi_comm_video.h>
#include <cvi_defines.h>
#include <cvi_math.h>
#include <vi_isp.h>
#include <vi_tun_cfg.h>
#include <vi_uapi.h>
#include <vi_snsr.h>

#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* __CVI_COMM_INC_H__ */
