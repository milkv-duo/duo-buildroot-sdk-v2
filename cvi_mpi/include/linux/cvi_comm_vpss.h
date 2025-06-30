/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: include/cvi_comm_vpss.h
 * Description:
 *   The common data type defination for VPSS module.
 */

#ifndef __CVI_COMM_VPSS_H__
#define __CVI_COMM_VPSS_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#include <linux/cvi_type.h>
#include <linux/cvi_common.h>
#include <linux/cvi_comm_video.h>
#include <linux/cvi_comm_vpss.h>

#define VPSS_INVALID_FRMRATE -1
#define VPSS_CHN0             0
#define VPSS_CHN1             1
#define VPSS_CHN2             2
#define VPSS_CHN3             3
#define VPSS_INVALID_CHN     -1
#define VPSS_INVALID_GRP     -1
#define CVI_STITCH_CHN_MAX_NUM 4

/*
 * VPSS_CROP_RATIO_COOR: Ratio coordinate, not supported currently.
 * VPSS_CROP_ABS_COOR: Absolute coordinate.
 */
typedef enum _VPSS_CROP_COORDINATE_E {
	VPSS_CROP_RATIO_COOR = 0,
	VPSS_CROP_ABS_COOR,
} VPSS_CROP_COORDINATE_E;

/**
 * VPSS_ROUNDING_TO_EVEN: Round off, refer to the table below.
 * VPSS_ROUNDING_AWAY_FROM_ZERO: Round off, refer to the table below.
 * VPSS_ROUNDING_TRUNCATE: Unconditional rounding, see table below.
*/
typedef enum _VPSS_ROUNDING_E {
	VPSS_ROUNDING_TO_EVEN = 0,
	VPSS_ROUNDING_AWAY_FROM_ZERO,
	VPSS_ROUNDING_TRUNCATE,
	VPSS_ROUNDING_MAX,
} VPSS_ROUNDING_E;

/*
 * bEnable: Whether Normalize is enabled.
 * factor: scaling factors for 3 planes.
 * mean: minus means for 3 planes.
 * rounding: the pattern of rounding mode during Normalize.
 */
typedef struct _VPSS_NORMALIZE_S {
	CVI_BOOL bEnable;
	CVI_FLOAT factor[3];
	CVI_FLOAT mean[3];
	VPSS_ROUNDING_E rounding;
} VPSS_NORMALIZE_S;

/*
 * u32MaxW: Range: Width of source image.
 * u32MaxH: Range: Height of source image.
 * enPixelFormat: Pixel format of target image.
 * stFrameRate: Frame rate control info.
 * u8VpssDev: Only meaningful if VPSS_MODE_DUAL.
 */
typedef struct _VPSS_GRP_ATTR_S {
	CVI_U32 u32MaxW;
	CVI_U32 u32MaxH;
	PIXEL_FORMAT_E enPixelFormat;
	FRAME_RATE_CTRL_S stFrameRate;
	CVI_U8 u8VpssDev;
} VPSS_GRP_ATTR_S;

/*
 * u32Width: Width of target image.
 * u32Height: Height of target image.
 * enVideoFormat: Video format of target image.
 * enPixelFormat: Pixel format of target image.
 * stFrameRate: Frame rate control info.
 * bMirror: Mirror enable.
 * bFlip: Flip enable.
 * u32Depth: User get list depth.
 * stAspectRatio: Aspect Ratio info.
 */
typedef struct _VPSS_CHN_ATTR_S {
	CVI_U32 u32Width;
	CVI_U32 u32Height;
	VIDEO_FORMAT_E enVideoFormat;
	PIXEL_FORMAT_E enPixelFormat;
	FRAME_RATE_CTRL_S stFrameRate;
	CVI_BOOL bMirror;
	CVI_BOOL bFlip;
	CVI_U32 u32Depth;
	ASPECT_RATIO_S stAspectRatio;
	VPSS_NORMALIZE_S stNormalize;
} VPSS_CHN_ATTR_S;

