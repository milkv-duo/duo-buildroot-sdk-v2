#pragma once
#include <cvi_buffer.h>
#include <cvi_vb.h>
#include <cvi_vpss.h>
#include "cvi_comm.h"
#ifdef CONFIG_ALIOS
#include "cvi_tdl_log.hpp"
#endif
#include <inttypes.h>
#include <math.h>
#include <string.h>
#include <syslog.h>

/**
 * \addtogroup core_vpss Vpss Helper Functions
 * \ingroup core_cvitdlcore
 */

/** @def MMF_INIT_HELPER_BLKCNT_DEFAULT
 * @ingroup core_vpss
 * @brief The default number of image MMF_INIT_HELPER will open for 2 mem pool.
 */
#define MMF_INIT_HELPER_BLKCNT_DEFAULT 12

/** @def SCALAR_4096_ALIGN_BUG
 * @ingroup core_vpss
 * @brief HW image channel alignment.
 */
#define SCALAR_4096_ALIGN_BUG 0x1000
#define max(a, b)           \
  ({                        \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a > _b ? _a : _b;      \
  })

#define min(a, b)           \
  ({                        \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a < _b ? _a : _b;      \
  })

/**
 * @brief Middleware init helper function.
 * @ingroup core_vpss
 *
 * @param stVbConf The middleware config structure.
 * @return int Return CVI_SUCCESS on success.
 */
static inline int __attribute__((always_inline)) MMF_INIT(const VB_CONFIG_S *stVbConf) {
  CVI_S32 s32Ret = CVI_FAILURE;

  CVI_SYS_Exit();
  CVI_VB_Exit();

  s32Ret = CVI_VB_SetConfig(stVbConf);
  if (s32Ret != CVI_SUCCESS) {
    syslog(LOG_ERR, "CVI_VB_SetConf failed!\n");
    return s32Ret;
  }
  s32Ret = CVI_VB_Init();
  if (s32Ret != CVI_SUCCESS) {
    syslog(LOG_ERR, "CVI_VB_Init failed!\n");
    return s32Ret;
  }
  s32Ret = CVI_SYS_Init();
  if (s32Ret != CVI_SUCCESS) {
    syslog(LOG_ERR, "CVI_SYS_Init failed!\n");
    CVI_VB_Exit();
    return s32Ret;
  }

  return s32Ret;
}

/**
 * @brief A helper function to open one input and one output memory pool for developers.
 * @ingroup core_vpss
 *
 * @param enSrcWidth Desired input image width.
 * @param enSrcHeight Desired input image height.
 * @param enSrcFormat Desired input image format.
 * @param inBlkCount Max available images in the input pool.
 * @param enDstWidth Desired output image width.
 * @param enDstHeight Desired output image height.
 * @param enDstFormat Desired output image format.
 * @param outBlkCount Max available images in the output pool.
 * @return int Return CVI_SUCCESS on success.
 */
static inline int __attribute__((always_inline))
MMF_INIT_HELPER2(uint32_t enSrcWidth, uint32_t enSrcHeight, PIXEL_FORMAT_E enSrcFormat,
                 const uint32_t inBlkCount, uint32_t enDstWidth, uint32_t enDstHeight,
                 PIXEL_FORMAT_E enDstFormat, const uint32_t outBlkCount) {
  COMPRESS_MODE_E enCompressMode = COMPRESS_MODE_NONE;
  // Init SYS and Common VB,
  // Running w/ Vi don't need to do it again. Running Vpss along need init below
  // FIXME: Can only be init once in one pipeline
  VB_CONFIG_S stVbConf;
  memset(&stVbConf, 0, sizeof(VB_CONFIG_S));
  stVbConf.u32MaxPoolCnt = 2;
  CVI_U32 u32BlkSize;
  u32BlkSize = COMMON_GetPicBufferSize(enSrcWidth, enSrcHeight, enSrcFormat, DATA_BITWIDTH_8,
                                       enCompressMode, DEFAULT_ALIGN);
  stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
  stVbConf.astCommPool[0].u32BlkCnt = inBlkCount;
  u32BlkSize = COMMON_GetPicBufferSize(enDstWidth, enDstHeight, enDstFormat, DATA_BITWIDTH_8,
                                       enCompressMode, DEFAULT_ALIGN);
  stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
  stVbConf.astCommPool[1].u32BlkCnt = outBlkCount;

  return MMF_INIT(&stVbConf);
}

/**
 * @brief A helper function to open one input and one output memory pool with default image number
 * for developers.
 * @ingroup core_vpss
 *
 * @param enSrcWidth Desired input image width.
 * @param enSrcHeight Desired input image height.
 * @param enSrcFormat Desired input image format.
 * @param enDstWidth Desired output image width.
 * @param enDstHeight Desired output image height.
 * @param enDstFormat Desired output image format..
 * @return int Return CVI_SUCCESS on success.
 */
static inline int __attribute__((always_inline))
MMF_INIT_HELPER(uint32_t enSrcWidth, uint32_t enSrcHeight, PIXEL_FORMAT_E enSrcFormat,
                uint32_t enDstWidth, uint32_t enDstHeight, PIXEL_FORMAT_E enDstFormat) {
  return MMF_INIT_HELPER2(enSrcWidth, enSrcHeight, enSrcFormat, MMF_INIT_HELPER_BLKCNT_DEFAULT,
                          enDstWidth, enDstHeight, enDstFormat, MMF_INIT_HELPER_BLKCNT_DEFAULT);
}

