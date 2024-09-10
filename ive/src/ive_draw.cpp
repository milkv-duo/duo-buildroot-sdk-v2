#include "ive_internal.hpp"

CVI_S32 CVI_IVE_DrawRect(IVE_HANDLE pIveHandle, IVE_DST_IMAGE_S *pstDst,
                         IVE_DRAW_RECT_CTRL_S *pstDrawCtrl, bool bInstant) {
  if (pstDst->tpu_block == NULL) {
    LOGE("Destination cannot be empty.\n");
    return CVI_FAILURE;
  }
  if (pstDst->enType != IVE_IMAGE_TYPE_U8C3_PLANAR && pstDst->enType != IVE_IMAGE_TYPE_YUV420P &&
      pstDst->enType != IVE_IMAGE_TYPE_YUV420SP) {
    LOGE(
        "Currently only supports IVE_IMAGE_TYPE_YUV420P, IVE_IMAGE_TYPE_YUV420SP and "
        "IVE_IMAGE_TYPE_U8C3_PLANAR format.\n");
    return CVI_FAILURE;
  }
  if (pstDrawCtrl->rect == NULL) {
    LOGE("pstDrawCtrl->rect is an empty array.\n");
    return CVI_FAILURE;
  }

  CVI_U8 pColorConverted[3] = {0};
  if (pstDst->enType == IVE_IMAGE_TYPE_YUV420P || pstDst->enType == IVE_IMAGE_TYPE_YUV420SP) {
    getYUVColorLimitedUV(pstDrawCtrl->color, pColorConverted);
  } else if (pstDst->enType == IVE_IMAGE_TYPE_U8C3_PLANAR) {
    pColorConverted[0] = pstDrawCtrl->color.r;
    pColorConverted[1] = pstDrawCtrl->color.g;
    pColorConverted[2] = pstDrawCtrl->color.b;
  } else {
    LOGE("Unsupported image convert type CVI_IVE_DrawRect.\n");
    return CVI_FAILURE;
  }
  IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);
  CviImg *cpp_dst = reinterpret_cast<CviImg *>(pstDst->tpu_block);
  for (CVI_U8 i = 0; i < pstDrawCtrl->numsOfRect; i++) {
    IVE_RECT_S &rect = pstDrawCtrl->rect[i];
    IveTPUDrawHollowRect::addCmd(handle_ctx->cvk_ctx, rect.pts[0].x, rect.pts[0].y, rect.pts[1].x,
                                 rect.pts[1].y, pColorConverted, (*cpp_dst));
    // Submit every 20 boxes
    if ((i + 1) % 20 == 0) {
      CVI_RT_Submit(handle_ctx->cvk_ctx);
    }
  }
  CVI_RT_Submit(handle_ctx->cvk_ctx);
  return CVI_SUCCESS;
}