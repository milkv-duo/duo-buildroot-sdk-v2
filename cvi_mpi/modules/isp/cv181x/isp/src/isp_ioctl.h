/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_ioctl.h
 * Description:
 *
 */

#ifndef _ISP_IOCTL_H_
#define _ISP_IOCTL_H_

#ifndef ARCH_RTOS_CV181X
#include <sys/ioctl.h>
#endif
// #include <sys/time.h>

// #include <linux/vi_uapi.h>
// #include <linux/vi_isp.h>

#include "isp_defines.h"
#include "isp_main_local.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct isp_ioctl_param {
	VI_PIPE ViPipe;
	CVI_U32 id;
	CVI_VOID *ptr;
	CVI_U32 ptr_size;
	CVI_U32 val;

	CVI_U32 *out_val;
};

#ifndef ARCH_RTOS_CV181X
#define S_EXT_CTRLS_PTR(_id, _ptr)\
	do {\
		ISP_CTX_S *pstIspCtx = NULL;\
		ISP_GET_CTX(ViPipe, pstIspCtx);\
		struct vi_ext_control ec1;\
		memset(&ec1, 0, sizeof(ec1));\
		ec1.id = _id;\
		ec1.ptr = (void *)_ptr;\
		if (ioctl(pstIspCtx->ispDevFd, VI_IOC_S_CTRL, &ec1) < 0) {\
			ISP_IOCTL_ERR(pstIspCtx->ispDevFd, ec1);\
			return -1;\
		} \
	} while (0)
#else
#define S_EXT_CTRLS_PTR(_id, _ptr)
#endif

#ifndef ARCH_RTOS_CV181X
#define S_EXT_CTRLS_VALUE(_id, in, out)\
	do {\
		ISP_CTX_S *pstIspCtx = NULL;\
		ISP_GET_CTX(ViPipe, pstIspCtx);\
		struct vi_ext_control ec1;\
		memset(&ec1, 0, sizeof(ec1));\
		CVI_U32 val = in;\
		CVI_U32 *ptr = out;\
		ec1.id = _id;\
		ec1.value = val;\
		if (ioctl(pstIspCtx->ispDevFd, VI_IOC_S_CTRL, &ec1) < 0) {\
			ISP_IOCTL_ERR(pstIspCtx->ispDevFd, ec1);\
			return -1;\
		} \
		if (ptr != CVI_NULL)\
			*ptr = ec1.value;\
	} while (0)
#else
#define S_EXT_CTRLS_VALUE(_id, in, out)
#endif

#ifndef ARCH_RTOS_CV181X
#define G_EXT_CTRLS_PTR(_id, _ptr)\
	do {\
		ISP_CTX_S *pstIspCtx = NULL;\
		ISP_GET_CTX(ViPipe, pstIspCtx);\
		struct vi_ext_control ec1;\
		memset(&ec1, 0, sizeof(ec1));\
		ec1.id = _id;\
		ec1.ptr = (void *)_ptr;\
		if (ioctl(pstIspCtx->ispDevFd, VI_IOC_G_CTRL, &ec1) < 0) {\
			ISP_IOCTL_ERR(pstIspCtx->ispDevFd, ec1);\
			return -1;\
		} \
	} while (0)
#else
#define G_EXT_CTRLS_PTR(_id, _ptr)
#endif

#ifndef ARCH_RTOS_CV181X
#define G_EXT_CTRLS_VALUE(_id, in, out)\
	do {\
		ISP_CTX_S *pstIspCtx = NULL;\
		ISP_GET_CTX(ViPipe, pstIspCtx);\
		struct vi_ext_control ec1;\
		memset(&ec1, 0, sizeof(ec1));\
		CVI_U32 val = in;\
		CVI_U32 *ptr = out;\
		ec1.id = _id;\
		ec1.value = val;\
		ec1.sdk_cfg.pipe = *out;\
		if (ioctl(pstIspCtx->ispDevFd, VI_IOC_G_CTRL, &ec1) < 0) {\
			ISP_IOCTL_ERR(pstIspCtx->ispDevFd, ec1);\
			return -1;\
		} \
		if (ptr != CVI_NULL)\
			*ptr = ec1.value;\
	} while (0)
#else
#define G_EXT_CTRLS_VALUE(_id, in, out)
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_IOCTL_H_