/**
 * @brief A helper function to get default VPSS_GRP_ATTR_S value with given image information.
 * @ingroup core_vpss
 *
 * @param srcWidth Input image width.
 * @param srcHeight Input image height.
 * @param enSrcFormat Input image format.
 */
inline void __attribute__((always_inline))
VPSS_GRP_DEFAULT_HELPER2(VPSS_GRP_ATTR_S *pstVpssGrpAttr, CVI_U32 srcWidth, CVI_U32 srcHeight,
                         PIXEL_FORMAT_E enSrcFormat, CVI_U8 dev) {
  memset(pstVpssGrpAttr, 0, sizeof(VPSS_GRP_ATTR_S));
  pstVpssGrpAttr->stFrameRate.s32SrcFrameRate = -1;
  pstVpssGrpAttr->stFrameRate.s32DstFrameRate = -1;
  pstVpssGrpAttr->enPixelFormat = enSrcFormat;
  pstVpssGrpAttr->u32MaxW = srcWidth;
  pstVpssGrpAttr->u32MaxH = srcHeight;
#ifndef CV186X
  pstVpssGrpAttr->u8VpssDev = dev;
#endif
}

/**
 * @brief A helper function to get default VPSS_GRP_ATTR_S value with given image information.
 * @ingroup core_vpss
 *
 * @param srcWidth Input image width.
 * @param srcHeight Input image height.
 * @param enSrcFormat Input image format.
 */
inline void __attribute__((always_inline))
VPSS_GRP_DEFAULT_HELPER(VPSS_GRP_ATTR_S *pstVpssGrpAttr, CVI_U32 srcWidth, CVI_U32 srcHeight,
                        PIXEL_FORMAT_E enSrcFormat) {
  VPSS_GRP_DEFAULT_HELPER2(pstVpssGrpAttr, srcWidth, srcHeight, enSrcFormat, 0);
}

/**
 * @brief A helper function to get default VPSS_CHN_ATTR_S value with given image information.
 * @ingroup core_vpss
 *
 * @param dstWidth Output image width.
 * @param dstHeight Output image height.
 * @param enDstFormat Output image format.
 * @param keepAspectRatio Keep aspect ratio or not.
 */
inline void __attribute__((always_inline))
VPSS_CHN_DEFAULT_HELPER(VPSS_CHN_ATTR_S *pastVpssChnAttr, CVI_U32 dstWidth, CVI_U32 dstHeight,
                        PIXEL_FORMAT_E enDstFormat, CVI_BOOL keepAspectRatio) {
  pastVpssChnAttr->u32Width = dstWidth;
  pastVpssChnAttr->u32Height = dstHeight;
  pastVpssChnAttr->enVideoFormat = VIDEO_FORMAT_LINEAR;
  pastVpssChnAttr->enPixelFormat = enDstFormat;

  pastVpssChnAttr->stFrameRate.s32SrcFrameRate = -1;
  pastVpssChnAttr->stFrameRate.s32DstFrameRate = -1;
  pastVpssChnAttr->u32Depth = 1;
  pastVpssChnAttr->bMirror = CVI_FALSE;
  pastVpssChnAttr->bFlip = CVI_FALSE;
  if (keepAspectRatio) {
    pastVpssChnAttr->stAspectRatio.enMode = ASPECT_RATIO_AUTO;
    pastVpssChnAttr->stAspectRatio.u32BgColor = RGB_8BIT(0, 0, 0);
  } else {
    pastVpssChnAttr->stAspectRatio.enMode = ASPECT_RATIO_NONE;
  }
  pastVpssChnAttr->stNormalize.bEnable = CVI_FALSE;
  pastVpssChnAttr->stNormalize.factor[0] = 0;
  pastVpssChnAttr->stNormalize.factor[1] = 0;
  pastVpssChnAttr->stNormalize.factor[2] = 0;
  pastVpssChnAttr->stNormalize.mean[0] = 0;
  pastVpssChnAttr->stNormalize.mean[1] = 0;
  pastVpssChnAttr->stNormalize.mean[2] = 0;
  pastVpssChnAttr->stNormalize.rounding = VPSS_ROUNDING_TO_EVEN;
}

/**
 * @brief A helper function with manual ratio control to get default VPSS_CHN_ATTR_S value with
 * given image information.
 * @ingroup core_vpss
 *
 * @param dstWidth Output image width.
 * @param dstHeight Output image height.
 * @param enDstFormat Output image format.
 * @param ratioX The upper-left x coordinate.
 * @param ratioY The upper-left y coordinate.
 * @param ratioWidth The desired width of the image from group. Black will be filled if (ratioX +
 * ratioWidth) < dstWidth.
 * @param ratioHeight The desired height of the image grom group. Black will be filled if (ratioY +
 * ratioWidth) < dstHeight.
 */
