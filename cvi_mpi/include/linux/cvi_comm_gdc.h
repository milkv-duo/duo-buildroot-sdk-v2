/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: include/cvi_comm_gdc.h
 * Description:
 *   Common gdc definitions.
 */

#ifndef __CVI_COMM_GDC_H__
#define __CVI_COMM_GDC_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#include <linux/cvi_type.h>
#include <linux/cvi_common.h>
#include <linux/cvi_comm_video.h>


#ifdef __arm__
typedef CVI_S32 GDC_HANDLE; /* gdc handle*/
#else
typedef CVI_S64 GDC_HANDLE; /* gdc handle*/
#endif

/*
 * stImgIn: Input picture
 * stImgOut: Output picture
 * au64privateData[4]: RW; Private data of task
 * reserved: RW; Debug information,state of current picture
 */
typedef struct _GDC_TASK_ATTR_S {
	VIDEO_FRAME_INFO_S stImgIn;
	VIDEO_FRAME_INFO_S stImgOut;
	CVI_U64 au64privateData[4];
	CVI_U64 reserved;
} GDC_TASK_ATTR_S;

/* Buffer Wrap
 *
 * bEnable: Whether bufwrap is enabled.
 * u32BufLine: buffer line number.
 *             Support 64, 128, 192, 256.
 * u32WrapBufferSize: buffer size.
 */
typedef struct _DWA_BUF_WRAP_S {
	CVI_BOOL bEnable;
	CVI_U32 u32BufLine;
	CVI_U32 u32WrapBufferSize;
} DWA_BUF_WRAP_S;

/* Vi gdc attribute
 *
 * chn: vi chnnel
 */
typedef struct _VI_MESH_ATTR_S {
	VI_CHN chn;
} VI_MESH_ATTR_S;

/* Vpss gdc attribute
 *
 * grp: vpss group
 * chn: vpss chnnel
 */
typedef struct _VPSS_MESH_ATTR_S {
	VPSS_GRP grp;
	VPSS_CHN chn;
} VPSS_MESH_ATTR_S;

/* Mesh dump attribute
 *
 * binFileName: mesh file name
 * enModId: module id
 * viMeshAttr: vi gdc attribute
 * vpssMeshAttr:  gdc attribute
 */
typedef struct _MESH_DUMP_ATTR_S {
	CVI_CHAR binFileName[128];
	MOD_ID_E enModId;
	union {
		VI_MESH_ATTR_S viMeshAttr;
		VPSS_MESH_ATTR_S vpssMeshAttr;
	};
} MESH_DUMP_ATTR_S;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __CVI_COMM_GDC_H__ */