/*
 * bEnable: RW; CROP enable.
 * enCropCoordinate: RW; Coordinate mode of the crop start point.
 * stCropRect: CROP rectangle.
 */
typedef struct _VPSS_CROP_INFO_S {
	CVI_BOOL bEnable;
	VPSS_CROP_COORDINATE_E enCropCoordinate;
	RECT_S stCropRect;
} VPSS_CROP_INFO_S;

/*
 * bEnable: Whether LDC is enbale.
 * stAttr: LDC Attribute.
 */
// ++++++++ If you want to change these interfaces, please contact the isp team. ++++++++
typedef struct _VPSS_LDC_ATTR_S {
	CVI_BOOL bEnable;
	LDC_ATTR_S stAttr;
} VPSS_LDC_ATTR_S;
// -------- If you want to change these interfaces, please contact the isp team. --------

/**
 * u32VpssVbSource: Video memory block pool type.
 * u32VpssSplitNodeNum: Number of block nodes.
*/
typedef struct _VPSS_PARAM_MOD_S {
	CVI_U32 u32VpssVbSource;
	CVI_U32 u32VpssSplitNodeNum;
} VPSS_MOD_PARAM_S;

/**
 * VPSS_SCALE_COEF_BICUBIC: bicubic algorithm.
 * VPSS_SCALE_COEF_BILINEAR: bilinear algorithm.
 * VPSS_SCALE_COEF_NEAREST: nearest algorithm.
 * VPSS_SCALE_COEF_DOWNSCALE_SMOOTH: downscale algorithm.
 * VPSS_SCALE_COEF_OPENCV_BILINEAR: opencv bilinear algorithm.
*/
typedef enum _VPSS_SCALE_COEF_E {
	VPSS_SCALE_COEF_BICUBIC = 0,
	VPSS_SCALE_COEF_BILINEAR,
	VPSS_SCALE_COEF_NEAREST,
	VPSS_SCALE_COEF_DOWNSCALE_SMOOTH,
	VPSS_SCALE_COEF_OPENCV_BILINEAR,
	VPSS_SCALE_COEF_MAX,
} VPSS_SCALE_COEF_E;

/**
 * bEnable: switch for enable chn buffer wrap.
 * u32BufLine: wrap buffer row height.
 * u32WrapBufferSize: wrap buffer size.
*/
typedef struct _VPSS_CHN_BUF_WRAP_S {
	CVI_BOOL bEnable;
	CVI_U32 u32BufLine;	// 64, 128
	CVI_U32 u32WrapBufferSize;	// depth for now
} VPSS_CHN_BUF_WRAP_S;

/*vpss stitch src info*/
typedef struct _CVI_STITCH_SRC_S {
	VPSS_GRP VpssGrp;
	VPSS_CHN VpssChn;
} CVI_STITCH_SRC_S;

/**
 * stStitchSrc: vpss stitch src info
 * stDstRect: vpss stitch dest position
 * u8Priority: Priority
 */
typedef struct _CVI_STITCH_CHN_S {
	CVI_STITCH_SRC_S stStitchSrc;
	RECT_S stDstRect;
	CVI_U8 u8Priority;
} CVI_STITCH_CHN_S;

/**
 * u8ChnNum: the number of vpss stitch chn
 * VoChn: Vo chn id
 * s32OutFps: Output FPS
 * enOutPixelFormat: image pixel format
 * stOutSize: Output size
 * hVbPool: Attached vb pool
 * astStitchChn: vpss stitch chn attr
 */
typedef struct _CVI_STITCH_ATTR_S {
	CVI_U8 u8ChnNum;
	CVI_U8 VoChn;
	CVI_S32 s32OutFps;
	PIXEL_FORMAT_E enOutPixelFormat;
	SIZE_S stOutSize;
	CVI_U32 hVbPool;
	CVI_STITCH_CHN_S astStitchChn[CVI_STITCH_CHN_MAX_NUM];
} CVI_STITCH_ATTR_S;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
#endif /* __CVI_COMM_VPSS_H__ */
