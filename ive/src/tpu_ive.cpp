#include "ive_internal.hpp"

#include <iostream>
#include <memory>


/**
 * @brief IVE version info
 *
 */
const std::string g_ive_version = std::string(
    std::string("tpu-ive") + "_" +
    std::regex_replace(std::string(__DATE__), std::regex{" "}, std::string{"-"}) + "-" + __TIME__);
#define IVE_VERSION g_ive_version.c_str()



IVE_HANDLE CVI_TPU_IVE_CreateHandle() {
  IVE_HANDLE_CTX *handle_ctx = new IVE_HANDLE_CTX;
  if (createHandle(&handle_ctx->rt_handle, &handle_ctx->cvk_ctx) != CVI_SUCCESS) {
    LOGE("Create handle failed.\n");
    delete handle_ctx;
    return NULL;
  }
  if (handle_ctx->t_h.t_tblmgr.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx) != CVI_SUCCESS) {
    LOGE("Create table failed.\n");
    delete handle_ctx;
    return NULL;
  }
  LOGI("IVE_HANDLE created, version %s", IVE_VERSION);
  return (void *)handle_ctx;
}

CVI_S32 CVI_TPU_IVE_DestroyHandle(IVE_HANDLE pIveHandle) {
  IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);
  handle_ctx->t_h.t_tblmgr.free(handle_ctx->rt_handle);
  destroyHandle(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
  delete handle_ctx;
  LOGI("Destroy handle.\n");
  return CVI_SUCCESS;
}

CVI_S32 CVI_IVE_ImageInit(IVE_IMAGE_S *pstSrc) {
  if (pstSrc->tpu_block != NULL) {
    return CVI_SUCCESS;
  }
  size_t c = 1;
  CVIIMGTYPE img_type = CVIIMGTYPE::CVI_GRAY;
  cvk_fmt_t fmt = CVK_FMT_U8;
  std::vector<uint32_t> heights;
  switch (pstSrc->enType) {
    case IVE_IMAGE_TYPE_U8C1: {
      heights.push_back(pstSrc->u16Height);
    } break;
    case IVE_IMAGE_TYPE_S8C1: {
      heights.push_back(pstSrc->u16Height);
      fmt = CVK_FMT_I8;
    } break;
    case IVE_IMAGE_TYPE_YUV420SP: {
      c = 2;
      img_type = CVIIMGTYPE::CVI_YUV420SP;
      heights.push_back(pstSrc->u16Height);
      heights.push_back(pstSrc->u16Height >> 1);
    } break;
    case IVE_IMAGE_TYPE_YUV420P: {
      c = 3;
      img_type = CVIIMGTYPE::CVI_YUV420P;
      heights.push_back(pstSrc->u16Height);
      heights.push_back(pstSrc->u16Height >> 1);
      heights.push_back(pstSrc->u16Height >> 1);
    } break;
    case IVE_IMAGE_TYPE_YUV422P: {
      c = 3;
      img_type = CVIIMGTYPE::CVI_YUV422P;
      heights.resize(3, pstSrc->u16Height);
    } break;
    case IVE_IMAGE_TYPE_U8C3_PACKAGE: {
      c = 1;
      img_type = CVIIMGTYPE::CVI_RGB_PACKED;
      heights.push_back(pstSrc->u16Height);
    } break;
    case IVE_IMAGE_TYPE_S8C3_PACKAGE: {
      c = 1;
      img_type = CVIIMGTYPE::CVI_RGB_PACKED;
      heights.push_back(pstSrc->u16Height);
      fmt = CVK_FMT_I8;
    } break;
    case IVE_IMAGE_TYPE_U8C3_PLANAR: {
      c = 3;
      img_type = CVIIMGTYPE::CVI_RGB_PLANAR;
      heights.resize(c, pstSrc->u16Height);
    } break;
    case IVE_IMAGE_TYPE_S8C3_PLANAR: {
      c = 3;
      img_type = CVIIMGTYPE::CVI_RGB_PLANAR;
      heights.resize(c, pstSrc->u16Height);
      fmt = CVK_FMT_I8;
    } break;
    default: {
      LOGE("Unsupported conversion type: %u.\n", pstSrc->enType);
      return CVI_FAILURE;
    } break;
  }
  std::vector<uint32_t> strides, u32_length;
  for (size_t i = 0; i < c; i++) {
    strides.push_back(pstSrc->u16Stride[i]);
    u32_length.push_back(pstSrc->u16Stride[i] * heights[i]);
  }
  auto *cpp_img = new CviImg(pstSrc->u16Height, pstSrc->u16Width, strides, heights, u32_length,
                             pstSrc->pu8VirAddr[0], pstSrc->u64PhyAddr[0], img_type, fmt);
  if (!cpp_img->IsInit()) {
    LOGE("Failed to init IVE_IMAGE_S.\n");
    return CVI_FAILURE;
  }

  pstSrc->tpu_block = reinterpret_cast<CVI_IMG *>(cpp_img);
  return CVI_SUCCESS;
}