inline void __attribute__((always_inline))
VPSS_CHN_RATIO_MANUAL_HELPER(VPSS_CHN_ATTR_S *pastVpssChnAttr, CVI_U32 dstWidth, CVI_U32 dstHeight,
                             PIXEL_FORMAT_E enDstFormat, CVI_U32 ratioX, CVI_U32 ratioY,
                             CVI_U32 ratioWidth, CVI_U32 ratioHeight) {
  pastVpssChnAttr->u32Width = dstWidth;
  pastVpssChnAttr->u32Height = dstHeight;
  pastVpssChnAttr->enVideoFormat = VIDEO_FORMAT_LINEAR;
  pastVpssChnAttr->enPixelFormat = enDstFormat;

  pastVpssChnAttr->stFrameRate.s32SrcFrameRate = -1;
  pastVpssChnAttr->stFrameRate.s32DstFrameRate = -1;
  pastVpssChnAttr->u32Depth = 1;
  pastVpssChnAttr->bMirror = CVI_FALSE;
  pastVpssChnAttr->bFlip = CVI_FALSE;
  pastVpssChnAttr->stAspectRatio.enMode = ASPECT_RATIO_MANUAL;
  pastVpssChnAttr->stAspectRatio.stVideoRect.s32Y = ratioX;
  pastVpssChnAttr->stAspectRatio.stVideoRect.s32X = ratioY;
  pastVpssChnAttr->stAspectRatio.stVideoRect.u32Width = ratioWidth;
  pastVpssChnAttr->stAspectRatio.stVideoRect.u32Height = ratioHeight;
  pastVpssChnAttr->stAspectRatio.u32BgColor = RGB_8BIT(0, 0, 0);
  pastVpssChnAttr->stNormalize.bEnable = CVI_FALSE;
  pastVpssChnAttr->stNormalize.factor[0] = 0;
  pastVpssChnAttr->stNormalize.factor[1] = 0;
  pastVpssChnAttr->stNormalize.factor[2] = 0;
  pastVpssChnAttr->stNormalize.mean[0] = 0;
  pastVpssChnAttr->stNormalize.mean[1] = 0;
  pastVpssChnAttr->stNormalize.mean[2] = 0;
  pastVpssChnAttr->stNormalize.rounding = VPSS_ROUNDING_TO_EVEN;
}

/**
 * @brief A helper function to help developers to create a quantization + scaling step for TDL
 * preprocessing using vpss hardware.
 * @ingroup core_vpss
 *
 * @param dstWidth Output image width.
 * @param dstHeight Output image height.
 * @param enDstFormat Output image format.
 * @param factor The quantized factor, size = 3.
 * @param mean The quantized mean, size = 3.
 * @param padReverse If your code do quantization + scale before padding, set this to true. Default
 * if false.
 */
inline void __attribute__((always_inline))
VPSS_CHN_SQ_HELPER(VPSS_CHN_ATTR_S *pastVpssChnAttr, const CVI_U32 dstWidth,
                   const CVI_U32 dstHeight, const PIXEL_FORMAT_E enDstFormat,
                   const CVI_FLOAT *factor, const CVI_FLOAT *mean, const bool padReverse) {
  pastVpssChnAttr->u32Width = dstWidth;
  pastVpssChnAttr->u32Height = dstHeight;
  pastVpssChnAttr->enVideoFormat = VIDEO_FORMAT_LINEAR;
  pastVpssChnAttr->enPixelFormat = enDstFormat;
  pastVpssChnAttr->stFrameRate.s32SrcFrameRate = -1;
  pastVpssChnAttr->stFrameRate.s32DstFrameRate = -1;
  pastVpssChnAttr->u32Depth = 1;
  pastVpssChnAttr->bMirror = CVI_FALSE;
  pastVpssChnAttr->bFlip = CVI_FALSE;
  pastVpssChnAttr->stAspectRatio.enMode = ASPECT_RATIO_AUTO;
  pastVpssChnAttr->stAspectRatio.bEnableBgColor = CVI_TRUE;
  if (padReverse) {
    pastVpssChnAttr->stAspectRatio.u32BgColor = RGB_8BIT(
        (int)(mean[0] / factor[0]), (int)(mean[1] / factor[1]), (int)(mean[2] / factor[2]));
  } else {
    pastVpssChnAttr->stAspectRatio.u32BgColor = RGB_8BIT(0, 0, 0);
  }
  pastVpssChnAttr->stNormalize.bEnable = CVI_TRUE;
  for (uint32_t i = 0; i < 3; i++) {
    pastVpssChnAttr->stNormalize.factor[i] = factor[i];
  }
  for (uint32_t i = 0; i < 3; i++) {
    pastVpssChnAttr->stNormalize.mean[i] = mean[i];
  }
  pastVpssChnAttr->stNormalize.rounding = VPSS_ROUNDING_TO_EVEN;
}

/**
 * @brief A helper function to help developers to create a quantization + scaling step for TDL
 * preprocessing using vpss hardware, but only pad right and bottom instead of four directions.
 * @ingroup core_vpss
 *
 * @param srcWidth Input image width.
 * @param srcHeight Intput image height.
 * @param dstWidth Output image width.
 * @param dstHeight Output image height.
 * @param enDstFormat Output image format.
 * @param factor The quantized factor, size = 3.
 * @param mean The quantized mean, size = 3.
 * @param padReverse If your code do quantization + scale before padding, set this to true. Default
 * if false.
 */
