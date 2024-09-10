#pragma once
#include <sample_comm.h>
#include <cvi_sys.h>

#include "ctx.h"

/**
 * @brief A helper function to get default VPSS_GRP_ATTR_S value with given image information.
 * @ingroup core_vpss
 *
 * @param srcWidth Input image width.
 * @param srcHeight Input image height.
 * @param enSrcFormat Input image format.
 */
inline void __attribute__((always_inline))
    vpss_grp_attr_default(VPSS_GRP_ATTR_S *pstVpssGrpAttr, uint32_t srcWidth, uint32_t srcHeight,
            PIXEL_FORMAT_E enSrcFormat, uint8_t u8Dev) {
        memset(pstVpssGrpAttr, 0, sizeof(VPSS_GRP_ATTR_S));
        pstVpssGrpAttr->stFrameRate.s32SrcFrameRate = -1;
        pstVpssGrpAttr->stFrameRate.s32DstFrameRate = -1;
        pstVpssGrpAttr->enPixelFormat = enSrcFormat;
        pstVpssGrpAttr->u32MaxW = srcWidth;
        pstVpssGrpAttr->u32MaxH = srcHeight;
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
    vpss_chn_attr_default(VPSS_CHN_ATTR_S *pastVpssChnAttr, uint32_t dstWidth, uint32_t dstHeight,
            PIXEL_FORMAT_E enDstFormat, bool keepAspectRatio) {
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

int vpss_init_helper(uint32_t vpssGrpId, uint32_t enSrcWidth, uint32_t enSrcHeight,
        PIXEL_FORMAT_E enSrcFormat, uint32_t enDstWidth, uint32_t enDstHeight,
        PIXEL_FORMAT_E enDstFormat, uint32_t enabledChannel,
		bool keepAspectRatio, uint8_t u8Dev, SERVICE_SBM sbm);

int init_vi(SERVICE_CTX *ctx);
int init_vpss(SERVICE_CTX *ctx);
void deinit_vi(SERVICE_CTX *ctx);