static void ViewAsYuv420(IVE_IMAGE_S *src) {
  int w = src->u16Stride[0];
  int w1 = src->u16Stride[1];
  int w2 = src->u16Stride[1];
  int h = src->u16Height;
  int h2 = src->u16Height / 2;

  memcpy(src->pu8VirAddr[2], src->pu8VirAddr[0] + w * h + w1 * h2, w2 * h2);
  memcpy(src->pu8VirAddr[1], src->pu8VirAddr[0] + w * h, w1 * h2);
}

static CviImg *ViewAsU8C1(IVE_IMAGE_S *src) {
  CVIIMGTYPE img_type = CVIIMGTYPE::CVI_GRAY;
  cvk_fmt_t fmt = CVK_FMT_U8;
  std::vector<uint32_t> heights;
  uint16_t new_height;
  CviImg *orig_cpp = reinterpret_cast<CviImg *>(src->tpu_block);

  if (src->enType == IVE_IMAGE_TYPE_YUV420P) {
    int halfh = src->u16Height / 2;
    int u_plane_size = src->u16Stride[1] * halfh;
    int v_plane_size = src->u16Stride[2] * halfh;
    int added_h = (u_plane_size + v_plane_size) / src->u16Stride[0];
    LOGD("ViewAsU8C1 stride0:%d,%d,%d,addedh:%d\n", (int)src->u16Stride[0], (int)src->u16Stride[1],
         (int)src->u16Stride[2], added_h);
    new_height = src->u16Height + added_h;

    if (orig_cpp->IsSubImg()) {
      std::copy(src->pu8VirAddr[1], src->pu8VirAddr[1] + u_plane_size,
                src->pu8VirAddr[0] + src->u16Stride[0] * src->u16Height);
      std::copy(src->pu8VirAddr[2], src->pu8VirAddr[2] + v_plane_size,
                src->pu8VirAddr[0] + src->u16Stride[0] * src->u16Height + u_plane_size);
    }
  } else if (src->enType == IVE_IMAGE_TYPE_U8C3_PLANAR) {
    new_height = src->u16Height * 3;
  } else {
    LOGE("Only support IVE_IMAGE_TYPE_YUV420P or IVE_IMAGE_TYPE_U8C3_PLANAR.\n");
    return nullptr;
  }

  heights.push_back(new_height);

  std::vector<uint32_t> strides, u32_length;
  strides.push_back(src->u16Stride[0]);

  if (Is4096Workaound(orig_cpp->GetImgType())) {
    u32_length.push_back(orig_cpp->GetImgCOffsets()[3]);
  } else {
    u32_length.push_back(src->u16Stride[0] * new_height);
  }

  auto *cpp_img = new CviImg(new_height, src->u16Width, strides, heights, u32_length,
                             src->pu8VirAddr[0], src->u64PhyAddr[0], img_type, fmt);
  if (!cpp_img->IsInit()) {
    LOGE("Failed to init IVE_IMAGE_S.\n");
    delete cpp_img;
    return nullptr;
  }
  return cpp_img;
}