inline void __attribute__((always_inline))
VPSS_CHN_SQ_RB_HELPER(VPSS_CHN_ATTR_S *pastVpssChnAttr, const CVI_U32 srcWidth,
                      const CVI_U32 srcHeight, const CVI_U32 dstWidth, const CVI_U32 dstHeight,
                      const PIXEL_FORMAT_E enDstFormat, const CVI_FLOAT *factor,
                      const CVI_FLOAT *mean, const bool padReverse) {
  float ratio_w = (float)dstWidth / srcWidth;
  float ratio_h = (float)dstHeight / srcHeight;
  float ratio = min(ratio_w, ratio_h);

  pastVpssChnAttr->u32Width = dstWidth;
  pastVpssChnAttr->u32Height = dstHeight;
  pastVpssChnAttr->enVideoFormat = VIDEO_FORMAT_LINEAR;
  pastVpssChnAttr->enPixelFormat = enDstFormat;
  pastVpssChnAttr->stFrameRate.s32SrcFrameRate = -1;
  pastVpssChnAttr->stFrameRate.s32DstFrameRate = -1;
  pastVpssChnAttr->u32Depth = 1;
  pastVpssChnAttr->bMirror = CVI_FALSE;
  pastVpssChnAttr->bFlip = CVI_FALSE;
  pastVpssChnAttr->stAspectRatio.enMode = ASPECT_RATIO_MANUAL;
  pastVpssChnAttr->stAspectRatio.stVideoRect.s32X = 0;
  pastVpssChnAttr->stAspectRatio.stVideoRect.s32Y = 0;
  pastVpssChnAttr->stAspectRatio.stVideoRect.u32Width = (srcWidth * ratio) + 0.5;
  pastVpssChnAttr->stAspectRatio.stVideoRect.u32Height = (srcHeight * ratio) + 0.5;
  pastVpssChnAttr->stAspectRatio.bEnableBgColor = CVI_TRUE;

  if (padReverse) {
    pastVpssChnAttr->stAspectRatio.u32BgColor = RGB_8BIT(
        (int)(mean[0] / factor[0]), (int)(mean[1] / factor[1]), (int)(mean[2] / factor[2]));
  } else {
    pastVpssChnAttr->stAspectRatio.u32BgColor = RGB_8BIT(0, 0, 0);
  }
  pastVpssChnAttr->stNormalize.bEnable = CVI_TRUE;
  for (uint32_t i = 0; i < 3; i++) {
    pastVpssChnAttr->stNormalize.factor[i] = factor[i];
  }
  for (uint32_t i = 0; i < 3; i++) {
    pastVpssChnAttr->stNormalize.mean[i] = mean[i];
  }
  pastVpssChnAttr->stNormalize.rounding = VPSS_ROUNDING_TO_EVEN;
}

/***
 * pad_val,rgb order
 * pad_type ,0:pad none, 1:pad center,2:pad right bottom
 */
inline void __attribute__((always_inline))
VPSS_CHN_SQ_HELPER_X(VPSS_CHN_ATTR_S *pastVpssChnAttr, const CVI_U32 srcWidth,
                     const CVI_U32 srcHeight, const CVI_U32 dstWidth, const CVI_U32 dstHeight,
                     const PIXEL_FORMAT_E enDstFormat, const CVI_FLOAT *factor,
                     const CVI_FLOAT *mean, int *pad_val, int pad_type) {
  pastVpssChnAttr->u32Width = dstWidth;
  pastVpssChnAttr->u32Height = dstHeight;
  pastVpssChnAttr->enVideoFormat = VIDEO_FORMAT_LINEAR;
  pastVpssChnAttr->enPixelFormat = enDstFormat;
  pastVpssChnAttr->stFrameRate.s32SrcFrameRate = -1;
  pastVpssChnAttr->stFrameRate.s32DstFrameRate = -1;
  pastVpssChnAttr->u32Depth = 1;
  pastVpssChnAttr->bMirror = CVI_FALSE;
  pastVpssChnAttr->bFlip = CVI_FALSE;
  if (pad_type == 0) {
    pastVpssChnAttr->stAspectRatio.enMode = ASPECT_RATIO_NONE;
  } else if (pad_type == 1) {
    pastVpssChnAttr->stAspectRatio.enMode = ASPECT_RATIO_AUTO;
    pastVpssChnAttr->stAspectRatio.bEnableBgColor = CVI_TRUE;
  } else if (pad_type == 2) {
    float ratio_w = (float)dstWidth / srcWidth;
    float ratio_h = (float)dstHeight / srcHeight;
    float ratio = min(ratio_w, ratio_h);
    pastVpssChnAttr->stAspectRatio.enMode = ASPECT_RATIO_MANUAL;
    pastVpssChnAttr->stAspectRatio.stVideoRect.s32X = 0;
    pastVpssChnAttr->stAspectRatio.stVideoRect.s32Y = 0;
    pastVpssChnAttr->stAspectRatio.stVideoRect.u32Width = (srcWidth * ratio) + 0.5;
    pastVpssChnAttr->stAspectRatio.stVideoRect.u32Height = (srcHeight * ratio) + 0.5;
    pastVpssChnAttr->stAspectRatio.bEnableBgColor = CVI_TRUE;
  }
  if (pad_val != NULL) {
    pastVpssChnAttr->stAspectRatio.u32BgColor = RGB_8BIT(pad_val[0], pad_val[1], pad_val[2]);
  } else {
    pastVpssChnAttr->stAspectRatio.u32BgColor = RGB_8BIT(0, 0, 0);
  }

  if (factor != NULL) {
    pastVpssChnAttr->stNormalize.bEnable = CVI_TRUE;
    for (uint32_t i = 0; i < 3; i++) {
      pastVpssChnAttr->stNormalize.factor[i] = factor[i];
    }
    for (uint32_t i = 0; i < 3; i++) {
      pastVpssChnAttr->stNormalize.mean[i] = mean[i];
    }
    pastVpssChnAttr->stNormalize.rounding = VPSS_ROUNDING_TO_EVEN;
  } else {
    pastVpssChnAttr->stNormalize.bEnable = CVI_FALSE;
    pastVpssChnAttr->stNormalize.factor[0] = 0;
    pastVpssChnAttr->stNormalize.factor[1] = 0;
    pastVpssChnAttr->stNormalize.factor[2] = 0;
    pastVpssChnAttr->stNormalize.mean[0] = 0;
    pastVpssChnAttr->stNormalize.mean[1] = 0;
    pastVpssChnAttr->stNormalize.mean[2] = 0;
    pastVpssChnAttr->stNormalize.rounding = VPSS_ROUNDING_TO_EVEN;
  }
}

#undef max
#undef min

/**
 * @brief A helper function to init vpss hardware with given image information. This function DOES
 NOT set VPSS mode for you.

 * @param enSrcWidth Input image width.
 * @param enSrcHeight Intput image height.
 * @param enSrcFormat Input image format.
 * @param enDstWidth Output image width.
 * @param enDstHeight Output image height.
 * @param enDstFormat Output image format.
 * @param enabledChannel Number of enabled channels.
 * @param mode The mode of VPSS. Support single mode (1->4) and dual mode (1->3, 1->1)
 * @param keepAspectRatio Keep aspect ratio or not.
 */
inline int __attribute__((always_inline))
VPSS_INIT_HELPER2(CVI_U32 vpssGrpId, uint32_t enSrcWidth, uint32_t enSrcHeight,
                  PIXEL_FORMAT_E enSrcFormat, uint32_t enDstWidth, uint32_t enDstHeight,
                  PIXEL_FORMAT_E enDstFormat, uint32_t enabledChannel, bool keepAspectRatio) {
  syslog(LOG_INFO, "VPSS init with src (%u, %u) dst (%u, %u).\n", enSrcWidth, enSrcHeight,
         enDstWidth, enDstHeight);
  CVI_S32 s32Ret = CVI_FAILURE;

  VPSS_GRP_ATTR_S stVpssGrpAttr;
  VPSS_CHN_ATTR_S stVpssChnAttr;
  VPSS_GRP_DEFAULT_HELPER(&stVpssGrpAttr, enSrcWidth, enSrcHeight, enSrcFormat);
  VPSS_CHN_DEFAULT_HELPER(&stVpssChnAttr, enDstWidth, enDstHeight, enDstFormat, keepAspectRatio);

  /*start vpss*/
  s32Ret = CVI_VPSS_CreateGrp(vpssGrpId, &stVpssGrpAttr);
  if (s32Ret != CVI_SUCCESS) {
    printf("???? %d\n", vpssGrpId);
    syslog(LOG_ERR, "CVI_VPSS_CreateGrp(grp:%d) failed with %#x!\n", vpssGrpId, s32Ret);
    return s32Ret;
  }
  s32Ret = CVI_VPSS_ResetGrp(vpssGrpId);
  if (s32Ret != CVI_SUCCESS) {
    syslog(LOG_ERR, "CVI_VPSS_ResetGrp(grp:%d) failed with %#x!\n", vpssGrpId, s32Ret);
    return s32Ret;
  }
  if (enabledChannel > 3) {
    syslog(LOG_ERR, "Maximum value for enabledChannel is 3.");
  }
  for (uint32_t i = 0; i < enabledChannel; i++) {
    s32Ret = CVI_VPSS_SetChnAttr(vpssGrpId, i, &stVpssChnAttr);
    if (s32Ret != CVI_SUCCESS) {
      syslog(LOG_ERR, "CVI_VPSS_SetChnAttr failed with %#x\n", s32Ret);
      return s32Ret;
    }
    s32Ret = CVI_VPSS_EnableChn(vpssGrpId, i);
    if (s32Ret != CVI_SUCCESS) {
      syslog(LOG_ERR, "CVI_VPSS_EnableChn failed with %#x\n", s32Ret);
      return s32Ret;
    }
  }
  s32Ret = CVI_VPSS_StartGrp(vpssGrpId);
  if (s32Ret != CVI_SUCCESS) {
    syslog(LOG_ERR, "CVI_VPSS_StartGrp failed with %#x\n", s32Ret);
    return s32Ret;
  }

  return s32Ret;
}

/**
 * @brief A helper function to create a VIDEO_FRAME_INFO_S buffer from given image information.
 * @ingroup core_vpss
 *
 * @param blk The ion instance of VIDEO_FRAME_INFO_S.
 * @param vbFrame Output VIDEO_FRAME_INFO_S buffer.
 * @param srcWidth Desired image width.
 * @param srcHeight Desired image height.
 * @param pixelFormat Desired image format.
 */