CVI_S32 CVI_IVE_Blend_Pixel(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1,
                            IVE_SRC_IMAGE_S *pstSrc2, IVE_SRC_IMAGE_S *pstAlpha,
                            IVE_DST_IMAGE_S *pstDst, bool bInstant) {
  ScopedTrace t(__PRETTY_FUNCTION__);
  if (!IsValidImageType(pstSrc1, STRFY(pstSrc1), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_U8C3_PLANAR,
                        IVE_IMAGE_TYPE_YUV420P)) {
    LOGE(
        "image type of pstSrc1 should be one of (IVE_IMAGE_TYPE_U8C1, "
        "IVE_IMAGE_TYPE_U8C3_PLANAR)\n");
    return CVI_FAILURE;
  }
  if (!IsValidImageType(pstSrc2, STRFY(pstSrc2), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_U8C3_PLANAR,
                        IVE_IMAGE_TYPE_YUV420P)) {
    LOGE(
        "image type of pstSrc2 should be one of (IVE_IMAGE_TYPE_U8C1, "
        "IVE_IMAGE_TYPE_U8C3_PLANAR)\n");
    return CVI_FAILURE;
  }
  if (!IsValidImageType(pstAlpha, STRFY(pstAlpha), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_U8C3_PLANAR,
                        IVE_IMAGE_TYPE_YUV420P)) {
    LOGE(
        "image type of pstDst should be one of (IVE_IMAGE_TYPE_U8C1, "
        "IVE_IMAGE_TYPE_U8C3_PLANAR)\n");
    return CVI_FAILURE;
  }
  if (!IsValidImageType(pstDst, STRFY(pstDst), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_U8C3_PLANAR,
                        IVE_IMAGE_TYPE_YUV420P)) {
    LOGE(
        "image type of pstDst should be one of (IVE_IMAGE_TYPE_U8C1, "
        "IVE_IMAGE_TYPE_U8C3_PLANAR) \n");
    return CVI_FAILURE;
  }

  if ((pstDst->enType != pstSrc1->enType) || (pstDst->enType != pstSrc2->enType) ||
      (pstDst->enType != pstAlpha->enType)) {
    LOGE("source1/source2/dst image pixel format do not match,%d,%d,%d!\n", pstSrc1->enType,
         pstSrc2->enType, pstDst->enType);
    return CVI_FAILURE;
  }

  int ret = CVI_FAILURE;
  IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);

  std::shared_ptr<CviImg> cpp_src1;
  std::shared_ptr<CviImg> cpp_src2;
  std::shared_ptr<CviImg> cpp_alpha;
  std::shared_ptr<CviImg> cpp_dst;

  if (pstDst->enType == IVE_IMAGE_TYPE_YUV420P) {
    // NOTE: Computing tpu slice with different stride in different channel is quite complicated.
    // Instead, we consider YUV420P image as U8C1 with Wx(H + H / 2) image size so that there is
    // only one channel have to blended.
    cpp_src1 = std::shared_ptr<CviImg>(ViewAsU8C1(pstSrc1));
    cpp_src2 = std::shared_ptr<CviImg>(ViewAsU8C1(pstSrc2));
    cpp_alpha = std::shared_ptr<CviImg>(ViewAsU8C1(pstAlpha));
    cpp_dst = std::shared_ptr<CviImg>(ViewAsU8C1(pstDst));
  } else {
    cpp_src1 =
        std::shared_ptr<CviImg>(reinterpret_cast<CviImg *>(pstSrc1->tpu_block), [](CviImg *) {});
    cpp_src2 =
        std::shared_ptr<CviImg>(reinterpret_cast<CviImg *>(pstSrc2->tpu_block), [](CviImg *) {});
    cpp_alpha =
        std::shared_ptr<CviImg>(reinterpret_cast<CviImg *>(pstAlpha->tpu_block), [](CviImg *) {});
    cpp_dst =
        std::shared_ptr<CviImg>(reinterpret_cast<CviImg *>(pstDst->tpu_block), [](CviImg *) {});
  }

  if (cpp_src1 == nullptr || cpp_src2 == nullptr || cpp_alpha == nullptr || cpp_dst == nullptr) {
    LOGE("Cannot get tpu block\n");
    return CVI_FAILURE;
  }

  if ((cpp_src1->GetImgHeight() != cpp_src2->GetImgHeight()) ||
      (cpp_src1->GetImgHeight() != cpp_dst->GetImgHeight()) ||
      (cpp_src1->GetImgWidth() != cpp_src2->GetImgWidth()) ||
      (cpp_src1->GetImgWidth() != cpp_dst->GetImgWidth()) ||
      (cpp_src1->GetImgWidth() != cpp_alpha->GetImgWidth()) ||
      (cpp_src1->GetImgHeight() != cpp_alpha->GetImgHeight()) ||
      (cpp_src1->GetImgChannel() != cpp_src2->GetImgChannel()) ||
      (cpp_src1->GetImgChannel() != cpp_alpha->GetImgChannel()) ||
      (cpp_src1->GetImgChannel() != cpp_dst->GetImgChannel())) {
    LOGE("source1/source2/alpha/dst image size do not match!\n");
    return CVI_FAILURE;
  }

  std::vector<CviImg *> inputs = {cpp_src1.get(), cpp_src2.get(), cpp_alpha.get()};
  std::vector<CviImg *> outputs = {cpp_dst.get()};

  handle_ctx->t_h.t_blend_pixel.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
  ret = handle_ctx->t_h.t_blend_pixel.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs,
                                          outputs);
  if (pstDst->enType == IVE_IMAGE_TYPE_YUV420P) {
    CviImg *orig_cpp = reinterpret_cast<CviImg *>(pstDst->tpu_block);
    if (orig_cpp->IsSubImg()) {
      ViewAsYuv420(pstDst);
    }
  }
  return ret;
}