inline int __attribute__((always_inline))
CREATE_VBFRAME_HELPER(VB_BLK *blk, VIDEO_FRAME_INFO_S *vbFrame, CVI_U32 srcWidth, CVI_U32 srcHeight,
                      PIXEL_FORMAT_E pixelFormat) {
  // Create Src Video Frame
  VIDEO_FRAME_S *vFrame = &vbFrame->stVFrame;
  memset(vFrame, 0, sizeof(VIDEO_FRAME_S));
  vFrame->enCompressMode = COMPRESS_MODE_NONE;
  vFrame->enPixelFormat = pixelFormat;
  vFrame->enVideoFormat = VIDEO_FORMAT_LINEAR;
  vFrame->enColorGamut = COLOR_GAMUT_BT709;
  vFrame->u32TimeRef = 0;
  vFrame->u64PTS = 0;
  vFrame->enDynamicRange = DYNAMIC_RANGE_SDR8;

  vFrame->u32Width = srcWidth;
  vFrame->u32Height = srcHeight;
  switch (vFrame->enPixelFormat) {
    case PIXEL_FORMAT_RGB_888:
    case PIXEL_FORMAT_BGR_888: {
      vFrame->u32Stride[0] = ALIGN(vFrame->u32Width, DEFAULT_ALIGN) * 3;
      vFrame->u32Stride[1] = 0;
      vFrame->u32Stride[2] = 0;
      // Don't need to align cause only 1 chn.
      vFrame->u32Length[0] = vFrame->u32Stride[0] * vFrame->u32Height;
      vFrame->u32Length[1] = 0;
      vFrame->u32Length[2] = 0;
    } break;
    case PIXEL_FORMAT_RGB_888_PLANAR: {
      vFrame->u32Stride[0] = ALIGN(vFrame->u32Width, DEFAULT_ALIGN);
      vFrame->u32Stride[1] = vFrame->u32Stride[0];
      vFrame->u32Stride[2] = vFrame->u32Stride[0];
      vFrame->u32Length[0] = ALIGN(vFrame->u32Stride[0] * vFrame->u32Height, SCALAR_4096_ALIGN_BUG);
      vFrame->u32Length[1] = vFrame->u32Length[0];
      vFrame->u32Length[2] = vFrame->u32Length[0];
    } break;
    case PIXEL_FORMAT_YUV_PLANAR_422: {
      vFrame->u32Stride[0] = ALIGN(vFrame->u32Width, DEFAULT_ALIGN);
      vFrame->u32Stride[1] = ALIGN(vFrame->u32Width >> 1, DEFAULT_ALIGN);
      vFrame->u32Stride[2] = ALIGN(vFrame->u32Width >> 1, DEFAULT_ALIGN);
      vFrame->u32Length[0] = ALIGN(vFrame->u32Stride[0] * vFrame->u32Height, SCALAR_4096_ALIGN_BUG);
      vFrame->u32Length[1] = ALIGN(vFrame->u32Stride[1] * vFrame->u32Height, SCALAR_4096_ALIGN_BUG);
      vFrame->u32Length[2] = ALIGN(vFrame->u32Stride[2] * vFrame->u32Height, SCALAR_4096_ALIGN_BUG);
    } break;
    case PIXEL_FORMAT_YUV_PLANAR_420: {
      uint32_t newHeight = ALIGN(vFrame->u32Height, 2);
      vFrame->u32Stride[0] = ALIGN(vFrame->u32Width, DEFAULT_ALIGN);
      vFrame->u32Stride[1] = ALIGN(vFrame->u32Width >> 1, DEFAULT_ALIGN);
      vFrame->u32Stride[2] = ALIGN(vFrame->u32Width >> 1, DEFAULT_ALIGN);
      vFrame->u32Length[0] = ALIGN(vFrame->u32Stride[0] * newHeight, SCALAR_4096_ALIGN_BUG);
      vFrame->u32Length[1] = ALIGN(vFrame->u32Stride[1] * newHeight / 2, SCALAR_4096_ALIGN_BUG);
      vFrame->u32Length[2] = ALIGN(vFrame->u32Stride[2] * newHeight / 2, SCALAR_4096_ALIGN_BUG);
    } break;
    case PIXEL_FORMAT_YUV_400: {
      vFrame->u32Stride[0] = ALIGN(vFrame->u32Width, DEFAULT_ALIGN);
      vFrame->u32Stride[1] = 0;
      vFrame->u32Stride[2] = 0;
      vFrame->u32Length[0] = vFrame->u32Stride[0] * vFrame->u32Height;
      vFrame->u32Length[1] = 0;
      vFrame->u32Length[2] = 0;
    } break;
    case PIXEL_FORMAT_FP32_C1: {
      vFrame->u32Stride[0] = ALIGN(vFrame->u32Width, DEFAULT_ALIGN) * sizeof(float);
      vFrame->u32Stride[1] = 0;
      vFrame->u32Stride[2] = 0;
      vFrame->u32Length[0] = vFrame->u32Stride[0] * vFrame->u32Height;
      vFrame->u32Length[1] = 0;
      vFrame->u32Length[2] = 0;
    } break;
    case PIXEL_FORMAT_BF16_C1: {
      vFrame->u32Stride[0] = ALIGN(vFrame->u32Width, DEFAULT_ALIGN) * sizeof(uint16_t);
      vFrame->u32Stride[1] = 0;
      vFrame->u32Stride[2] = 0;
      vFrame->u32Length[0] = vFrame->u32Stride[0] * vFrame->u32Height;
      vFrame->u32Length[1] = 0;
      vFrame->u32Length[2] = 0;
    } break;
    default:
      syslog(LOG_ERR, "Currently unsupported format %u\n", vFrame->enPixelFormat);
      return CVI_FAILURE;
      break;
  }

  CVI_U32 u32MapSize = vFrame->u32Length[0] + vFrame->u32Length[1] + vFrame->u32Length[2];
  *blk = CVI_VB_GetBlock(VB_INVALID_POOLID, u32MapSize);
  if (*blk == VB_INVALID_HANDLE) {
    syslog(LOG_ERR, "Can't acquire vb block Size: %d\n", u32MapSize);
    return CVI_FAILURE;
  }
  vbFrame->u32PoolId = CVI_VB_Handle2PoolId(*blk);
  vFrame->u64PhyAddr[0] = CVI_VB_Handle2PhysAddr(*blk);
  vFrame->u64PhyAddr[1] = vFrame->u64PhyAddr[0] + vFrame->u32Length[0];
  vFrame->u64PhyAddr[2] = vFrame->u64PhyAddr[1] + vFrame->u32Length[1];

#ifdef CONFIG_ALIOS
  vFrame->pu8VirAddr[0] = (uint8_t *)vFrame->u64PhyAddr[0];
#else
  vFrame->pu8VirAddr[0] = (uint8_t *)CVI_SYS_MmapCache(vFrame->u64PhyAddr[0], u32MapSize);
#endif
  vFrame->pu8VirAddr[1] = vFrame->pu8VirAddr[0] + vFrame->u32Length[0];
  vFrame->pu8VirAddr[2] = vFrame->pu8VirAddr[1] + vFrame->u32Length[1];

  return CVI_SUCCESS;
}