CVI_S32 VideoFrameYInfo2Image(VIDEO_FRAME_INFO_S *pstVFISrc, IVE_IMAGE_S *pstIIDst) {
  CviImg *cpp_img = nullptr;
  if (pstIIDst->tpu_block != NULL) {
    cpp_img = reinterpret_cast<CviImg *>(pstIIDst->tpu_block);
    if (!cpp_img->IsNullMem()) {
      LOGE("pstIIDst->tpu_block->m_rtmem is not NULL");
      return CVI_FAILURE;
    }
    if (cpp_img->GetMagicNum() != CVI_IMG_VIDEO_FRM_MAGIC_NUM) {
      printf("pstIIDst->tpu_block is not constructed from VIDEO_FRAME_INFO_S");
      return CVI_FAILURE;
    }
  }
  VIDEO_FRAME_S *pstVFSrc = &pstVFISrc->stVFrame;
  size_t c = 1;
  CVIIMGTYPE img_type = CVIIMGTYPE::CVI_GRAY;
  cvk_fmt_t fmt = CVK_FMT_U8;
  std::vector<uint32_t> heights;
  switch (pstVFSrc->enPixelFormat) {
    case PIXEL_FORMAT_YUV_400:
    case PIXEL_FORMAT_NV21:
    case PIXEL_FORMAT_NV12:
    case PIXEL_FORMAT_YUV_PLANAR_420: {
      pstIIDst->enType = IVE_IMAGE_TYPE_U8C1;
      heights.push_back(pstVFSrc->u32Height);
    } break;
    default: {
      LOGE("Unsupported conversion type: %u.\n", pstVFSrc->enPixelFormat);
      return CVI_FAILURE;
    } break;
  }
  std::vector<uint32_t> strides, u32_length;
  for (size_t i = 0; i < c; i++) {
    strides.push_back(pstVFSrc->u32Stride[i]);
    u32_length.push_back(pstVFSrc->u32Length[i]);
  }
  if (cpp_img == nullptr) {
    cpp_img = new CviImg(pstVFSrc->u32Height, pstVFSrc->u32Width, strides, heights, u32_length,
                         pstVFSrc->pu8VirAddr[0], pstVFSrc->u64PhyAddr[0], img_type, fmt);
  } else {
    cpp_img->ReInit(pstVFSrc->u32Height, pstVFSrc->u32Width, strides, heights, u32_length,
                    pstVFSrc->pu8VirAddr[0], pstVFSrc->u64PhyAddr[0], img_type, fmt);
  }

  if (!cpp_img->IsInit()) {
    LOGE("Failed to init IVE_IMAGE_S.\n");
    return CVI_FAILURE;
  }

  pstIIDst->tpu_block = reinterpret_cast<CVI_IMG *>(cpp_img);
  pstIIDst->u16Width = cpp_img->GetImgWidth();
  pstIIDst->u16Height = cpp_img->GetImgHeight();
  pstIIDst->u16Reserved = getFmtSize(fmt);

  size_t i_limit = cpp_img->GetImgChannel();
  for (size_t i = 0; i < i_limit; i++) {
    pstIIDst->pu8VirAddr[i] = cpp_img->GetVAddr() + cpp_img->GetImgCOffsets()[i];
    pstIIDst->u64PhyAddr[i] = cpp_img->GetPAddr() + cpp_img->GetImgCOffsets()[i];
    pstIIDst->u16Stride[i] = cpp_img->GetImgStrides()[i];
  }

  for (size_t i = i_limit; i < 3; i++) {
    pstIIDst->pu8VirAddr[i] = NULL;
    pstIIDst->u64PhyAddr[i] = 0;
    pstIIDst->u16Stride[i] = 0;
  }
  return CVI_SUCCESS;
}