/**
 * @brief A helper function to create a VIDEO_FRAME_INFO_S buffer with given image information from
 * ION.
 * @ingroup core_vpss
 *
 * @param vbFrame Output VIDEO_FRAME_INFO_S buffer.
 * @param srcWidth Desired image width.
 * @param srcHeight Desired image height.
 * @param pixelFormat Desired image format.
 * @param alloc_name ION block name
 */
inline int __attribute__((always_inline))
CREATE_ION_HELPER(VIDEO_FRAME_INFO_S *vbFrame, CVI_U32 srcWidth, CVI_U32 srcHeight,
                  PIXEL_FORMAT_E pixelFormat, const char *alloc_name) {
  // Create Src Video Frame
  VIDEO_FRAME_S *vFrame = &vbFrame->stVFrame;
  memset(vFrame, 0, sizeof(VIDEO_FRAME_S));
  vFrame->enCompressMode = COMPRESS_MODE_NONE;
  vFrame->enPixelFormat = pixelFormat;
  vFrame->enVideoFormat = VIDEO_FORMAT_LINEAR;
  vFrame->enColorGamut = COLOR_GAMUT_BT709;
  vFrame->u32TimeRef = 0;
  vFrame->u64PTS = 0;
  vFrame->enDynamicRange = DYNAMIC_RANGE_SDR8;

  vFrame->u32Width = srcWidth;
  vFrame->u32Height = srcHeight;
  switch (vFrame->enPixelFormat) {
    case PIXEL_FORMAT_RGB_888:
    case PIXEL_FORMAT_BGR_888: {
      vFrame->u32Stride[0] = ALIGN(vFrame->u32Width * 3, DEFAULT_ALIGN);
      vFrame->u32Stride[1] = 0;
      vFrame->u32Stride[2] = 0;
      // Don't need to align cause only 1 chn.
      vFrame->u32Length[0] = vFrame->u32Stride[0] * vFrame->u32Height;
      vFrame->u32Length[1] = 0;
      vFrame->u32Length[2] = 0;
    } break;
    case PIXEL_FORMAT_RGB_888_PLANAR: {
      vFrame->u32Stride[0] = ALIGN(vFrame->u32Width, DEFAULT_ALIGN);
      vFrame->u32Stride[1] = vFrame->u32Stride[0];
      vFrame->u32Stride[2] = vFrame->u32Stride[0];
      vFrame->u32Length[0] = ALIGN(vFrame->u32Stride[0] * vFrame->u32Height, SCALAR_4096_ALIGN_BUG);
      vFrame->u32Length[1] = vFrame->u32Length[0];
      vFrame->u32Length[2] = vFrame->u32Length[0];
    } break;
    case PIXEL_FORMAT_YUV_PLANAR_422: {
      vFrame->u32Stride[0] = ALIGN(vFrame->u32Width, DEFAULT_ALIGN);
      vFrame->u32Stride[1] = ALIGN(vFrame->u32Width >> 1, DEFAULT_ALIGN);
      vFrame->u32Stride[2] = ALIGN(vFrame->u32Width >> 1, DEFAULT_ALIGN);
      vFrame->u32Length[0] = ALIGN(vFrame->u32Stride[0] * vFrame->u32Height, SCALAR_4096_ALIGN_BUG);
      vFrame->u32Length[1] = ALIGN(vFrame->u32Stride[1] * vFrame->u32Height, SCALAR_4096_ALIGN_BUG);
      vFrame->u32Length[2] = ALIGN(vFrame->u32Stride[2] * vFrame->u32Height, SCALAR_4096_ALIGN_BUG);
    } break;
    case PIXEL_FORMAT_YUV_PLANAR_420: {
      uint32_t newHeight = ALIGN(vFrame->u32Height, 2);
      vFrame->u32Stride[0] = ALIGN(vFrame->u32Width, DEFAULT_ALIGN);
      vFrame->u32Stride[1] = ALIGN(vFrame->u32Width >> 1, DEFAULT_ALIGN);
      vFrame->u32Stride[2] = ALIGN(vFrame->u32Width >> 1, DEFAULT_ALIGN);
      vFrame->u32Length[0] = ALIGN(vFrame->u32Stride[0] * newHeight, SCALAR_4096_ALIGN_BUG);
      vFrame->u32Length[1] = ALIGN(vFrame->u32Stride[1] * newHeight / 2, SCALAR_4096_ALIGN_BUG);
      vFrame->u32Length[2] = ALIGN(vFrame->u32Stride[2] * newHeight / 2, SCALAR_4096_ALIGN_BUG);
    } break;
    case PIXEL_FORMAT_YUV_400: {
      vFrame->u32Stride[0] = ALIGN(vFrame->u32Width, DEFAULT_ALIGN);
      vFrame->u32Stride[1] = 0;
      vFrame->u32Stride[2] = 0;
      vFrame->u32Length[0] = vFrame->u32Stride[0] * vFrame->u32Height;
      vFrame->u32Length[1] = 0;
      vFrame->u32Length[2] = 0;
    } break;
    case PIXEL_FORMAT_NV12: {
      vFrame->u32Stride[0] = ALIGN(vFrame->u32Width, DEFAULT_ALIGN);
      vFrame->u32Stride[1] = ALIGN(vFrame->u32Width, DEFAULT_ALIGN);
      vFrame->u32Stride[2] = 0;
      vFrame->u32Length[0] = ALIGN(vFrame->u32Stride[0] * vFrame->u32Height, SCALAR_4096_ALIGN_BUG);
      vFrame->u32Length[1] =
          ALIGN(vFrame->u32Stride[0] * vFrame->u32Height / 2, SCALAR_4096_ALIGN_BUG);
      vFrame->u32Length[2] = 0;
    } break;
    case PIXEL_FORMAT_NV21: {
      vFrame->u32Stride[0] = ALIGN(vFrame->u32Width, DEFAULT_ALIGN);
      vFrame->u32Stride[1] = ALIGN(vFrame->u32Width, DEFAULT_ALIGN);
      vFrame->u32Stride[2] = 0;
      vFrame->u32Length[0] = ALIGN(vFrame->u32Stride[0] * vFrame->u32Height, SCALAR_4096_ALIGN_BUG);
      vFrame->u32Length[1] =
          ALIGN(vFrame->u32Stride[0] * vFrame->u32Height / 2, SCALAR_4096_ALIGN_BUG);
      vFrame->u32Length[2] = 0;
    } break;
    case PIXEL_FORMAT_FP32_C1: {
      vFrame->u32Stride[0] = ALIGN(vFrame->u32Width, DEFAULT_ALIGN) * sizeof(float);
      vFrame->u32Stride[1] = 0;
      vFrame->u32Stride[2] = 0;
      vFrame->u32Length[0] = vFrame->u32Stride[0] * vFrame->u32Height;
      vFrame->u32Length[1] = 0;
      vFrame->u32Length[2] = 0;
    } break;
    case PIXEL_FORMAT_BF16_C1: {
      vFrame->u32Stride[0] = ALIGN(vFrame->u32Width, DEFAULT_ALIGN) * sizeof(uint16_t);
      vFrame->u32Stride[1] = 0;
      vFrame->u32Stride[2] = 0;
      vFrame->u32Length[0] = vFrame->u32Stride[0] * vFrame->u32Height;
      vFrame->u32Length[1] = 0;
      vFrame->u32Length[2] = 0;
    } break;
    default:
      syslog(LOG_ERR, "Currently unsupported format %u\n", vFrame->enPixelFormat);
      return CVI_FAILURE;
      break;
  }

  CVI_U32 u32MapSize = vFrame->u32Length[0] + vFrame->u32Length[1] + vFrame->u32Length[2];
  int ret = CVI_SYS_IonAlloc(&vFrame->u64PhyAddr[0], (CVI_VOID **)&vFrame->pu8VirAddr[0],
                             alloc_name, u32MapSize);
  if (ret != CVI_SUCCESS) {
    syslog(LOG_ERR, "Cannot allocate ion, size: %d, ret=%#x\n", u32MapSize, ret);
    return CVI_FAILURE;
  }

  vFrame->u64PhyAddr[1] = vFrame->u64PhyAddr[0] + vFrame->u32Length[0];
  vFrame->u64PhyAddr[2] = vFrame->u64PhyAddr[1] + vFrame->u32Length[1];
  vFrame->pu8VirAddr[1] = vFrame->pu8VirAddr[0] + vFrame->u32Length[0];
  vFrame->pu8VirAddr[2] = vFrame->pu8VirAddr[1] + vFrame->u32Length[1];

  return CVI_SUCCESS;
}

/**
 * @brief A helper function to unmap virtual address and set to NULL.
 * @ingroup core_vpss
 *
 * @param frame Input frame.
 */
inline void __attribute__((always_inline)) CACHED_VBFRAME_FLUSH_UNMAP(VIDEO_FRAME_INFO_S *frame) {
  uint32_t image_size =
      frame->stVFrame.u32Length[0] + frame->stVFrame.u32Length[1] + frame->stVFrame.u32Length[2];
  CVI_SYS_IonFlushCache(frame->stVFrame.u64PhyAddr[0], frame->stVFrame.pu8VirAddr[0], image_size);
  CVI_SYS_Munmap(
      (void *)frame->stVFrame.pu8VirAddr[0],
      frame->stVFrame.u32Length[0] + frame->stVFrame.u32Length[1] + frame->stVFrame.u32Length[2]);
  frame->stVFrame.pu8VirAddr[0] = NULL;
  frame->stVFrame.pu8VirAddr[1] = NULL;
  frame->stVFrame.pu8VirAddr[2] = NULL;
}