CVI_S32 CVI_IVE_Blend_Pixel_Y(IVE_HANDLE pIveHandle, VIDEO_FRAME_INFO_S *pstSrc1,
                              VIDEO_FRAME_INFO_S *pstSrc2_dst, VIDEO_FRAME_INFO_S *pstAlpha) {
  IVE_IMAGE_S src1, src2, alpha, dst;
  memset(&src1, 0, sizeof(IVE_IMAGE_S));
  memset(&src2, 0, sizeof(IVE_IMAGE_S));
  memset(&alpha, 0, sizeof(IVE_IMAGE_S));
  memset(&dst, 0, sizeof(IVE_IMAGE_S));

  CVI_S32 ret = CVI_SUCCESS;
  ret = VideoFrameYInfo2Image(pstSrc1, &src1);
  if (ret != CVI_SUCCESS) {
    LOGE("pstSrc1 type not supported,could not extract Y plane");
    return ret;
  }

  ret = VideoFrameYInfo2Image(pstSrc2_dst, &src2);
  if (ret != CVI_SUCCESS) {
    LOGE("pstSrc2_dst type not supported,could not extract Y plane");
    return ret;
  }

  ret = VideoFrameYInfo2Image(pstAlpha, &alpha);
  if (ret != CVI_SUCCESS) {
    LOGE("pstAlpha type not supported,could not extract Y plane");
    return ret;
  }

  ret = VideoFrameYInfo2Image(pstSrc2_dst, &dst);
  if (ret != CVI_SUCCESS) {
    LOGE("pstSrc2 type not supported,could not extract Y plane");
    return ret;
  }

  CVI_IVE_Blend_Pixel(pIveHandle, &src1, &src2, &alpha, &dst, true);

  CviImg *src1_img = reinterpret_cast<CviImg *>(src1.tpu_block);
  delete src1_img;
  CviImg *src2_img = reinterpret_cast<CviImg *>(src2.tpu_block);
  delete src2_img;
  CviImg *alpha_img = reinterpret_cast<CviImg *>(alpha.tpu_block);
  delete alpha_img;
  CviImg *dst_img = reinterpret_cast<CviImg *>(dst.tpu_block);
  delete dst_img;
  return ret;
}
