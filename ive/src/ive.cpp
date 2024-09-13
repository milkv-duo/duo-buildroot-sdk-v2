#include "ive_internal.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#include <iostream>
#include <memory>
/**
 * @brief String array of IVE_IMAGE_S enType.
 *
 */
// clang-format off
const char *cviIveImgEnTypeStr[] = {STRFY(IVE_IMAGE_TYPE_U8C1),
                                    STRFY(IVE_IMAGE_TYPE_S8C1),
                                    STRFY(IVE_IMAGE_TYPE_YUV420SP),
                                    STRFY(IVE_IMAGE_TYPE_YUV422SP),
                                    STRFY(IVE_IMAGE_TYPE_YUV420P),
                                    STRFY(IVE_IMAGE_TYPE_YUV422P),
                                    STRFY(IVE_IMAGE_TYPE_S8C2_PACKAGE),
                                    STRFY(IVE_IMAGE_TYPE_S8C2_PLANAR),
                                    STRFY(IVE_IMAGE_TYPE_S16C1),
                                    STRFY(IVE_IMAGE_TYPE_U16C1),
                                    STRFY(IVE_IMAGE_TYPE_U8C3_PACKAGE),
                                    STRFY(IVE_IMAGE_TYPE_U8C3_PLANAR),
                                    STRFY(IVE_IMAGE_TYPE_S32C1),
                                    STRFY(IVE_IMAGE_TYPE_U32C1),
                                    STRFY(IVE_IMAGE_TYPE_S64C1),
                                    STRFY(IVE_IMAGE_TYPE_U64C1),
                                    STRFY(IVE_IMAGE_TYPE_BF16C1),
                                    STRFY(IVE_IMAGE_TYPE_FP32C1)};
// clang-format on

/**
 * @brief IVE version info
 *
 */
const std::string g_ive_version = std::string(
    std::string(CVIIVE_TAG) + "_" +
    std::regex_replace(std::string(__DATE__), std::regex{" "}, std::string{"-"}) + "-" + __TIME__);
#define IVE_VERSION g_ive_version.c_str()

static CviImg *ExtractYuvPlane(IVE_IMAGE_S *src, int plane) {
  CVIIMGTYPE img_type = CVIIMGTYPE::CVI_GRAY;
  if (src->enType != IVE_IMAGE_TYPE_YUV420P) {
    return nullptr;
  }
  cvk_fmt_t fmt = CVK_FMT_U8;
  std::vector<uint32_t> heights;
  uint32_t new_height = src->u32Height;
  uint32_t new_width = src->u32Width;
  if (plane > 0) {
    new_height = new_height / 2;
    new_width = new_width / 2;
  }

  heights.push_back(new_height);

  std::vector<uint32_t> strides, u32_length;
  strides.push_back(src->u16Stride[plane]);
  CviImg *orig_cpp = reinterpret_cast<CviImg *>(src->tpu_block);

  if (Is4096Workaound(orig_cpp->GetImgType())) {
    LOGD("to extract uv plane:%d,size:%d,size1:%d,stride:%d,newheight:%d\n", (int)plane,
         orig_cpp->GetImgCOffsets()[plane + 1] - orig_cpp->GetImgCOffsets()[plane],
         src->u16Stride[plane] * new_height, (int)src->u16Stride[plane], (int)new_height);
    u32_length.push_back(orig_cpp->GetImgCOffsets()[plane + 1] - orig_cpp->GetImgCOffsets()[plane]);
  } else {
    u32_length.push_back(src->u16Stride[plane] * new_height);
  }

  auto *cpp_img = new CviImg(new_height, new_width, strides, heights, u32_length,
                             src->pu8VirAddr[plane], src->u64PhyAddr[plane], img_type, fmt);

  if (!cpp_img->IsInit()) {
    LOGE("Failed to init IVE_IMAGE_S.\n");
    delete cpp_img;
    return nullptr;
  }
  return cpp_img;
}

IVE_HANDLE CVI_IVE_CreateHandle() {
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

CVI_S32 CVI_IVE_DestroyHandle(IVE_HANDLE pIveHandle) {
  IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);
  handle_ctx->t_h.t_tblmgr.free(handle_ctx->rt_handle);
  destroyHandle(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
  delete handle_ctx;
  LOGI("Destroy handle.\n");
  return CVI_SUCCESS;
}

CVI_S32 CVI_IVE_BufFlush(IVE_HANDLE pIveHandle, IVE_IMAGE_S *pstImg) {
  IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);
  if (pstImg->tpu_block == NULL) {
    return CVI_FAILURE;
  }
  auto *img = reinterpret_cast<CviImg *>(pstImg->tpu_block);
  return img->Flush(handle_ctx->rt_handle);
}

CVI_S32 CVI_IVE_BufRequest(IVE_HANDLE pIveHandle, IVE_IMAGE_S *pstImg) {
  ScopedTrace t(__PRETTY_FUNCTION__);
  IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);
  if (pstImg->tpu_block == NULL) {
    return CVI_FAILURE;
  }
  auto *img = reinterpret_cast<CviImg *>(pstImg->tpu_block);
  return img->Invld(handle_ctx->rt_handle);
}

CVI_S32 CVI_IVE_CreateMemInfo(IVE_HANDLE pIveHandle, IVE_MEM_INFO_S *pstMemInfo,
                              CVI_U32 u32ByteSize) {
  pstMemInfo->u32PhyAddr = 0;
  pstMemInfo->pu8VirAddr = new CVI_U8[u32ByteSize];
  pstMemInfo->u32ByteSize = u32ByteSize;
  return CVI_SUCCESS;
}

CVI_S32 CVI_IVE_CreateImage2(IVE_HANDLE pIveHandle, IVE_IMAGE_S *pstImg, IVE_IMAGE_TYPE_E enType,
                             uint32_t u32Width, uint32_t u32Height, IVE_IMAGE_S *pstBuffer) {
  if (u32Width == 0 || u32Height == 0) {
    LOGE("Image width or height cannot be 0.\n");
    pstImg->tpu_block = NULL;
    pstImg->enType = enType;
    pstImg->u32Width = 0;
    pstImg->u32Height = 0;
    pstImg->u16Reserved = 0;
    for (size_t i = 0; i < 3; i++) {
      pstImg->pu8VirAddr[i] = NULL;
      pstImg->u64PhyAddr[i] = 0;
      pstImg->u16Stride[i] = 0;
    }
    return CVI_FAILURE;
  }
  IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);
  int fmt_size = 1;
  cvk_fmt_t fmt = CVK_FMT_U8;
  CVIIMGTYPE img_type;
  std::vector<uint32_t> strides;
  std::vector<uint32_t> heights;
  switch (enType) {
    case IVE_IMAGE_TYPE_S8C1: {
      img_type = CVI_GRAY;
      const uint32_t stride = WidthAlign(u32Width, DEFAULT_ALIGN);
      strides.push_back(stride);
      heights.push_back(u32Height);
      fmt = CVK_FMT_I8;
    } break;
    case IVE_IMAGE_TYPE_U8C1: {
      const uint32_t stride = WidthAlign(u32Width, DEFAULT_ALIGN);
      strides.push_back(stride);
      heights.push_back(u32Height);
      img_type = CVI_GRAY;
    } break;
    case IVE_IMAGE_TYPE_YUV420SP: {
      img_type = CVI_YUV420SP;
      const uint32_t stride = WidthAlign(u32Width, DEFAULT_ALIGN);
      strides.push_back(stride);
      strides.push_back(stride);
      heights.push_back(u32Height);
      heights.push_back(u32Height >> 1);
    } break;
    case IVE_IMAGE_TYPE_YUV420P: {
      img_type = CVI_YUV420P;
      const uint32_t stride = WidthAlign(u32Width, DEFAULT_ALIGN);
      strides.push_back(stride);
      const uint32_t stride2 = WidthAlign(u32Width >> 1, DEFAULT_ALIGN);
      strides.push_back(stride2);
      strides.push_back(stride2);
      heights.push_back(u32Height);
      heights.push_back(u32Height >> 1);
      heights.push_back(u32Height >> 1);
    } break;
    case IVE_IMAGE_TYPE_YUV422P: {
      img_type = CVI_YUV422P;
      const uint32_t stride = WidthAlign(u32Width, DEFAULT_ALIGN);
      const uint32_t stride2 = WidthAlign(u32Width >> 1, DEFAULT_ALIGN);
      strides.push_back(stride);
      strides.push_back(stride2);
      strides.push_back(stride2);
      heights.resize(3, u32Height);
    } break;
    case IVE_IMAGE_TYPE_U8C3_PACKAGE: {
      img_type = CVI_RGB_PACKED;
      const uint32_t stride = WidthAlign(u32Width * 3, DEFAULT_ALIGN);
      strides.push_back(stride);
      heights.push_back(u32Height);
    } break;
    case IVE_IMAGE_TYPE_S8C3_PACKAGE: {
      img_type = CVI_RGB_PACKED;
      const uint32_t stride = WidthAlign(u32Width * 3, DEFAULT_ALIGN);
      strides.push_back(stride);
      heights.push_back(u32Height);
      fmt = CVK_FMT_I8;
    } break;
    case IVE_IMAGE_TYPE_U8C3_PLANAR: {
      img_type = CVI_RGB_PLANAR;
      const uint32_t stride = WidthAlign(u32Width, DEFAULT_ALIGN);
      strides.resize(3, stride);
      heights.resize(3, u32Height);
    } break;
    case IVE_IMAGE_TYPE_S8C3_PLANAR: {
      img_type = CVI_RGB_PLANAR;
      const uint32_t stride = WidthAlign(u32Width, DEFAULT_ALIGN);
      strides.resize(3, stride);
      heights.resize(3, u32Height);
      fmt = CVK_FMT_I8;
    } break;
    case IVE_IMAGE_TYPE_BF16C1: {
      img_type = CVI_SINGLE;
      const uint32_t stride = WidthAlign(u32Width, DEFAULT_ALIGN) * sizeof(int16_t);
      strides.push_back(stride);
      heights.push_back(u32Height);
      fmt_size = 2;
      fmt = CVK_FMT_BF16;
    } break;
    case IVE_IMAGE_TYPE_U16C1: {
      img_type = CVI_SINGLE;
      const uint32_t stride = WidthAlign(u32Width, DEFAULT_ALIGN) * sizeof(uint16_t);
      strides.push_back(stride);
      heights.push_back(u32Height);
      fmt_size = 2;
      fmt = CVK_FMT_U16;
    } break;
    case IVE_IMAGE_TYPE_S16C1: {
      img_type = CVI_SINGLE;
      const uint32_t stride = WidthAlign(u32Width, DEFAULT_ALIGN) * sizeof(uint16_t);
      strides.push_back(stride);
      heights.push_back(u32Height);
      fmt_size = 2;
      fmt = CVK_FMT_I16;
    } break;
    case IVE_IMAGE_TYPE_U32C1: {
      img_type = CVI_SINGLE;
      const uint32_t stride = WidthAlign(u32Width, DEFAULT_ALIGN) * sizeof(uint32_t);
      strides.push_back(stride);
      heights.push_back(u32Height);
      fmt_size = 4;
      fmt = CVK_FMT_U32;
    } break;
    case IVE_IMAGE_TYPE_FP32C1: {
      img_type = CVI_SINGLE;
      const uint32_t stride = WidthAlign(u32Width, DEFAULT_ALIGN) * sizeof(float);
      strides.push_back(stride);
      heights.push_back(u32Height);
      fmt_size = 4;
      fmt = CVK_FMT_F32;
    } break;
    default:
      LOGE("Not supported enType %s.\n", cviIveImgEnTypeStr[enType]);
      return CVI_FAILURE;
      break;
  }
  if (strides.size() == 0 || heights.size() == 0) {
    LOGE("[DEV] Stride not set.\n");
    return CVI_FAILURE;
  }

  CviImg *buffer_ptr =
      pstBuffer == NULL ? nullptr : reinterpret_cast<CviImg *>(pstBuffer->tpu_block);
  auto *cpp_img = new CviImg(handle_ctx->rt_handle, u32Height, u32Width, strides, heights, img_type,
                             fmt, buffer_ptr);
  if (!cpp_img->IsInit()) {
    LOGE("Failed to init IVE_IMAGE_S.\n");
    return CVI_FAILURE;
  }

  pstImg->tpu_block = reinterpret_cast<CVI_IMG *>(cpp_img);
  pstImg->enType = enType;
  pstImg->u32Width = cpp_img->GetImgWidth();
  pstImg->u32Height = cpp_img->GetImgHeight();
  pstImg->u16Reserved = fmt_size;

  size_t i_limit = cpp_img->GetImgChannel();
  for (size_t i = 0; i < i_limit; i++) {
    pstImg->pu8VirAddr[i] = cpp_img->GetVAddr() + cpp_img->GetImgCOffsets()[i];
    pstImg->u64PhyAddr[i] = cpp_img->GetPAddr() + cpp_img->GetImgCOffsets()[i];
    pstImg->u16Stride[i] = cpp_img->GetImgStrides()[i];
  }

  for (size_t i = i_limit; i < 3; i++) {
    pstImg->pu8VirAddr[i] = NULL;
    pstImg->u64PhyAddr[i] = -1;
    pstImg->u16Stride[i] = 0;
  }
  return CVI_SUCCESS;
}

CVI_S32 CVI_IVE_CreateImage(IVE_HANDLE pIveHandle, IVE_IMAGE_S *pstImg, IVE_IMAGE_TYPE_E enType,
                            CVI_U32 u32Width, CVI_U32 u32Height) {
  return CVI_IVE_CreateImage2(pIveHandle, pstImg, enType, u32Width, u32Height, NULL);
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
      heights.push_back(pstSrc->u32Height);
    } break;
    case IVE_IMAGE_TYPE_S8C1: {
      heights.push_back(pstSrc->u32Height);
      fmt = CVK_FMT_I8;
    } break;
    case IVE_IMAGE_TYPE_YUV420SP: {
      c = 2;
      img_type = CVIIMGTYPE::CVI_YUV420SP;
      heights.push_back(pstSrc->u32Height);
      heights.push_back(pstSrc->u32Height >> 1);
    } break;
    case IVE_IMAGE_TYPE_YUV420P: {
      c = 3;
      img_type = CVIIMGTYPE::CVI_YUV420P;
      heights.push_back(pstSrc->u32Height);
      heights.push_back(pstSrc->u32Height >> 1);
      heights.push_back(pstSrc->u32Height >> 1);
    } break;
    case IVE_IMAGE_TYPE_YUV422P: {
      c = 3;
      img_type = CVIIMGTYPE::CVI_YUV422P;
      heights.resize(3, pstSrc->u32Height);
    } break;
    case IVE_IMAGE_TYPE_U8C3_PACKAGE: {
      c = 1;
      img_type = CVIIMGTYPE::CVI_RGB_PACKED;
      heights.push_back(pstSrc->u32Height);
    } break;
    case IVE_IMAGE_TYPE_S8C3_PACKAGE: {
      c = 1;
      img_type = CVIIMGTYPE::CVI_RGB_PACKED;
      heights.push_back(pstSrc->u32Height);
      fmt = CVK_FMT_I8;
    } break;
    case IVE_IMAGE_TYPE_U8C3_PLANAR: {
      c = 3;
      img_type = CVIIMGTYPE::CVI_RGB_PLANAR;
      heights.resize(c, pstSrc->u32Height);
    } break;
    case IVE_IMAGE_TYPE_S8C3_PLANAR: {
      c = 3;
      img_type = CVIIMGTYPE::CVI_RGB_PLANAR;
      heights.resize(c, pstSrc->u32Height);
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
  auto *cpp_img = new CviImg(pstSrc->u32Height, pstSrc->u32Width, strides, heights, u32_length,
                             pstSrc->pu8VirAddr[0], pstSrc->u64PhyAddr[0], img_type, fmt);
  if (!cpp_img->IsInit()) {
    LOGE("Failed to init IVE_IMAGE_S.\n");
    return CVI_FAILURE;
  }

  pstSrc->tpu_block = reinterpret_cast<CVI_IMG *>(cpp_img);
  return CVI_SUCCESS;
}

CVI_S32 CVI_IVE_SubImage(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
                         CVI_U16 u16X1, CVI_U16 u16Y1, CVI_U16 u16X2, CVI_U16 u16Y2) {
  if (!IsValidImageType(pstSrc, STRFY(pstSrc), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_U8C3_PLANAR,
                        IVE_IMAGE_TYPE_BF16C1, IVE_IMAGE_TYPE_YUV422P, IVE_IMAGE_TYPE_YUV420P,
                        IVE_IMAGE_TYPE_YUV420SP)) {
    return CVI_FAILURE;
  }
  if (u16X1 >= u16X2 || u16Y1 >= u16Y2) {
    LOGE("(X1, Y1) must smaller than (X2, Y2).\n");
    return CVI_FAILURE;
  }
  if ((u16X1 % 2 != 0 || u16X2 % 2 != 0) && pstSrc->enType == IVE_IMAGE_TYPE_YUV422P) {
    LOGE("(X1, X2) must all not be odd.\n");
    return CVI_FAILURE;
  }
  if ((u16X1 % 2 != 0 || u16X2 % 2 != 0) && (u16Y1 % 2 != 0 || u16Y2 % 2 != 0) &&
      (pstSrc->enType == IVE_IMAGE_TYPE_YUV420P || pstSrc->enType == IVE_IMAGE_TYPE_YUV420SP)) {
    LOGE("(X1, X2, Y1, Y2) must all not be odd.\n");
    return CVI_FAILURE;
  }
  if (pstDst->tpu_block != NULL) {
    LOGE("pstDst must be empty.\n");
    return CVI_FAILURE;
  }
  IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);
  auto *src_img = reinterpret_cast<CviImg *>(pstSrc->tpu_block);
  auto *cpp_img = new CviImg(handle_ctx->rt_handle, *src_img, u16X1, u16Y1, u16X2, u16Y2);
  if (cpp_img->GetVAddr() == nullptr) {
    LOGE("generate sub image failed\n");
    delete cpp_img;
    return CVI_FAILURE;
  }
  pstDst->tpu_block = reinterpret_cast<CVI_IMG *>(cpp_img);

  pstDst->enType = pstSrc->enType;
  pstDst->u32Width = cpp_img->m_tg.shape.w;
  pstDst->u32Height = cpp_img->m_tg.shape.h;
  pstDst->u16Reserved = pstSrc->u16Reserved;

  size_t num_plane = cpp_img->GetImgCOffsets().size() - 1;
  LOGD("channel:%d,numplane:%d\n", (int)cpp_img->m_tg.shape.c, (int)num_plane);
  for (size_t i = 0; i < num_plane; i++) {
    pstDst->pu8VirAddr[i] = cpp_img->GetVAddr() + cpp_img->GetImgCOffsets()[i];
    pstDst->u64PhyAddr[i] = cpp_img->GetPAddr() + cpp_img->GetImgCOffsets()[i];
    pstDst->u16Stride[i] = cpp_img->GetImgStrides()[i];
    LOGD("updatesubimg ,plane:%d,coffset:%d\n", (int)i, (int)cpp_img->GetImgCOffsets()[i]);
  }

  LOGD("subimg planeoffset:%d,%d,%d\n", (int)(pstDst->u64PhyAddr[0] - pstSrc->u64PhyAddr[0]),
       (int)(pstDst->u64PhyAddr[1] - pstSrc->u64PhyAddr[1]),
       (int)(pstDst->u64PhyAddr[2] - pstSrc->u64PhyAddr[2]));

  for (size_t i = num_plane; i < 3; i++) {
    pstDst->pu8VirAddr[i] = NULL;
    pstDst->u64PhyAddr[i] = 0;
    pstDst->u16Stride[i] = 0;
  }
  return CVI_SUCCESS;
}

CVI_S32 CVI_IVE_Image2VideoFrameInfo(IVE_IMAGE_S *pstIISrc, VIDEO_FRAME_INFO_S *pstVFIDst) {
  pstVFIDst->u32PoolId = -1;
  VIDEO_FRAME_S *pstVFDst = &pstVFIDst->stVFrame;
  memset(pstVFDst, 0, sizeof(VIDEO_FRAME_S));
  switch (pstIISrc->enType) {
    case IVE_IMAGE_TYPE_U8C1: {
      pstVFDst->enPixelFormat = PIXEL_FORMAT_YUV_400;
    } break;
    case IVE_IMAGE_TYPE_YUV420SP: {
      pstVFDst->enPixelFormat = PIXEL_FORMAT_NV12;
    } break;
    case IVE_IMAGE_TYPE_YUV420P: {
      pstVFDst->enPixelFormat = PIXEL_FORMAT_YUV_PLANAR_420;
    } break;
    case IVE_IMAGE_TYPE_YUV422P: {
      pstVFDst->enPixelFormat = PIXEL_FORMAT_YUV_PLANAR_422;
    } break;
    case IVE_IMAGE_TYPE_U8C3_PACKAGE: {
      pstVFDst->enPixelFormat = PIXEL_FORMAT_RGB_888;
    } break;
    case IVE_IMAGE_TYPE_U8C3_PLANAR: {
      pstVFDst->enPixelFormat = PIXEL_FORMAT_RGB_888_PLANAR;
    } break;
    default: {
      LOGE("Unsupported conversion type: %s.\n", cviIveImgEnTypeStr[pstIISrc->enType]);
      return CVI_FAILURE;
    } break;
  }
  auto *src_img = reinterpret_cast<CviImg *>(pstIISrc->tpu_block);
  pstVFDst->u32Width = pstIISrc->u32Width;
  pstVFDst->u32Height = pstIISrc->u32Height;
  for (size_t i = 0; i < src_img->GetImgHeights().size(); i++) {
    pstVFDst->u32Stride[i] = pstIISrc->u16Stride[i];
    pstVFDst->u64PhyAddr[i] = pstIISrc->u64PhyAddr[i];
    pstVFDst->pu8VirAddr[i] = pstIISrc->pu8VirAddr[i];
  }

  for (size_t i = 0; i < src_img->GetImgHeights().size(); i++) {
    pstVFDst->u32Length[i] = src_img->GetImgHeights()[i] * src_img->GetImgStrides()[i];
  }
  return CVI_SUCCESS;
}

CVI_S32 CVI_IVE_VideoFrameInfo2Image(VIDEO_FRAME_INFO_S *pstVFISrc, IVE_IMAGE_S *pstIIDst) {
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
    case PIXEL_FORMAT_YUV_400: {
      pstIIDst->enType = IVE_IMAGE_TYPE_U8C1;
      heights.push_back(pstVFSrc->u32Height);
    } break;
    case PIXEL_FORMAT_NV21:
    case PIXEL_FORMAT_NV12: {
      c = 2;
      img_type = CVIIMGTYPE::CVI_YUV420SP;
      pstIIDst->enType = IVE_IMAGE_TYPE_YUV420SP;
      heights.push_back(pstVFSrc->u32Height);
      heights.push_back(pstVFSrc->u32Height >> 1);
    } break;
    case PIXEL_FORMAT_YUV_PLANAR_420: {
      c = 3;
      img_type = CVIIMGTYPE::CVI_YUV420P;
      pstIIDst->enType = IVE_IMAGE_TYPE_YUV420P;
      heights.push_back(pstVFSrc->u32Height);
      heights.push_back(pstVFSrc->u32Height >> 1);
      heights.push_back(pstVFSrc->u32Height >> 1);
    } break;
    case PIXEL_FORMAT_YUV_PLANAR_422: {
      c = 3;
      img_type = CVIIMGTYPE::CVI_YUV422P;
      pstIIDst->enType = IVE_IMAGE_TYPE_YUV422P;
      heights.resize(3, pstVFSrc->u32Height);
    } break;
    case PIXEL_FORMAT_RGB_888:
    case PIXEL_FORMAT_BGR_888: {
      c = 1;
      img_type = CVIIMGTYPE::CVI_RGB_PACKED;
      pstIIDst->enType = IVE_IMAGE_TYPE_U8C3_PACKAGE;
      heights.push_back(pstVFSrc->u32Height);
    } break;
    case PIXEL_FORMAT_RGB_888_PLANAR: {
      c = 3;
      img_type = CVIIMGTYPE::CVI_RGB_PLANAR;
      pstIIDst->enType = IVE_IMAGE_TYPE_U8C3_PLANAR;
      heights.resize(c, pstVFSrc->u32Height);
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
  pstIIDst->u32Width = cpp_img->GetImgWidth();
  pstIIDst->u32Height = cpp_img->GetImgHeight();
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

IVE_IMAGE_S CVI_IVE_ReadImage2(IVE_HANDLE pIveHandle, const char *filename, IVE_IMAGE_TYPE_E enType,
                               CVI_BOOL invertPackage) {
  int desiredNChannels = -1;
  switch (enType) {
    case IVE_IMAGE_TYPE_S8C1:
    case IVE_IMAGE_TYPE_U8C1:
      desiredNChannels = STBI_grey;
      break;
    case IVE_IMAGE_TYPE_S8C3_PLANAR:
    case IVE_IMAGE_TYPE_U8C3_PLANAR:
      desiredNChannels = STBI_rgb;
      break;
    case IVE_IMAGE_TYPE_S8C3_PACKAGE:
    case IVE_IMAGE_TYPE_U8C3_PACKAGE:
      desiredNChannels = STBI_rgb;
      break;
    default:
      LOGE("Not support channel %s.\n", cviIveImgEnTypeStr[enType]);
      break;
  }
  LOGI("to read image:%s,type:%d,channels:%d", filename, enType, desiredNChannels);
  IVE_IMAGE_S img;
  memset(&img, 0, sizeof(IVE_IMAGE_S));
  if (desiredNChannels >= 0) {
    int width, height, nChannels;
    stbi_uc *stbi_data = stbi_load(filename, &width, &height, &nChannels, desiredNChannels);
    if (stbi_data == nullptr) {
      LOGE("Image %s read failed.\n", filename);
      return img;
    }
    LOGI("to create cviimage,channels, width, height: %d %d %d\n", desiredNChannels, width, height);
    CVI_IVE_CreateImage(pIveHandle, &img, enType, width, height);
    LOGI("desiredNChannels, width, height: %d %d %d\n", desiredNChannels, width, height);
    if (enType == IVE_IMAGE_TYPE_U8C3_PLANAR || enType == IVE_IMAGE_TYPE_S8C3_PLANAR) {
      for (size_t i = 0; i < (size_t)height; i++) {
        for (size_t j = 0; j < (size_t)width; j++) {
          size_t stb_idx = (i * width + j) * 3;
          size_t img_idx = (i * img.u16Stride[0] + j);
          img.pu8VirAddr[0][img_idx] = stbi_data[stb_idx];
          img.pu8VirAddr[1][img_idx] = stbi_data[stb_idx + 1];
          img.pu8VirAddr[2][img_idx] = stbi_data[stb_idx + 2];
        }
      }
    } else {
      if (invertPackage &&
          (enType == IVE_IMAGE_TYPE_U8C3_PACKAGE || enType == IVE_IMAGE_TYPE_S8C3_PACKAGE)) {
        for (size_t i = 0; i < (size_t)height; i++) {
          uint32_t stb_stride = i * width * 3;
          uint32_t image_stride = (i * img.u16Stride[0]);
          for (size_t j = 0; j < (size_t)width; j++) {
            uint32_t stb_idx = stb_stride + (j * 3);
            uint32_t img_idx = image_stride + (j * 3);
            img.pu8VirAddr[0][img_idx] = stbi_data[stb_idx + 2];
            img.pu8VirAddr[0][img_idx + 1] = stbi_data[stb_idx + 1];
            img.pu8VirAddr[0][img_idx + 2] = stbi_data[stb_idx];
          }
        }
      } else {
        stbi_uc *ptr = stbi_data;
        for (size_t j = 0; j < (size_t)height; j++) {
          memcpy(img.pu8VirAddr[0] + (j * img.u16Stride[0]), ptr, width * desiredNChannels);
          ptr += width * desiredNChannels;
        }
      }
    }
    CVI_IVE_BufFlush(pIveHandle, &img);
    stbi_image_free(stbi_data);
  }
  return img;
}

IVE_IMAGE_S CVI_IVE_ReadImage(IVE_HANDLE pIveHandle, const char *filename,
                              IVE_IMAGE_TYPE_E enType) {
  return CVI_IVE_ReadImage2(pIveHandle, filename, enType, false);
}
#if 1
CVI_S32 CVI_IVE_ReadRawImage(IVE_HANDLE pIveHandle, IVE_IMAGE_S *pstImg, const char *filename,
                             IVE_IMAGE_TYPE_E enType, CVI_U16 u32Width, CVI_U16 u32Height) {
  float desiredNChannels = -1;

  switch (enType) {
    case IVE_IMAGE_TYPE_U8C1:
    case IVE_IMAGE_TYPE_S8C1:
      desiredNChannels = 1;
      break;
    case IVE_IMAGE_TYPE_YUV420SP:
      desiredNChannels = 1.5;
      break;
    case IVE_IMAGE_TYPE_U16C1:
    case IVE_IMAGE_TYPE_S16C1:
    case IVE_IMAGE_TYPE_YUV422SP:
      desiredNChannels = 2;
      break;
    case IVE_IMAGE_TYPE_U8C3_PLANAR:
    case IVE_IMAGE_TYPE_U8C3_PACKAGE:
      desiredNChannels = 3;
      break;
    default:
      printf("Not support channel %s.\n", cviIveImgEnTypeStr[enType]);
      return CVI_FAILURE_ILLEGAL_PARAM;
  }

  if (desiredNChannels > 0) {
    int buf_size = (int)((float)u32Width * (float)u32Height * (float)desiredNChannels);
    char buffer[buf_size];
    FILE *fp;

    fp = fopen(filename, "r");
    int readCnt = fread(buffer, 1, buf_size, fp);

    if (readCnt == 0) {
      printf("Image %s read failed.\n", filename);
      return ERR_IVE_READ_FILE;
    }
    fclose(fp);

    CVI_IVE_ReadImageArray(pIveHandle, pstImg, buffer, enType, u32Width, u32Height);
    return CVI_SUCCESS;
  }
  return CVI_FAILURE;
}

CVI_S32 CVI_IVE_ReadImageArray(IVE_HANDLE pIveHandle, IVE_IMAGE_S *pstImg, char *pBuffer,
                               IVE_IMAGE_TYPE_E enType, CVI_U16 u32Width, CVI_U16 u32Height) {
  char *ptr = NULL;

  memset(pstImg, 0, sizeof(IVE_IMAGE_S));

  CVI_IVE_CreateImage(pIveHandle, pstImg, enType, u32Width, u32Height);

  if (enType == IVE_IMAGE_TYPE_U8C3_PLANAR) {
    ptr = pBuffer;
    for (size_t j = 0; j < (size_t)u32Height; j++) {
      memcpy((char *)(uintptr_t)(pstImg->pu8VirAddr[0] + (j * pstImg->u16Stride[0])), ptr,
             u32Width);
      ptr += u32Width;
    }
    for (size_t j = 0; j < (size_t)pstImg->u32Height; j++) {
      memcpy((char *)(uintptr_t)(pstImg->pu8VirAddr[1] + (j * pstImg->u16Stride[1])), ptr,
             u32Width);
      ptr += u32Width;
    }
    for (size_t j = 0; j < (size_t)pstImg->u32Height; j++) {
      memcpy((char *)(uintptr_t)(pstImg->pu8VirAddr[2] + (j * pstImg->u16Stride[2])), ptr,
             u32Width);
      ptr += u32Width;
    }
  } else if (enType == IVE_IMAGE_TYPE_U8C3_PACKAGE) {
    // yyy... uuu... vvv... to yyy... uuu... vvv...
    ptr = pBuffer;
    for (size_t i = 0; i < (size_t)u32Height; i++) {
      uint32_t stb_stride = i * u32Width * 3;
      uint32_t image_stride = (i * pstImg->u16Stride[0]);

      for (size_t j = 0; j < (size_t)u32Width; j++) {
        uint32_t buf_idx = stb_stride + (j * 3);
        uint32_t img_idx = image_stride + (j * 3);
        ((char *)(uintptr_t)pstImg->pu8VirAddr[0])[img_idx] = ptr[buf_idx];
        ((char *)(uintptr_t)pstImg->pu8VirAddr[0])[img_idx + 1] = ptr[buf_idx + 1];
        ((char *)(uintptr_t)pstImg->pu8VirAddr[0])[img_idx + 2] = ptr[buf_idx + 2];
      }
    }
  } else if (enType == IVE_IMAGE_TYPE_YUV420SP) {
    ptr = pBuffer;
    for (size_t j = 0; j < (size_t)u32Height; j++) {
      memcpy((char *)(uintptr_t)(pstImg->pu8VirAddr[0] + (j * pstImg->u16Stride[0])), ptr,
             u32Width);
      ptr += u32Width;
    }
    for (size_t j = 0; j < (size_t)pstImg->u32Height / 2; j++) {
      memcpy((char *)(uintptr_t)(pstImg->pu8VirAddr[1] + (j * pstImg->u16Stride[1])), ptr,
             u32Width);
      ptr += u32Width;
    }
  } else if (enType == IVE_IMAGE_TYPE_YUV422SP) {
    ptr = pBuffer;
    for (size_t j = 0; j < (size_t)u32Height; j++) {
      memcpy((char *)(uintptr_t)(pstImg->pu8VirAddr[0] + (j * pstImg->u16Stride[0])), ptr,
             u32Width);
      ptr += u32Width;
    }
    for (size_t j = 0; j < (size_t)pstImg->u32Height; j++) {
      memcpy((char *)(uintptr_t)(pstImg->pu8VirAddr[1] + (j * pstImg->u16Stride[1])), ptr,
             u32Width);
      ptr += u32Width;
    }
  } else if (enType == IVE_IMAGE_TYPE_U16C1 || enType == IVE_IMAGE_TYPE_S16C1) {
    ptr = pBuffer;
    for (size_t j = 0; j < (size_t)u32Height; j++) {
      memcpy((char *)(uintptr_t)(pstImg->pu8VirAddr[0] + (j * pstImg->u16Stride[0])), ptr,
             u32Width * (sizeof(uint16_t)));
      ptr += u32Width * (sizeof(uint16_t));
    }
  } else {
    ptr = pBuffer;
    for (size_t j = 0; j < (size_t)u32Height; j++) {
      memcpy((char *)(uintptr_t)(pstImg->pu8VirAddr[0] + (j * pstImg->u16Stride[0])), ptr,
             u32Width);
      ptr += u32Width;
    }
  }

  return CVI_SUCCESS;
}

CVI_S32 CVI_IVE_WriteImage(IVE_HANDLE pIveHandle, const char *filename, IVE_IMAGE_S *pstImg) {
  int desiredNChannels = -1;
  int stride = 1;
  uint8_t *arr = nullptr;
  bool remove_buffer = false;
  switch (pstImg->enType) {
    case IVE_IMAGE_TYPE_U8C1:
      desiredNChannels = STBI_grey;
      arr = pstImg->pu8VirAddr[0];
      break;
    case IVE_IMAGE_TYPE_U8C3_PLANAR: {
      desiredNChannels = STBI_rgb;
      stride = 1;
      arr = new uint8_t[pstImg->u16Stride[0] * pstImg->u32Height * desiredNChannels];
      size_t image_total = pstImg->u16Stride[0] * pstImg->u32Height;
      for (size_t i = 0; i < image_total; i++) {
        size_t stb_idx = i * 3;
        arr[stb_idx] = pstImg->pu8VirAddr[0][i];
        arr[stb_idx + 1] = pstImg->pu8VirAddr[1][i];
        arr[stb_idx + 2] = pstImg->pu8VirAddr[2][i];
      }
      stride = 3;
      remove_buffer = true;
    } break;
    case IVE_IMAGE_TYPE_U8C3_PACKAGE:
      desiredNChannels = STBI_rgb;
      arr = pstImg->pu8VirAddr[0];
      stride = 1;
      break;
    default:
      LOGE("Not supported channel %s.", cviIveImgEnTypeStr[pstImg->enType]);
      return CVI_FAILURE;
      break;
  }
  CVI_IVE_BufRequest(pIveHandle, pstImg);
  stbi_write_png(filename, pstImg->u32Width, pstImg->u32Height, desiredNChannels, arr,
                 pstImg->u16Stride[0] * stride);
  if (remove_buffer) {
    delete[] arr;
  }
  return CVI_SUCCESS;
}
#endif

CVI_S32 CVI_SYS_FreeM(IVE_HANDLE pIveHandle, IVE_MEM_INFO_S *pstMemInfo) {
  delete[] pstMemInfo->pu8VirAddr;
  pstMemInfo->u32ByteSize = 0;
  return CVI_SUCCESS;
}

CVI_S32 CVI_SYS_FreeI(IVE_HANDLE pIveHandle, IVE_IMAGE_S *pstImg) {
  if (pstImg->tpu_block == NULL) {
    LOGD("Image tpu block is freed.\n");
    return CVI_SUCCESS;
  }
  auto *cpp_img = reinterpret_cast<CviImg *>(pstImg->tpu_block);
  if (!cpp_img->IsNullMem()) {
    if (pIveHandle == NULL) {
      LOGE("should have ive handle to release cpp_img inside memory");
    }
    IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);
    cpp_img->Free(handle_ctx->rt_handle);
  }
  delete cpp_img;
  pstImg->tpu_block = NULL;
  return CVI_SUCCESS;
}

CVI_S32 CVI_IVE_DMA(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
                    IVE_DMA_CTRL_S *pstDmaCtrl, bool bInstant) {
  ScopedTrace t(__PRETTY_FUNCTION__);
  if (CVI_IVE_ImageInit(pstSrc) != CVI_SUCCESS) {
    LOGE("Source cannot be inited.\n");
    return CVI_FAILURE;
  }
  if (CVI_IVE_ImageInit(pstDst) != CVI_SUCCESS) {
    LOGE("Destination cannot be inited.\n");
    return CVI_FAILURE;
  }
  // Special check with YUV 420 and 422, for copy only
  if ((pstSrc->enType == IVE_IMAGE_TYPE_YUV420P || pstSrc->enType == IVE_IMAGE_TYPE_YUV420SP ||
       pstSrc->enType == IVE_IMAGE_TYPE_YUV422P) &&
      pstDmaCtrl->enMode != IVE_DMA_MODE_DIRECT_COPY) {
    LOGE("Currently only supports IVE_DMA_MODE_DIRECT_COPY for YUV 420 and 422 images.");
    return CVI_FAILURE;
  }
  int ret = CVI_FAILURE;
  IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);
  if (pstDmaCtrl->enMode == IVE_DMA_MODE_DIRECT_COPY) {
#ifdef USE_CPU_COPY
    CVI_IVE_BufRequest(pIveHandle, pstSrc);
    CVI_IVE_BufRequest(pIveHandle, pstDst);
    uint size = pstSrc->u16Stride[0] * pstSrc->u32Height;
    memcpy(pstDst->pu8VirAddr[0], pstSrc->pu8VirAddr[0], size);
    CVI_IVE_BufFlush(pIveHandle, pstSrc);
    CVI_IVE_BufFlush(pIveHandle, pstDst);
#else
    CviImg *cpp_src = reinterpret_cast<CviImg *>(pstSrc->tpu_block);
    CviImg *cpp_dst = reinterpret_cast<CviImg *>(pstDst->tpu_block);
    // std::vector<CviImg*> inputs = {cpp_src};
    // std::vector<CviImg*> outputs = {cpp_dst};
    LOGD("use IVE_DMA_MODE_DIRECT_COPY\n");
    ret = IveTPUCopyDirect::run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, cpp_src, cpp_dst);
#endif
  } else if (pstDmaCtrl->enMode == IVE_DMA_MODE_INTERVAL_COPY) {
    handle_ctx->t_h.t_copy_int.setInvertal(pstDmaCtrl->u8HorSegSize, pstDmaCtrl->u8VerSegRows);
    handle_ctx->t_h.t_copy_int.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
    CviImg *cpp_src = reinterpret_cast<CviImg *>(pstSrc->tpu_block);
    CviImg *cpp_dst = reinterpret_cast<CviImg *>(pstDst->tpu_block);
    std::vector<CviImg *> inputs = {cpp_src};
    std::vector<CviImg *> outputs = {cpp_dst};

    ret = handle_ctx->t_h.t_copy_int.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs,
                                         outputs, true);
  }
  return ret;
}

CVI_S32 CVI_IVE_ImageTypeConvert(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
                                 IVE_DST_IMAGE_S *pstDst, IVE_ITC_CRTL_S *pstItcCtrl,
                                 bool bInstant) {
#ifndef CV180X
  if (CVI_IVE_ImageInit(pstSrc) != CVI_SUCCESS) {
    LOGE("Source cannot be inited.\n");
    return CVI_FAILURE;
  }
  if (CVI_IVE_ImageInit(pstDst) != CVI_SUCCESS) {
    LOGE("Destination cannot be inited.\n");
    return CVI_FAILURE;
  }
  ScopedTrace t(__PRETTY_FUNCTION__);
  IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);
  CviImg *cpp_src = reinterpret_cast<CviImg *>(pstSrc->tpu_block);
  CviImg *cpp_dst = reinterpret_cast<CviImg *>(pstDst->tpu_block);
  if (pstItcCtrl->enType == IVE_ITC_NORMALIZE) {
    if (cpp_src->m_tg.fmt == CVK_FMT_BF16 && cpp_dst->m_tg.fmt == CVK_FMT_F32) {
      cpp_src->Invld(handle_ctx->rt_handle);
      cpp_dst->Invld(handle_ctx->rt_handle);
      // uint16_t *src_ptr = (uint16_t *)cpp_src->GetVAddr();
      // float *dst_ptr = (float *)cpp_dst->GetVAddr();
      // uint64_t img_size = cpp_src->GetImgSize() / 2;
      // neonBF162F32(src_ptr, dst_ptr, img_size);
      union {
        short a[2];
        float b;
      } aaa;
      uint16_t stride = cpp_src->GetImgStrides()[0];
      uint16_t stride2 = cpp_dst->GetImgStrides()[0];
      for (size_t i = 0; i < cpp_src->GetImgHeight(); i++) {
        uint16_t *line16 = (uint16_t *)(cpp_src->GetVAddr() + i * stride);
        float *linef = (float *)(cpp_dst->GetVAddr() + i * stride2);
        for (size_t j = 0; j < cpp_src->GetImgWidth(); j++) {
          aaa.a[0] = 0;
          aaa.a[1] = line16[j];
          linef[j] = aaa.b;
        }
      }
      cpp_src->Flush(handle_ctx->rt_handle);
      cpp_dst->Flush(handle_ctx->rt_handle);
    } else if (cpp_src->m_tg.fmt == CVK_FMT_BF16 && cpp_dst->m_tg.fmt == CVK_FMT_U16) {
      cpp_src->Invld(handle_ctx->rt_handle);
      cpp_dst->Invld(handle_ctx->rt_handle);
      uint16_t *src_ptr = (uint16_t *)cpp_src->GetVAddr();
      uint16_t *dst_ptr = (uint16_t *)cpp_dst->GetVAddr();
      float min = std::numeric_limits<float>::max(), max = std::numeric_limits<float>::min();
      uint64_t img_size = cpp_src->m_tg.shape.c * cpp_src->m_tg.shape.h * cpp_src->m_tg.shape.w;
      neonBF16FindMinMax(src_ptr, img_size, &min, &max);
      neonBF162U16Normalize(src_ptr, dst_ptr, img_size, min, max);
      cpp_src->Flush(handle_ctx->rt_handle);
      cpp_dst->Flush(handle_ctx->rt_handle);
    } else if (cpp_src->m_tg.fmt == CVK_FMT_BF16 && cpp_dst->m_tg.fmt == CVK_FMT_I16) {
      cpp_src->Invld(handle_ctx->rt_handle);
      cpp_dst->Invld(handle_ctx->rt_handle);
      uint16_t *src_ptr = (uint16_t *)cpp_src->GetVAddr();
      int16_t *dst_ptr = (int16_t *)cpp_dst->GetVAddr();
      float min = std::numeric_limits<float>::max(), max = std::numeric_limits<float>::min();
      uint64_t img_size = cpp_src->m_tg.shape.c * cpp_src->m_tg.shape.h * cpp_src->m_tg.shape.w;
      neonBF16FindMinMax(src_ptr, img_size, &min, &max);
      neonBF162S16Normalize(src_ptr, dst_ptr, img_size, min, max);
      cpp_src->Flush(handle_ctx->rt_handle);
      cpp_dst->Flush(handle_ctx->rt_handle);
    } else if (cpp_src->m_tg.fmt == CVK_FMT_U16 &&
               (cpp_dst->m_tg.fmt == CVK_FMT_U8 || cpp_dst->m_tg.fmt == CVK_FMT_I8)) {
      cpp_src->Invld(handle_ctx->rt_handle);
      cpp_dst->Invld(handle_ctx->rt_handle);
      uint16_t *src_ptr = (uint16_t *)cpp_src->GetVAddr();
      uint8_t *dst_ptr = (uint8_t *)cpp_dst->GetVAddr();
      uint16_t min = 65535, max = 0;
      uint64_t img_size = cpp_src->m_tg.shape.c * cpp_src->m_tg.shape.h * cpp_src->m_tg.shape.w;
      neonU16FindMinMax(src_ptr, img_size, &min, &max);
      if (cpp_dst->m_tg.fmt == CVK_FMT_U8) {
        neonU162U8Normalize(src_ptr, dst_ptr, img_size, min, max);
      } else {
        neonU162S8Normalize(src_ptr, (int8_t *)dst_ptr, img_size, min, max);
      }
      cpp_src->Flush(handle_ctx->rt_handle);
      cpp_dst->Flush(handle_ctx->rt_handle);
    } else if (cpp_src->m_tg.fmt == CVK_FMT_BF16 &&
               (cpp_dst->m_tg.fmt == CVK_FMT_U8 || cpp_dst->m_tg.fmt == CVK_FMT_I8)) {
      cpp_src->Invld(handle_ctx->rt_handle);
      cpp_dst->Invld(handle_ctx->rt_handle);
      uint16_t *src_ptr = (uint16_t *)cpp_src->GetVAddr();
      float min = std::numeric_limits<float>::max(), max = std::numeric_limits<float>::min();
      uint64_t img_size = cpp_src->m_tg.shape.c * cpp_src->m_tg.shape.h * cpp_src->m_tg.shape.w;
      neonBF16FindMinMax(src_ptr, img_size, &min, &max);
      handle_ctx->t_h.t_norm.setMinMax(min, max);
      handle_ctx->t_h.t_norm.setOutputFMT(cpp_dst->m_tg.fmt);
      handle_ctx->t_h.t_norm.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
      CviImg *cpp_src = reinterpret_cast<CviImg *>(pstSrc->tpu_block);
      CviImg *cpp_dst = reinterpret_cast<CviImg *>(pstDst->tpu_block);
      std::vector<CviImg *> inputs = {cpp_src};
      std::vector<CviImg *> outputs = {cpp_dst};
      handle_ctx->t_h.t_norm.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs, outputs);
    }
  } else if (pstItcCtrl->enType == IVE_ITC_SATURATE) {
    if (cpp_src->m_tg.fmt == CVK_FMT_BF16 && cpp_dst->m_tg.fmt == CVK_FMT_F32) {
      cpp_src->Invld(handle_ctx->rt_handle);
      cpp_dst->Invld(handle_ctx->rt_handle);
      // uint16_t *src_ptr = (uint16_t *)cpp_src->GetVAddr();
      // float *dst_ptr = (float *)cpp_dst->GetVAddr();
      // uint64_t img_size = cpp_src->GetImgSize() / 2;
      // neonBF162F32(src_ptr, dst_ptr, img_size);
      union {
        short a[2];
        float b;
      } aaa;
      uint16_t stride = cpp_src->GetImgStrides()[0];
      uint16_t stride2 = cpp_dst->GetImgStrides()[0];
      for (size_t i = 0; i < cpp_src->GetImgHeight(); i++) {
        uint16_t *line16 = (uint16_t *)(cpp_src->GetVAddr() + i * stride);
        float *linef = (float *)(cpp_dst->GetVAddr() + i * stride2);
        for (size_t j = 0; j < cpp_src->GetImgWidth(); j++) {
          aaa.a[0] = 0;
          aaa.a[1] = line16[j];
          linef[j] = aaa.b;
        }
      }
      cpp_src->Flush(handle_ctx->rt_handle);
      cpp_dst->Flush(handle_ctx->rt_handle);
    } else if (cpp_src->m_tg.fmt == CVK_FMT_BF16 && cpp_dst->m_tg.fmt == CVK_FMT_U16) {
      cpp_src->Invld(handle_ctx->rt_handle);
      cpp_dst->Invld(handle_ctx->rt_handle);
      uint16_t *src_ptr = (uint16_t *)cpp_src->GetVAddr();
      uint16_t *dst_ptr = (uint16_t *)cpp_dst->GetVAddr();
      uint64_t img_size = cpp_src->GetImgSize() / 2;
      neonBF162U16(src_ptr, dst_ptr, img_size);
      cpp_src->Flush(handle_ctx->rt_handle);
      cpp_dst->Flush(handle_ctx->rt_handle);
    } else if (cpp_src->m_tg.fmt == CVK_FMT_BF16 && cpp_dst->m_tg.fmt == CVK_FMT_I16) {
      cpp_src->Invld(handle_ctx->rt_handle);
      cpp_dst->Invld(handle_ctx->rt_handle);
      uint16_t *src_ptr = (uint16_t *)cpp_src->GetVAddr();
      int16_t *dst_ptr = (int16_t *)cpp_dst->GetVAddr();
      uint64_t img_size = cpp_src->GetImgSize() / 2;
      neonBF162S16(src_ptr, dst_ptr, img_size);
      cpp_src->Flush(handle_ctx->rt_handle);
      cpp_dst->Flush(handle_ctx->rt_handle);
    } else if ((cpp_src->m_tg.fmt == CVK_FMT_BF16 || cpp_src->m_tg.fmt == CVK_FMT_U8 ||
                cpp_src->m_tg.fmt == CVK_FMT_I8) &&
               (cpp_dst->m_tg.fmt == CVK_FMT_BF16 || cpp_dst->m_tg.fmt == CVK_FMT_U8 ||
                cpp_dst->m_tg.fmt == CVK_FMT_I8)) {
      CviImg *cpp_src = reinterpret_cast<CviImg *>(pstSrc->tpu_block);
      CviImg *cpp_dst = reinterpret_cast<CviImg *>(pstDst->tpu_block);

      IveTPUCopyDirect::run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, cpp_src, cpp_dst);
    } else {
      LOGE("Unsupported input output image type ( %u, %u).\n", cpp_src->m_tg.fmt,
           cpp_dst->m_tg.fmt);
      return CVI_FAILURE;
    }
  } else {
    LOGE("Unsupported enType %u.\n", pstItcCtrl->enType);
    return CVI_FAILURE;
  }
  return CVI_SUCCESS;
#else
  return CVI_FAILURE;
#endif
}

CVI_S32 CVI_IVE_ConstFill(IVE_HANDLE pIveHandle, const CVI_FLOAT value, IVE_DST_IMAGE_S *pstDst,
                          bool bInstant) {
  ScopedTrace t(__PRETTY_FUNCTION__);
  if (IsValidImageType(pstDst, STRFY(pstDst), IVE_IMAGE_TYPE_YUV420P)) {
    int ret = CVI_SUCCESS;
    IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);
    for (int i = 0; i < 3; i++) {
      CviImg *planei = ExtractYuvPlane(pstDst, i);
      std::vector<CviImg *> outputs = {planei};
      ret |= handle_ctx->t_h.t_const_fill.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, value,
                                              outputs);
      delete planei;
    }
    return ret;
  }
  if (!IsValidImageType(pstDst, STRFY(pstDst), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_U8C3_PLANAR,
                        IVE_IMAGE_TYPE_BF16C1)) {
    return CVI_FAILURE;
  }

  IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);
  CviImg *cpp_dst = reinterpret_cast<CviImg *>(pstDst->tpu_block);
  std::vector<CviImg *> outputs = {cpp_dst};
  return handle_ctx->t_h.t_const_fill.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, value,
                                          outputs);
}

CVI_S32 CVI_IVE_ConvertScaleAbs(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
                                IVE_DST_IMAGE_S *pstDst, IVE_CONVERT_SCALE_ABS_CRTL *pstConvertCtrl,
                                bool bInstant) {
  ScopedTrace t(__PRETTY_FUNCTION__);
  if (!IsValidImageType(pstSrc, STRFY(pstSrc), IVE_IMAGE_TYPE_BF16C1)) {
    LOGE("image type of pstSrc should be IVE_IMAGE_TYPE_BF16C1\n");
    return CVI_FAILURE;
  }

  if (!IsValidImageType(pstDst, STRFY(pstDst), IVE_IMAGE_TYPE_U8C1)) {
    LOGE("image type of pstDst should be IVE_IMAGE_TYPE_U8C1\n");
    return CVI_FAILURE;
  }

  uint16_t alpha = convert_fp32_bf16(-1.0 * pstConvertCtrl->alpha);
  if (alpha == NAN_VALUE) {
    LOGE("alpha value: %f is NaN\n", pstConvertCtrl->alpha);
    return CVI_FAILURE;
  }

  uint16_t beta = convert_fp32_bf16(-1.0 * pstConvertCtrl->beta);
  if (beta == NAN_VALUE) {
    LOGE("beta value: %f is NaN\n", pstConvertCtrl->beta);
    return CVI_FAILURE;
  }

  int ret = CVI_FAILURE;
  IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);

  std::shared_ptr<CviImg> cpp_src;
  std::shared_ptr<CviImg> cpp_dst;

  cpp_src = std::shared_ptr<CviImg>(reinterpret_cast<CviImg *>(pstSrc->tpu_block), [](CviImg *) {});
  cpp_dst = std::shared_ptr<CviImg>(reinterpret_cast<CviImg *>(pstDst->tpu_block), [](CviImg *) {});

  if (cpp_src == nullptr || cpp_dst == nullptr) {
    LOGE("Cannot get tpu block\n");
    return CVI_FAILURE;
  }

  if ((cpp_src->GetImgHeight() != cpp_dst->GetImgHeight()) ||
      (cpp_src->GetImgWidth() != cpp_dst->GetImgWidth())) {
    LOGE("source/dst image size do not matched!\n");
    return CVI_FAILURE;
  }

  std::vector<CviImg *> inputs = {cpp_src.get()};
  std::vector<CviImg *> outputs = {cpp_dst.get()};

  handle_ctx->t_h.t_convert_scale_abs.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);

  handle_ctx->t_h.t_convert_scale_abs.setAlpha(alpha);
  handle_ctx->t_h.t_convert_scale_abs.setBeta(beta);
  ret = handle_ctx->t_h.t_convert_scale_abs.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs,
                                                outputs);
  return ret;
}

CVI_S32 CVI_IVE_Add(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1, IVE_SRC_IMAGE_S *pstSrc2,
                    IVE_DST_IMAGE_S *pstDst, IVE_ADD_CTRL_S *ctrl, bool bInstant) {
  ScopedTrace t(__PRETTY_FUNCTION__);
  if (!IsValidImageType(pstSrc1, STRFY(pstSrc1), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_U8C3_PLANAR,
                        IVE_IMAGE_TYPE_BF16C1)) {
    return CVI_FAILURE;
  }
  if (!IsValidImageType(pstSrc2, STRFY(pstSrc2), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_S8C1,
                        IVE_IMAGE_TYPE_U8C3_PLANAR, IVE_IMAGE_TYPE_BF16C1)) {
    return CVI_FAILURE;
  }
  if (!IsValidImageType(pstDst, STRFY(pstDst), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_U8C3_PLANAR,
                        IVE_IMAGE_TYPE_BF16C1)) {
    return CVI_FAILURE;
  }

  int ret = CVI_FAILURE;
  IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);
  CviImg *cpp_src1 = reinterpret_cast<CviImg *>(pstSrc1->tpu_block);
  CviImg *cpp_src2 = reinterpret_cast<CviImg *>(pstSrc2->tpu_block);
  CviImg *cpp_dst = reinterpret_cast<CviImg *>(pstDst->tpu_block);
  std::vector<CviImg *> inputs = {cpp_src1, cpp_src2};
  std::vector<CviImg *> outputs = {cpp_dst};

  const float &x = ctrl->aX;
  const float &y = ctrl->bY;
  bool is_bf16 =
      (pstSrc1->enType == IVE_IMAGE_TYPE_BF16C1 || pstSrc2->enType == IVE_IMAGE_TYPE_BF16C1 ||
       pstDst->enType == IVE_IMAGE_TYPE_BF16C1)
          ? true
          : false;
  if (((x == 1 && y == 1) || (x == 0.f && y == 0.f)) && !is_bf16) {
    if (pstSrc2->enType == IVE_IMAGE_TYPE_S8C1) {
      handle_ctx->t_h.t_add_signed.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
      ret = handle_ctx->t_h.t_add_signed.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs,
                                             outputs);
    } else {
      handle_ctx->t_h.t_add.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
      ret = handle_ctx->t_h.t_add.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs, outputs);
    }
  } else {
    handle_ctx->t_h.t_add_bf16.setCoef(x, y);
    handle_ctx->t_h.t_add_bf16.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
    ret =
        handle_ctx->t_h.t_add_bf16.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs, outputs);
  }
  return ret;
}

static void ViewAsYuv420(IVE_IMAGE_S *src) {
  int w = src->u16Stride[0];
  int w1 = src->u16Stride[1];
  int w2 = src->u16Stride[1];
  int h = src->u32Height;
  int h2 = src->u32Height / 2;

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
    int halfh = src->u32Height / 2;
    int u_plane_size = src->u16Stride[1] * halfh;
    int v_plane_size = src->u16Stride[2] * halfh;
    int added_h = (u_plane_size + v_plane_size) / src->u16Stride[0];
    LOGD("ViewAsU8C1 stride0:%d,%d,%d,addedh:%d\n", (int)src->u16Stride[0], (int)src->u16Stride[1],
         (int)src->u16Stride[2], added_h);
    new_height = src->u32Height + added_h;

    if (orig_cpp->IsSubImg()) {
      std::copy(src->pu8VirAddr[1], src->pu8VirAddr[1] + u_plane_size,
                src->pu8VirAddr[0] + src->u16Stride[0] * src->u32Height);
      std::copy(src->pu8VirAddr[2], src->pu8VirAddr[2] + v_plane_size,
                src->pu8VirAddr[0] + src->u16Stride[0] * src->u32Height + u_plane_size);
    }
  } else if (src->enType == IVE_IMAGE_TYPE_U8C3_PLANAR) {
    new_height = src->u32Height * 3;
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

  auto *cpp_img = new CviImg(new_height, src->u32Width, strides, heights, u32_length,
                             src->pu8VirAddr[0], src->u64PhyAddr[0], img_type, fmt);
  if (!cpp_img->IsInit()) {
    LOGE("Failed to init IVE_IMAGE_S.\n");
    delete cpp_img;
    return nullptr;
  }
  return cpp_img;
}

CVI_S32 CVI_IVE_Blend(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1, IVE_SRC_IMAGE_S *pstSrc2,
                      IVE_DST_IMAGE_S *pstDst, IVE_BLEND_CTRL_S *pstBlendCtrl, bool bInstant) {
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
  if (!IsValidImageType(pstDst, STRFY(pstDst), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_U8C3_PLANAR,
                        IVE_IMAGE_TYPE_YUV420P)) {
    LOGE(
        "image type of pstDst should be one of (IVE_IMAGE_TYPE_U8C1, "
        "IVE_IMAGE_TYPE_U8C3_PLANAR)\n");
    return CVI_FAILURE;
  }

  if ((pstDst->enType != pstSrc1->enType) || (pstDst->enType != pstSrc2->enType)) {
    LOGE("source1/source2/dst image pixel format do not match!\n");
    return CVI_FAILURE;
  }

  int ret = CVI_FAILURE;
  IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);

  std::shared_ptr<CviImg> cpp_src1;
  std::shared_ptr<CviImg> cpp_src2;
  std::shared_ptr<CviImg> cpp_dst;

  if (pstDst->enType == IVE_IMAGE_TYPE_YUV420P) {
    // NOTE: Computing tpu slice with different stride in different channel is quite complicated.
    // Instead, we consider YUV420P image as U8C1 with Wx(H + H / 2) image size so that there is
    // only one channel have to blended.
    cpp_src1 = std::shared_ptr<CviImg>(ViewAsU8C1(pstSrc1));
    cpp_src2 = std::shared_ptr<CviImg>(ViewAsU8C1(pstSrc2));
    cpp_dst = std::shared_ptr<CviImg>(ViewAsU8C1(pstDst));
  } else {
    cpp_src1 =
        std::shared_ptr<CviImg>(reinterpret_cast<CviImg *>(pstSrc1->tpu_block), [](CviImg *) {});
    cpp_src2 =
        std::shared_ptr<CviImg>(reinterpret_cast<CviImg *>(pstSrc2->tpu_block), [](CviImg *) {});
    cpp_dst =
        std::shared_ptr<CviImg>(reinterpret_cast<CviImg *>(pstDst->tpu_block), [](CviImg *) {});
  }

  if (cpp_src1 == nullptr || cpp_src2 == nullptr || cpp_dst == nullptr) {
    LOGE("Cannot get tpu block\n");
    return CVI_FAILURE;
  }

  if ((cpp_src1->GetImgHeight() != cpp_src2->GetImgHeight()) ||
      (cpp_src1->GetImgHeight() != cpp_dst->GetImgHeight()) ||
      (cpp_src1->GetImgWidth() != cpp_src2->GetImgWidth()) ||
      (cpp_src1->GetImgWidth() != cpp_dst->GetImgWidth())) {
    LOGE("source1/source2/dst image size do not matched!\n");
    return CVI_FAILURE;
  }

  std::vector<CviImg *> inputs = {cpp_src1.get(), cpp_src2.get()};
  std::vector<CviImg *> outputs = {cpp_dst.get()};

  handle_ctx->t_h.t_blend.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
  handle_ctx->t_h.t_blend.setWeight(pstBlendCtrl->u8Weight);
  ret = handle_ctx->t_h.t_blend.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs, outputs);
  return ret;
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

CVI_S32 CVI_IVE_Blend_Pixel_S8_CLIP(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1,
                                    IVE_SRC_IMAGE_S *pstSrc2, IVE_SRC_IMAGE_S *pstAlpha,
                                    IVE_DST_IMAGE_S *pstDst) {
  if (!IsValidImageType(pstSrc1, STRFY(pstSrc1), IVE_IMAGE_TYPE_S8C1, IVE_IMAGE_TYPE_S8C3_PLANAR)) {
    LOGE(
        "image type of pstSrc1 should be one of "
        "(IVE_IMAGE_TYPE_S8C1,IVE_IMAGE_TYPE_S8C3_PLANAR)\n");
    return CVI_FAILURE;
  }

  if (!IsValidImageType(pstSrc2, STRFY(pstSrc2), IVE_IMAGE_TYPE_S8C1, IVE_IMAGE_TYPE_S8C3_PLANAR)) {
    LOGE(
        "image type of pstSrc2 should be one of "
        "(IVE_IMAGE_TYPE_S8C1,IVE_IMAGE_TYPE_S8C3_PLANAR)\n");
    return CVI_FAILURE;
  }

  if (!IsValidImageType(pstAlpha, STRFY(pstAlpha), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_U8C3_PLANAR,
                        IVE_IMAGE_TYPE_YUV420P)) {
    LOGE(
        "image type of pstDst should be one of (IVE_IMAGE_TYPE_U8C1, "
        "IVE_IMAGE_TYPE_U8C3_PLANAR)\n");
    return CVI_FAILURE;
  }

  if (!IsValidImageType(pstDst, STRFY(pstDst), IVE_IMAGE_TYPE_S8C1, IVE_IMAGE_TYPE_S8C3_PLANAR)) {
    LOGE("image type of pstDst should be one of IVE_IMAGE_TYPE_S8C1,IVE_IMAGE_TYPE_S8C3_PLANAR)\n");
    return CVI_FAILURE;
  }

  if ((pstDst->enType != pstSrc1->enType) || (pstDst->enType != pstSrc2->enType)) {
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
  // handle_ctx->t_h.t_blend_pixel.set_right_shift_bit(0);
  handle_ctx->t_h.t_blend_pixel.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
  ret = handle_ctx->t_h.t_blend_pixel.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs,
                                          outputs);
  return ret;
}

CVI_S32 CVI_IVE_Blend_Pixel_U8_AB(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1,
                                  IVE_SRC_IMAGE_S *pstSrc2, IVE_SRC_IMAGE_S *pstWa,
                                  IVE_SRC_IMAGE_S *pstWb, IVE_DST_IMAGE_S *pstDst) {
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
  if (!IsValidImageType(pstWa, STRFY(pstWa), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_U8C3_PLANAR,
                        IVE_IMAGE_TYPE_YUV420P)) {
    LOGE(
        "image type of pstDst should be one of (IVE_IMAGE_TYPE_U8C1, "
        "IVE_IMAGE_TYPE_U8C3_PLANAR)\n");
    return CVI_FAILURE;
  }
  if (!IsValidImageType(pstWb, STRFY(pstWb), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_U8C3_PLANAR,
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
        "IVE_IMAGE_TYPE_U8C3_PLANAR)\n");
    return CVI_FAILURE;
  }

  if ((pstDst->enType != pstSrc1->enType) || (pstDst->enType != pstSrc2->enType)) {
    LOGE("source1/source2/dst image pixel format do not match,%d,%d,%d!\n", pstSrc1->enType,
         pstSrc2->enType, pstDst->enType);
    return CVI_FAILURE;
  }

  int ret = CVI_FAILURE;
  IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);

  std::shared_ptr<CviImg> cpp_src1;
  std::shared_ptr<CviImg> cpp_src2;
  std::shared_ptr<CviImg> cpp_alpha;
  std::shared_ptr<CviImg> cpp_alphb;
  std::shared_ptr<CviImg> cpp_dst;

  if (pstDst->enType == IVE_IMAGE_TYPE_YUV420P) {
    // NOTE: Computing tpu slice with different stride in different channel is quite complicated.
    // Instead, we consider YUV420P image as U8C1 with Wx(H + H / 2) image size so that there is
    // only one channel have to blended.
    cpp_src1 = std::shared_ptr<CviImg>(ViewAsU8C1(pstSrc1));
    cpp_src2 = std::shared_ptr<CviImg>(ViewAsU8C1(pstSrc2));
    cpp_alpha = std::shared_ptr<CviImg>(ViewAsU8C1(pstWa));
    cpp_alphb = std::shared_ptr<CviImg>(ViewAsU8C1(pstWb));
    cpp_dst = std::shared_ptr<CviImg>(ViewAsU8C1(pstDst));
  } else {
    cpp_src1 =
        std::shared_ptr<CviImg>(reinterpret_cast<CviImg *>(pstSrc1->tpu_block), [](CviImg *) {});
    cpp_src2 =
        std::shared_ptr<CviImg>(reinterpret_cast<CviImg *>(pstSrc2->tpu_block), [](CviImg *) {});
    cpp_alpha =
        std::shared_ptr<CviImg>(reinterpret_cast<CviImg *>(pstWa->tpu_block), [](CviImg *) {});
    cpp_alphb =
        std::shared_ptr<CviImg>(reinterpret_cast<CviImg *>(pstWb->tpu_block), [](CviImg *) {});
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
      (cpp_alphb->GetImgWidth() != cpp_alpha->GetImgWidth()) ||
      (cpp_alphb->GetImgHeight() != cpp_alpha->GetImgHeight()) ||
      (cpp_src1->GetImgChannel() != cpp_src2->GetImgChannel()) ||
      (cpp_src1->GetImgChannel() != cpp_alpha->GetImgChannel()) ||
      (cpp_src1->GetImgChannel() != cpp_dst->GetImgChannel())) {
    LOGE("source1/source2/alpha/dst image size do not match!\n");
    return CVI_FAILURE;
  }

  std::vector<CviImg *> inputs = {cpp_src1.get(), cpp_src2.get(), cpp_alpha.get(), cpp_alphb.get()};
  std::vector<CviImg *> outputs = {cpp_dst.get()};
  // handle_ctx->t_h.t_blend_pixel.set_right_shift_bit(0);
  handle_ctx->t_h.t_blend_pixel_ab.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
  ret = handle_ctx->t_h.t_blend_pixel_ab.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs,
                                             outputs);
  return ret;
}
CVI_S32 CVI_IVE_And(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1, IVE_SRC_IMAGE_S *pstSrc2,
                    IVE_DST_IMAGE_S *pstDst, bool bInstant) {
  ScopedTrace t(__PRETTY_FUNCTION__);
  if (!IsValidImageType(pstSrc1, STRFY(pstSrc1), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_U8C3_PLANAR)) {
    return CVI_FAILURE;
  }
  if (!IsValidImageType(pstSrc2, STRFY(pstSrc2), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_U8C3_PLANAR)) {
    return CVI_FAILURE;
  }
  if (!IsValidImageType(pstDst, STRFY(pstDst), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_U8C3_PLANAR)) {
    return CVI_FAILURE;
  }

  IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);
  handle_ctx->t_h.t_and.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
  CviImg *cpp_src1 = reinterpret_cast<CviImg *>(pstSrc1->tpu_block);
  CviImg *cpp_src2 = reinterpret_cast<CviImg *>(pstSrc2->tpu_block);
  CviImg *cpp_dst = reinterpret_cast<CviImg *>(pstDst->tpu_block);
  std::vector<CviImg *> inputs = {cpp_src1, cpp_src2};
  std::vector<CviImg *> outputs = {cpp_dst};

  return handle_ctx->t_h.t_and.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs, outputs);
}

CVI_S32 CVI_IVE_BLOCK(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
                      IVE_BLOCK_CTRL_S *pstBlkCtrl, bool bInstant) {
  ScopedTrace t(__PRETTY_FUNCTION__);
  if (!IsValidImageType(pstSrc, STRFY(pstSrc), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_U8C3_PLANAR,
                        IVE_IMAGE_TYPE_BF16C1)) {
    return CVI_FAILURE;
  }
  if (!IsValidImageType(pstDst, STRFY(pstDst), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_U8C3_PLANAR,
                        IVE_IMAGE_TYPE_BF16C1)) {
    return CVI_FAILURE;
  }

  CVI_U32 u32CellSize = pstBlkCtrl->u32CellSize;
  if (pstDst->u32Width != (pstSrc->u32Width / u32CellSize) ||
      (pstSrc->u32Width % u32CellSize != 0)) {
    LOGE("Dst block width not match! Src: %u, Dst: %u. Cell size :%u\n", pstSrc->u32Width,
         pstDst->u32Width, u32CellSize);
    return CVI_FAILURE;
  }
  if (pstDst->u32Height != (pstSrc->u32Height / u32CellSize) ||
      (pstSrc->u32Height % u32CellSize != 0)) {
    LOGE("Dst block height not match! Src: %u, Dst: %u. Cell size :%u\n", pstSrc->u32Height,
         pstDst->u32Height, u32CellSize);
    return CVI_FAILURE;
  }

  IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);
  CviImg *cpp_src = reinterpret_cast<CviImg *>(pstSrc->tpu_block);
  CviImg *cpp_dst = reinterpret_cast<CviImg *>(pstDst->tpu_block);
  std::vector<CviImg *> inputs = {cpp_src};
  std::vector<CviImg *> outputs = {cpp_dst};
  int ret = CVI_FAILURE;
  if (cpp_src->m_tg.fmt == CVK_FMT_U8 && cpp_dst->m_tg.fmt == CVK_FMT_U8) {
    handle_ctx->t_h.t_block.setScaleNum(pstBlkCtrl->f32ScaleSize);
    handle_ctx->t_h.t_block.setCellSize(u32CellSize, cpp_src->m_tg.shape.c);
    handle_ctx->t_h.t_block.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
    ret = handle_ctx->t_h.t_block.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs, outputs,
                                      true);
  } else {
    handle_ctx->t_h.t_block_bf16.setScaleNum(pstBlkCtrl->f32ScaleSize);
    handle_ctx->t_h.t_block_bf16.setCellSize(u32CellSize, cpp_src->m_tg.shape.c);
    handle_ctx->t_h.t_block_bf16.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
    ret = handle_ctx->t_h.t_block_bf16.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs,
                                           outputs, true);
  }
  return ret;
}

static void release_cviimage(std::vector<CviImg *> &imgs) {
  for (auto pimg : imgs) {
    delete pimg;
  }
}
CVI_S32 CVI_IVE_DOWNSAMPLE_420P(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
                                IVE_DST_IMAGE_S *pstDst, IVE_DOWNSAMPLE_CTRL_S *pstdsCtrl,
                                bool bInstant) {
  if (!IsValidImageType(pstSrc, STRFY(pstSrc), IVE_IMAGE_TYPE_YUV420P)) {
    return CVI_FAILURE;
  }
  if (!IsValidImageType(pstDst, STRFY(pstDst), IVE_IMAGE_TYPE_YUV420P)) {
    return CVI_FAILURE;
  }

  CVI_U32 u32CellSize = pstdsCtrl->u8KnerlSize;
  if (pstDst->u32Width != (pstSrc->u32Width / u32CellSize) ||
      (pstSrc->u32Width % u32CellSize != 0)) {
    LOGE("Dst downsample width not match! Src: %u, Dst: %u. Cell size :%u\n", pstSrc->u32Width,
         pstDst->u32Width, u32CellSize);
    return CVI_FAILURE;
  }
  if (pstDst->u32Height != (pstSrc->u32Height / u32CellSize) ||
      (pstSrc->u32Height % u32CellSize != 0)) {
    LOGE("Dst downsample height not match! Src: %u, Dst: %u. Cell size :%u\n", pstSrc->u32Height,
         pstDst->u32Height, u32CellSize);
    return CVI_FAILURE;
  }

  IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);
  std::vector<CviImg *> inputs = {ExtractYuvPlane(pstSrc, 0)};
  std::vector<CviImg *> outputs = {ExtractYuvPlane(pstDst, 0)};

  int ret = CVI_FAILURE;
  handle_ctx->t_h.t_downsample.setCellSize(u32CellSize, inputs[0]->m_tg.shape.c);
  handle_ctx->t_h.t_downsample.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
  handle_ctx->t_h.t_downsample.set_force_alignment(true);
  ret = handle_ctx->t_h.t_downsample.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs,
                                         outputs, true);
  std::vector<CviImg *> inputs1 = {ExtractYuvPlane(pstSrc, 1), ExtractYuvPlane(pstSrc, 2)};
  std::vector<CviImg *> outputs1 = {ExtractYuvPlane(pstDst, 1), ExtractYuvPlane(pstDst, 2)};

  ret = handle_ctx->t_h.t_downsample.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs1,
                                         outputs1, true);

  release_cviimage(inputs);
  release_cviimage(outputs);
  release_cviimage(inputs1);
  release_cviimage(outputs1);

  return ret;
}

CVI_S32 CVI_IVE_DOWNSAMPLE(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
                           IVE_DOWNSAMPLE_CTRL_S *pstdsCtrl, bool bInstant) {
  ScopedTrace t(__PRETTY_FUNCTION__);
  if ((pstSrc->enType == IVE_IMAGE_TYPE_YUV420P) && (pstDst->enType == IVE_IMAGE_TYPE_YUV420P)) {
    return CVI_IVE_DOWNSAMPLE_420P(pIveHandle, pstSrc, pstDst, pstdsCtrl, bInstant);
  }

  if (!IsValidImageType(pstSrc, STRFY(pstSrc), IVE_IMAGE_TYPE_U8C1)) {
    return CVI_FAILURE;
  }
  if (!IsValidImageType(pstDst, STRFY(pstDst), IVE_IMAGE_TYPE_U8C1)) {
    return CVI_FAILURE;
  }
  CVI_U32 u32CellSize = pstdsCtrl->u8KnerlSize;
  if (pstDst->u32Width != (pstSrc->u32Width / u32CellSize) ||
      (pstSrc->u32Width % u32CellSize != 0)) {
    LOGE("Dst downsample width not match! Src: %u, Dst: %u. Cell size :%u\n", pstSrc->u32Width,
         pstDst->u32Width, u32CellSize);
    return CVI_FAILURE;
  }
  if (pstDst->u32Height != (pstSrc->u32Height / u32CellSize) ||
      (pstSrc->u32Height % u32CellSize != 0)) {
    LOGE("Dst downsample height not match! Src: %u, Dst: %u. Cell size :%u\n", pstSrc->u32Height,
         pstDst->u32Height, u32CellSize);
    return CVI_FAILURE;
  }
  IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);
  CviImg *cpp_src = reinterpret_cast<CviImg *>(pstSrc->tpu_block);
  CviImg *cpp_dst = reinterpret_cast<CviImg *>(pstDst->tpu_block);
  std::vector<CviImg *> inputs = {cpp_src};
  std::vector<CviImg *> outputs = {cpp_dst};

  int ret = CVI_FAILURE;
  handle_ctx->t_h.t_downsample.setCellSize(u32CellSize, cpp_src->m_tg.shape.c);
  handle_ctx->t_h.t_downsample.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
  ret = handle_ctx->t_h.t_downsample.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs,
                                         outputs, true);
  return ret;
}

CVI_S32 CVI_IVE_Dilate(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
                       IVE_DILATE_CTRL_S *pstDilateCtrl, bool bInstant) {
  ScopedTrace t(__PRETTY_FUNCTION__);
  if (!IsValidImageType(pstSrc, STRFY(pstSrc), IVE_IMAGE_TYPE_U8C1)) {
    return CVI_FAILURE;
  }
  if (!IsValidImageType(pstDst, STRFY(pstDst), IVE_IMAGE_TYPE_U8C1)) {
    return CVI_FAILURE;
  }

  IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);
  handle_ctx->t_h.t_filter.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
  CviImg *cpp_src = reinterpret_cast<CviImg *>(pstSrc->tpu_block);
  CviImg *cpp_dst = reinterpret_cast<CviImg *>(pstDst->tpu_block);
  std::vector<CviImg *> inputs = {cpp_src};
  std::vector<CviImg *> outputs = {cpp_dst};

  uint32_t npu_num = handle_ctx->t_h.t_erode.getNpuNum(handle_ctx->cvk_ctx);
  CviImg cimg(handle_ctx->rt_handle, npu_num, 5, 5, CVK_FMT_U8);
  IveKernel kernel;
  kernel.img = cimg;
  kernel.img.GetVAddr();
  for (size_t i = 0; i < npu_num; i++) {
    memcpy(kernel.img.GetVAddr() + i * 25, pstDilateCtrl->au8Mask, 25);
  }
  kernel.multiplier.f = 1.f;
  QuantizeMultiplierSmallerThanOne(kernel.multiplier.f, &kernel.multiplier.base,
                                   &kernel.multiplier.shift);
  handle_ctx->t_h.t_filter.setKernel(kernel);
  int ret =
      handle_ctx->t_h.t_filter.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs, outputs);
  kernel.img.Free(handle_ctx->rt_handle);
  return ret;
}

CVI_S32 CVI_IVE_Erode(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
                      IVE_ERODE_CTRL_S *pstErodeCtrl, bool bInstant) {
  ScopedTrace t(__PRETTY_FUNCTION__);
  if (!IsValidImageType(pstSrc, STRFY(pstSrc), IVE_IMAGE_TYPE_U8C1)) {
    return CVI_FAILURE;
  }
  if (!IsValidImageType(pstDst, STRFY(pstDst), IVE_IMAGE_TYPE_U8C1)) {
    return CVI_FAILURE;
  }

  IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);
  handle_ctx->t_h.t_erode.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
  CviImg *cpp_src = reinterpret_cast<CviImg *>(pstSrc->tpu_block);
  CviImg *cpp_dst = reinterpret_cast<CviImg *>(pstDst->tpu_block);
  std::vector<CviImg *> inputs = {cpp_src};
  std::vector<CviImg *> outputs = {cpp_dst};

  uint32_t npu_num = handle_ctx->t_h.t_erode.getNpuNum(handle_ctx->cvk_ctx);
  CviImg cimg(handle_ctx->rt_handle, npu_num, 5, 5, CVK_FMT_U8);
  IveKernel kernel;
  kernel.img = cimg;
  kernel.img.GetVAddr();
  for (size_t i = 0; i < npu_num; i++) {
    memcpy(kernel.img.GetVAddr() + i * 25, pstErodeCtrl->au8Mask, 25);
  }
  kernel.multiplier.f = 1.f;
  QuantizeMultiplierSmallerThanOne(kernel.multiplier.f, &kernel.multiplier.base,
                                   &kernel.multiplier.shift);
  handle_ctx->t_h.t_erode.setKernel(kernel);
  int ret =
      handle_ctx->t_h.t_erode.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs, outputs);
  kernel.img.Free(handle_ctx->rt_handle);
  return ret;
}

CVI_S32 CVI_IVE_Filter(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
                       IVE_FILTER_CTRL_S *pstFltCtrl, bool bInstant) {
  ScopedTrace t(__PRETTY_FUNCTION__);
  if (pstSrc->enType != pstDst->enType) {
    LOGE("pstSrc & pstDst must have the same type.\n");
    return CVI_FAILURE;
  }
  if (!IsValidImageType(pstSrc, STRFY(pstSrc), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_U8C3_PLANAR)) {
    return CVI_FAILURE;
  }
  if (!IsValidImageType(pstDst, STRFY(pstDst), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_U8C3_PLANAR)) {
    return CVI_FAILURE;
  }

  IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);
  handle_ctx->t_h.t_filter.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
  CviImg *cpp_src = reinterpret_cast<CviImg *>(pstSrc->tpu_block);
  CviImg *cpp_dst = reinterpret_cast<CviImg *>(pstDst->tpu_block);
  std::vector<CviImg *> inputs = {cpp_src};
  std::vector<CviImg *> outputs = {cpp_dst};

  if (pstFltCtrl->u8MaskSize != 3 && pstFltCtrl->u8MaskSize != 5 && pstFltCtrl->u8MaskSize != 13) {
    LOGE("Currently Filter only supports filter size 3, 5, 13.\n");
  }
  uint32_t npu_num = handle_ctx->t_h.t_filter.getNpuNum(handle_ctx->cvk_ctx);
  CviImg cimg(handle_ctx->rt_handle, npu_num, pstFltCtrl->u8MaskSize, pstFltCtrl->u8MaskSize,
              CVK_FMT_I8);
  IveKernel kernel;
  kernel.img = cimg;
  kernel.img.GetVAddr();
  int mask_length = pstFltCtrl->u8MaskSize * pstFltCtrl->u8MaskSize;
  for (size_t i = 0; i < npu_num; i++) {
    memcpy((int8_t *)(kernel.img.GetVAddr() + i * mask_length), pstFltCtrl->as8Mask, mask_length);
  }
  kernel.img.Flush(handle_ctx->rt_handle);
  kernel.multiplier.f = 1.f / pstFltCtrl->u32Norm;
  QuantizeMultiplierSmallerThanOne(kernel.multiplier.f, &kernel.multiplier.base,
                                   &kernel.multiplier.shift);
  handle_ctx->t_h.t_filter.setKernel(kernel);
  int ret =
      handle_ctx->t_h.t_filter.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs, outputs);
  kernel.img.Free(handle_ctx->rt_handle);
  return ret;
}

inline bool get_hog_feature_info(uint16_t width, uint16_t height, uint16_t u32CellSize,
                                 uint16_t blk_size, uint32_t *width_cell, uint32_t *height_cell,
                                 uint32_t *width_block, uint32_t *height_block) {
  *height_cell = (uint32_t)height / u32CellSize;
  *width_cell = (uint32_t)width / u32CellSize;
  if (*height_cell < blk_size || *width_cell < blk_size) {
    return false;
  }
  *height_block = (*height_cell - blk_size) + 1;
  *width_block = (*width_cell - blk_size) + 1;
  return true;
}

CVI_S32 CVI_IVE_GET_HOG_SIZE(CVI_U32 u32Width, CVI_U32 u32Height, CVI_U8 u8BinSize,
                             CVI_U16 u16CellSize, CVI_U16 u16BlkSizeInCell, CVI_U16 u16BlkStepX,
                             CVI_U16 u16BlkStepY, CVI_U32 *u32HogSize) {
  if (u16BlkStepX == 0) {
    LOGE("u16BlkStepX cannot be 0.\n");
    return CVI_FAILURE;
  }
  if (u16BlkStepY == 0) {
    LOGE("u16BlkStepY cannot be 0.\n");
    return CVI_FAILURE;
  }
  uint32_t height_cell = 0, width_cell = 0, height_block = 0, width_block = 0;
  if (!get_hog_feature_info(u32Width, u32Height, u16CellSize, u16BlkSizeInCell, &width_cell,
                            &height_cell, &width_block, &height_block)) {
    LOGE("Block size exceed cell block.\n");
    return CVI_FAILURE;
  }
  uint32_t block_length = u16BlkSizeInCell * u16BlkSizeInCell;
  width_block = (width_block - 1) / u16BlkStepX + 1;
  height_block = (height_block - 1) / u16BlkStepY + 1;
  uint32_t num_of_block_data = height_block * width_block;
  *u32HogSize = num_of_block_data * (block_length * u8BinSize) * sizeof(float);
  return CVI_SUCCESS;
}

CVI_S32 CVI_IVE_HOG(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDstH,
                    IVE_DST_IMAGE_S *pstDstV, IVE_DST_IMAGE_S *pstDstMag,
                    IVE_DST_IMAGE_S *pstDstAng, IVE_DST_MEM_INFO_S *pstDstHist,
                    IVE_HOG_CTRL_S *pstHogCtrl, bool bInstant) {
#ifndef CV180X
  ScopedTrace t(__PRETTY_FUNCTION__);
  // No need to check here. Will check later.
  if (pstDstAng->u32Width % pstHogCtrl->u32CellSize != 0) {
    LOGE("Width %u is not divisible by %u.\n", pstDstAng->u32Width, pstHogCtrl->u32CellSize);
    return CVI_FAILURE;
  }
  if (pstDstAng->u32Height % pstHogCtrl->u32CellSize != 0) {
    LOGE("Height %u is not divisible by %u.\n", pstDstAng->u32Height, pstHogCtrl->u32CellSize);
    return CVI_FAILURE;
  }
  if (pstHogCtrl->u16BlkStepX == 0) {
    LOGE("u16BlkStepX cannot be 0.\n");
    return CVI_FAILURE;
  }
  if (pstHogCtrl->u16BlkStepY == 0) {
    LOGE("u16BlkStepY cannot be 0.\n");
    return CVI_FAILURE;
  }
  uint32_t height_cell = 0, width_cell = 0, height_block = 0, width_block = 0;
  if (!get_hog_feature_info(pstDstAng->u32Width, pstDstAng->u32Height, pstHogCtrl->u32CellSize,
                            pstHogCtrl->u16BlkSizeInCell, &width_cell, &height_cell, &width_block,
                            &height_block)) {
    LOGE("Block size exceed cell block.\n");
    return CVI_FAILURE;
  }
  // uint32_t &&cell_length = pstHogCtrl->u32CellSize * pstHogCtrl->u32CellSize;
  uint32_t &&cell_hist_length = height_cell * width_cell * pstHogCtrl->u8BinSize;
  uint32_t &&block_length = pstHogCtrl->u16BlkSizeInCell * pstHogCtrl->u16BlkSizeInCell;
  uint32_t &&num_of_block_data = ((height_block - 1) / pstHogCtrl->u16BlkStepY + 1) *
                                 ((width_block - 1) / pstHogCtrl->u16BlkStepX + 1);
  uint32_t &&hog_hist_length = num_of_block_data * (block_length * pstHogCtrl->u8BinSize);
  uint32_t &&hog_hist_size = hog_hist_length * sizeof(float);
  if (pstDstHist->u32ByteSize != hog_hist_size) {
    LOGE("Histogram size mismatch! Given: %u, required: %u ( %u * sizeof(uint32_t)).\n",
         pstDstHist->u32ByteSize, hog_hist_size, hog_hist_length);
    return CVI_FAILURE;
  }

  IVE_SOBEL_CTRL_S iveSblCtrl;
  iveSblCtrl.enOutCtrl = IVE_SOBEL_OUT_CTRL_BOTH;
  iveSblCtrl.u8MaskSize = 1;
  if (CVI_IVE_Sobel(pIveHandle, pstSrc, pstDstH, pstDstV, &iveSblCtrl, 0) != CVI_SUCCESS) {
    return CVI_FAILURE;
  }
  IVE_MAG_AND_ANG_CTRL_S iveMaaCtrl;
  iveMaaCtrl.enOutCtrl = IVE_MAG_AND_ANG_OUT_CTRL_MAG_AND_ANG;
  iveMaaCtrl.enDistCtrl = IVE_MAG_DIST_L2;
  if (CVI_IVE_MagAndAng(pIveHandle, pstDstH, pstDstV, pstDstMag, pstDstAng, &iveMaaCtrl, 0) !=
      CVI_SUCCESS) {
    return CVI_FAILURE;
  }

  // Get Histogram here.
  Tracer::TraceBegin("CPU histogram");
  Tracer::TraceBegin("Generate cell histogram");
  CVI_IVE_BufRequest(pIveHandle, pstDstAng);
  CVI_IVE_BufRequest(pIveHandle, pstDstMag);
  uint16_t *ang_ptr = (uint16_t *)pstDstAng->pu8VirAddr[0];
  uint16_t *mag_ptr = (uint16_t *)pstDstMag->pu8VirAddr[0];
  float div = 180 / pstHogCtrl->u8BinSize;
  float *cell_histogram = new float[cell_hist_length];
  memset(cell_histogram, 0, cell_hist_length * sizeof(float));
  // Do Add & DIV while creating histogram. Slow.
  auto &&u32Height_i = (uint32_t)(pstDstAng->u32Height - 1);
  auto &&u32Width_j = (uint32_t)(pstDstAng->u32Width - 1);
  for (uint32_t i = 1; i < u32Height_i; i++) {
    uint32_t &&row_skip = pstDstAng->u16Stride[0] * i;
    uint32_t &&cell_row_skip = (i / pstHogCtrl->u32CellSize) * width_cell;
    for (uint32_t j = 1; j < u32Width_j; j++) {
      uint32_t &&cell_index = (uint32_t)(cell_row_skip + (uint32_t)(j / pstHogCtrl->u32CellSize)) *
                              pstHogCtrl->u8BinSize;
      uint32_t degree = std::abs(convert_bf16_fp32(ang_ptr[j + row_skip]));
      uint32_t mag = convert_bf16_fp32(mag_ptr[j + row_skip]);
      float bin_div = degree / div;
      float bin_div_dec = bin_div - (uint32_t)(bin_div);
      if (bin_div_dec == 0) {
        uint32_t bin_index = bin_div;
        if (bin_index == pstHogCtrl->u8BinSize) {
          bin_index = 0;
        }
        cell_histogram[cell_index + bin_index] += mag;
      } else {
        uint32_t bin_index = bin_div;
        if (bin_index == pstHogCtrl->u8BinSize) {
          bin_index = 0;
        }
        uint32_t bin_index_2 = (bin_index + 1);
        if (bin_index_2 >= pstHogCtrl->u8BinSize) bin_index_2 = 0;
        float bin_div_dec_left = 1.f - bin_div_dec;
        cell_histogram[cell_index + bin_index] += (mag * bin_div_dec_left);
        cell_histogram[cell_index + bin_index_2] += (mag * bin_div_dec);
      }
    }
  }
  Tracer::TraceEnd();

  Tracer::TraceBegin("Generate HOG histogram");
  uint32_t &&copy_length = pstHogCtrl->u8BinSize * pstHogCtrl->u16BlkSizeInCell;
  uint32_t &&copy_data_length = copy_length * sizeof(float);
  float *hog_ptr = (float *)pstDstHist->pu8VirAddr;
  memset(hog_ptr, 0, pstDstHist->u32ByteSize);
  uint32_t count = 0;
  for (uint32_t i = 0; i < height_block; i += pstHogCtrl->u16BlkStepY) {
    uint32_t &&row_skip = i * width_block;
    for (uint32_t j = 0; j < width_block; j += pstHogCtrl->u16BlkStepX) {
      uint32_t &&skip = j + row_skip;
      for (uint32_t k = 0; k < pstHogCtrl->u16BlkSizeInCell; k++) {
        uint32_t &&index = skip + k * width_block;
        auto *cell_hist_ptr = cell_histogram + index;
        auto *dst_hog_ptr = hog_ptr + count;
        memcpy(dst_hog_ptr, cell_hist_ptr, copy_data_length);
        count += copy_length;
      }
    }
  }
  delete[] cell_histogram;
  if (count != hog_hist_length) {
    LOGE("Hog histogram not aligned. %u %u.\n", count, hog_hist_length);
    return CVI_FAILURE;
  }
  Tracer::TraceEnd();
  Tracer::TraceBegin("Normalizing HOG histogram");
  hog_ptr = (float *)pstDstHist->pu8VirAddr;
  uint32_t &&block_data_length = block_length * pstHogCtrl->u8BinSize;
  uint32_t nums_of_block_feature = hog_hist_length / block_data_length;
#ifdef __ARM_ARCH_7A__
  const uint32_t neon_turn = 0;
#else
  uint32_t neon_turn = block_data_length / 4;
#endif
  uint32_t neon_turn_left = neon_turn * 4;
  for (uint32_t i = 0; i < nums_of_block_feature; i++) {
    float count_total = 0;
    auto &&skip_i = i * block_data_length;
    float *block_head = hog_ptr + skip_i;
#ifndef __ARM_ARCH_7A__
    for (uint32_t j = 0; j < neon_turn; j++) {
      float32x4_t f = vld1q_f32(block_head);
      float32x4_t result = vmulq_f32(f, f);
      count_total += vaddvq_f32(result);
      block_head += 4;
    }
#endif
    for (uint32_t j = neon_turn_left; j < block_data_length; j++) {
      count_total += hog_ptr[skip_i + j] * hog_ptr[skip_i + j];
    }
    float count = count_total == 0 ? 0 : 1.f / sqrt(count_total);
    float32x4_t m = vdupq_n_f32(count);
    block_head = hog_ptr + skip_i;
    for (uint32_t j = 0; j < neon_turn; j++) {
      float32x4_t f = vld1q_f32(block_head);
      float32x4_t result = vmulq_f32(m, f);
      vst1q_f32(block_head, result);
      block_head += 4;
    }
    for (uint32_t j = neon_turn_left; j < block_data_length; j++) {
      hog_ptr[skip_i + j] *= count;
    }
  }
  Tracer::TraceEnd();
  Tracer::TraceEnd();
  return CVI_SUCCESS;
#else
  return CVI_FAILURE;
#endif
}

CVI_S32 CVI_IVE_MagAndAng(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrcH, IVE_SRC_IMAGE_S *pstSrcV,
                          IVE_DST_IMAGE_S *pstDstMag, IVE_DST_IMAGE_S *pstDstAng,
                          IVE_MAG_AND_ANG_CTRL_S *pstMaaCtrl, bool bInstant) {
  ScopedTrace t(__PRETTY_FUNCTION__);
  if (!IsValidImageType(pstSrcH, STRFY(pstSrcH), IVE_IMAGE_TYPE_BF16C1)) {
    return CVI_FAILURE;
  }
  if (!IsValidImageType(pstSrcV, STRFY(pstSrcV), IVE_IMAGE_TYPE_BF16C1)) {
    return CVI_FAILURE;
  }
  IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);
  handle_ctx->t_h.t_magandang.setTblMgr(&handle_ctx->t_h.t_tblmgr);
  CviImg *cpp_src1 = reinterpret_cast<CviImg *>(pstSrcH->tpu_block);
  CviImg *cpp_src2 = reinterpret_cast<CviImg *>(pstSrcV->tpu_block);
  CviImg *cpp_dst = nullptr, *cpp_dst2 = nullptr;
  std::vector<CviImg *> inputs = {cpp_src1, cpp_src2};
  std::vector<CviImg *> outputs;
  switch (pstMaaCtrl->enOutCtrl) {
    case IVE_MAG_AND_ANG_OUT_CTRL_MAG: {
      if (!IsValidImageType(pstDstMag, STRFY(pstDstMag), IVE_IMAGE_TYPE_BF16C1)) {
        return CVI_FAILURE;
      }
      cpp_dst = reinterpret_cast<CviImg *>(pstDstMag->tpu_block);
      handle_ctx->t_h.t_magandang.exportOption(true, false);
      outputs.emplace_back(cpp_dst);
    } break;
    case IVE_MAG_AND_ANG_OUT_CTRL_ANG: {
      if (!IsValidImageType(pstDstAng, STRFY(pstDstAng), IVE_IMAGE_TYPE_BF16C1)) {
        return CVI_FAILURE;
      }
      cpp_dst = reinterpret_cast<CviImg *>(pstDstAng->tpu_block);
      handle_ctx->t_h.t_magandang.exportOption(false, true);
      outputs.emplace_back(cpp_dst);
    } break;
    case IVE_MAG_AND_ANG_OUT_CTRL_MAG_AND_ANG: {
      if (!IsValidImageType(pstDstMag, STRFY(pstDstMag), IVE_IMAGE_TYPE_BF16C1)) {
        return CVI_FAILURE;
      }
      if (!IsValidImageType(pstDstAng, STRFY(pstDstAng), IVE_IMAGE_TYPE_BF16C1)) {
        return CVI_FAILURE;
      }
      handle_ctx->t_h.t_magandang.exportOption(true, true);
      cpp_dst = reinterpret_cast<CviImg *>(pstDstMag->tpu_block);
      cpp_dst2 = reinterpret_cast<CviImg *>(pstDstAng->tpu_block);
      outputs.emplace_back(cpp_dst);
      outputs.emplace_back(cpp_dst2);
    } break;
    default:
      LOGE("Not supported Mag and Angle type.\n");
      return CVI_FAILURE;
      break;
  }
  // True accuracy too low.
  handle_ctx->t_h.t_magandang.noNegative(false);
  handle_ctx->t_h.t_magandang.magDistMethod(pstMaaCtrl->enDistCtrl);
  handle_ctx->t_h.t_magandang.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
  return handle_ctx->t_h.t_magandang.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs,
                                         outputs);
}

CVI_S32 CVI_IVE_Map(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_MEM_INFO_S *pstMap,
                    IVE_DST_IMAGE_S *pstDst, bool bInstant) {
#ifndef CV180X
  ScopedTrace t(__PRETTY_FUNCTION__);
  if (!IsValidImageType(pstSrc, STRFY(pstSrc), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_U16C1)) {
    return CVI_FAILURE;
  }
  if (!IsValidImageType(pstDst, STRFY(pstDst), IVE_IMAGE_TYPE_U8C1)) {
    return CVI_FAILURE;
  }

  if (pstMap->u32ByteSize == 512) {
    IVE_IMAGE_S table_index, lookup_index;
    CVI_IVE_CreateImage(pIveHandle, &table_index, IVE_IMAGE_TYPE_U8C1, pstSrc->u32Width,
                        pstSrc->u32Height);
    CVI_IVE_CreateImage(pIveHandle, &lookup_index, IVE_IMAGE_TYPE_U8C1, pstSrc->u32Width,
                        pstSrc->u32Height);
    neonU16SeperateU8((uint16_t *)pstSrc->pu8VirAddr[0], table_index.pu8VirAddr[0],
                      lookup_index.pu8VirAddr[0], (pstSrc->u16Stride[0] / 2) * pstSrc->u32Height);

    CVI_IVE_BufFlush(pIveHandle, &table_index);
    CVI_IVE_BufFlush(pIveHandle, &lookup_index);

    IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);
    CviImg *cpp_src_index = reinterpret_cast<CviImg *>(table_index.tpu_block);
    CviImg *cpp_src = reinterpret_cast<CviImg *>(lookup_index.tpu_block);
    CviImg *cpp_dst = reinterpret_cast<CviImg *>(pstDst->tpu_block);
    std::vector<CviImg *> inputs = {cpp_src_index, cpp_src};
    std::vector<CviImg *> outputs = {cpp_dst};
    handle_ctx->t_h.t_tbl512.setTable(handle_ctx->rt_handle, &handle_ctx->t_h.t_tblmgr,
                                      pstMap->pu8VirAddr);
    handle_ctx->t_h.t_tbl512.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
    CVI_S32 ret =
        handle_ctx->t_h.t_tbl512.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs, outputs);

    CVI_SYS_FreeI(pIveHandle, &table_index);
    CVI_SYS_FreeI(pIveHandle, &lookup_index);

    return ret;
  } else {
    IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);
    auto &shape = handle_ctx->t_h.t_tblmgr.getTblTLShape(CVK_FMT_U8);
    uint32_t tbl_sz = shape.h * shape.w;
    if (pstMap->u32ByteSize != tbl_sz) {
      LOGE("Mapping table must be size %u in CVI_U8 format.\n", tbl_sz);
      return CVI_FAILURE;
    }
    CviImg *cpp_src = reinterpret_cast<CviImg *>(pstSrc->tpu_block);
    CviImg *cpp_dst = reinterpret_cast<CviImg *>(pstDst->tpu_block);
    std::vector<CviImg *> inputs = {cpp_src};
    std::vector<CviImg *> outputs = {cpp_dst};
    handle_ctx->t_h.t_tbl.setTable(handle_ctx->rt_handle, &handle_ctx->t_h.t_tblmgr,
                                   pstMap->pu8VirAddr);
    handle_ctx->t_h.t_tbl.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
    return handle_ctx->t_h.t_tbl.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs, outputs);
  }
#else
  return CVI_FAILURE;
#endif
}

CVI_S32 CVI_IVE_Mask(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1, IVE_SRC_IMAGE_S *pstSrc2,
                     IVE_SRC_IMAGE_S *pstMask, IVE_DST_IMAGE_S *pstDst, bool bInstant) {
  ScopedTrace t(__PRETTY_FUNCTION__);
  if (!IsValidImageType(pstSrc1, STRFY(pstSrc1), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_U8C3_PLANAR)) {
    return CVI_FAILURE;
  }
  if (!IsValidImageType(pstSrc2, STRFY(pstSrc2), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_U8C3_PLANAR)) {
    return CVI_FAILURE;
  }
  if (!IsValidImageType(pstMask, STRFY(pstMask), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_U8C3_PLANAR)) {
    return CVI_FAILURE;
  }
  if (!IsValidImageType(pstDst, STRFY(pstDst), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_U8C3_PLANAR)) {
    return CVI_FAILURE;
  }

  int ret = CVI_FAILURE;
  IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);
  CviImg *cpp_src1 = reinterpret_cast<CviImg *>(pstSrc1->tpu_block);
  CviImg *cpp_src2 = reinterpret_cast<CviImg *>(pstSrc2->tpu_block);
  CviImg *cpp_mask = reinterpret_cast<CviImg *>(pstMask->tpu_block);
  CviImg *cpp_dst = reinterpret_cast<CviImg *>(pstDst->tpu_block);
  std::vector<CviImg *> inputs = {cpp_src1, cpp_src2, cpp_mask};
  std::vector<CviImg *> outputs = {cpp_dst};

  handle_ctx->t_h.t_mask.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
  ret = handle_ctx->t_h.t_mask.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs, outputs);

  return ret;
}

CVI_S32 CVI_IVE_MulSum(IVE_HANDLE pIveHandle, IVE_IMAGE_S *pstImg, double *sum, bool bInstant) {
  ScopedTrace t(__PRETTY_FUNCTION__);
  if (!IsValidImageType(pstImg, STRFY(pstImg), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_BF16C1)) {
    return CVI_FAILURE;
  }

  IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);
  CviImg *cpp_src = reinterpret_cast<CviImg *>(pstImg->tpu_block);
  std::vector<CviImg *> inputs = {cpp_src};
  std::vector<CviImg *> outputs;
  handle_ctx->t_h.t_mulsum.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
  int ret =
      handle_ctx->t_h.t_mulsum.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs, outputs);
  *sum = handle_ctx->t_h.t_mulsum.getSum();
  return ret;
}

CVI_S32 CVI_IVE_NormGrad(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDstH,
                         IVE_DST_IMAGE_S *pstDstV, IVE_DST_IMAGE_S *pstDstHV,
                         IVE_NORM_GRAD_CTRL_S *pstNormGradCtrl, bool bInstant) {
  ScopedTrace t(__PRETTY_FUNCTION__);
  int kernel_size = pstNormGradCtrl->u8MaskSize;
  if (kernel_size != 1 && kernel_size != 3) {
    LOGE("Kernel size currently only supports 1 and 3.\n");
    return CVI_FAILURE;
  }
  if (!IsValidImageType(pstSrc, STRFY(pstSrc), IVE_IMAGE_TYPE_U8C1)) {
    return CVI_FAILURE;
  }

  IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);
  int npu_num = handle_ctx->t_h.t_sobel_gradonly.getNpuNum(handle_ctx->cvk_ctx);
  CviImg *cpp_src = reinterpret_cast<CviImg *>(pstSrc->tpu_block);
  std::vector<CviImg *> inputs = {cpp_src};
  std::vector<CviImg *> outputs;
  bool do_free = false;
  int ret = CVI_FAILURE;
  if (pstNormGradCtrl->enOutCtrl == IVE_NORM_GRAD_OUT_CTRL_HOR_AND_VER) {
    if (!IsValidImageType(pstDstH, STRFY(pstDstH), IVE_IMAGE_TYPE_S16C1, IVE_IMAGE_TYPE_U8C1)) {
      return CVI_FAILURE;
    }
    if (!IsValidImageType(pstDstV, STRFY(pstDstV), IVE_IMAGE_TYPE_S16C1, IVE_IMAGE_TYPE_U8C1)) {
      return CVI_FAILURE;
    }

    IVE_IMAGE_S dstH_BF16, dstV_BF16;
    CVI_IVE_CreateImage(pIveHandle, &dstH_BF16, IVE_IMAGE_TYPE_BF16C1, pstSrc->u32Width,
                        pstSrc->u32Height);
    CVI_IVE_CreateImage(pIveHandle, &dstV_BF16, IVE_IMAGE_TYPE_BF16C1, pstSrc->u32Width,
                        pstSrc->u32Height);
    CviImg *cpp_dstv = reinterpret_cast<CviImg *>(dstV_BF16.tpu_block);
    CviImg *cpp_dsth = reinterpret_cast<CviImg *>(dstH_BF16.tpu_block);
    outputs.emplace_back(cpp_dstv);
    outputs.emplace_back(cpp_dsth);
    IveKernel kernel_w =
        createKernel(handle_ctx->rt_handle, npu_num, kernel_size, kernel_size, IVE_KERNEL::SOBEL_X);
    IveKernel kernel_h =
        createKernel(handle_ctx->rt_handle, npu_num, kernel_size, kernel_size, IVE_KERNEL::SOBEL_Y);
    handle_ctx->t_h.t_sobel_gradonly.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
    handle_ctx->t_h.t_sobel_gradonly.setKernel(kernel_w, kernel_h);
    ret = handle_ctx->t_h.t_sobel_gradonly.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs,
                                               outputs);
    kernel_w.img.Free(handle_ctx->rt_handle);
    kernel_h.img.Free(handle_ctx->rt_handle);
    IVE_ITC_CRTL_S iveItcCtrl;
    iveItcCtrl.enType = pstNormGradCtrl->enITCType;
    ret |= CVI_IVE_ImageTypeConvert(pIveHandle, &dstV_BF16, pstDstV, &iveItcCtrl, 0);
    ret |= CVI_IVE_ImageTypeConvert(pIveHandle, &dstH_BF16, pstDstH, &iveItcCtrl, 0);
    ret |= CVI_SYS_FreeI(pIveHandle, &dstV_BF16);
    ret |= CVI_SYS_FreeI(pIveHandle, &dstH_BF16);
  } else if (pstNormGradCtrl->enOutCtrl == IVE_NORM_GRAD_OUT_CTRL_HOR) {
    if (!IsValidImageType(pstDstH, STRFY(pstDstH), IVE_IMAGE_TYPE_S16C1, IVE_IMAGE_TYPE_U8C1)) {
      return CVI_FAILURE;
    }

    IVE_IMAGE_S dst_BF16;
    if (pstDstH->enType == IVE_IMAGE_TYPE_U16C1 ||
        pstNormGradCtrl->enITCType == IVE_ITC_NORMALIZE) {
      CVI_IVE_CreateImage(pIveHandle, &dst_BF16, IVE_IMAGE_TYPE_BF16C1, pstSrc->u32Width,
                          pstSrc->u32Height);
      CviImg *cpp_dsth = reinterpret_cast<CviImg *>(dst_BF16.tpu_block);
      outputs.emplace_back(cpp_dsth);
      do_free = true;
    } else {
      CviImg *cpp_dsth = reinterpret_cast<CviImg *>(pstDstH->tpu_block);
      outputs.emplace_back(cpp_dsth);
    }
    IveKernel kernel_h =
        createKernel(handle_ctx->rt_handle, npu_num, kernel_size, kernel_size, IVE_KERNEL::SOBEL_Y);
    handle_ctx->t_h.t_filter_bf16.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
    handle_ctx->t_h.t_filter_bf16.setKernel(kernel_h);
    ret = handle_ctx->t_h.t_filter_bf16.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs,
                                            outputs);
    kernel_h.img.Free(handle_ctx->rt_handle);
    if (do_free) {
      IVE_ITC_CRTL_S iveItcCtrl;
      iveItcCtrl.enType = pstNormGradCtrl->enITCType;
      ret |= CVI_IVE_ImageTypeConvert(pIveHandle, &dst_BF16, pstDstH, &iveItcCtrl, 0);
      ret |= CVI_SYS_FreeI(pIveHandle, &dst_BF16);
    }
  } else if (pstNormGradCtrl->enOutCtrl == IVE_NORM_GRAD_OUT_CTRL_VER) {
    if (!IsValidImageType(pstDstV, STRFY(pstDstV), IVE_IMAGE_TYPE_S16C1, IVE_IMAGE_TYPE_U8C1)) {
      return CVI_FAILURE;
    }

    IVE_IMAGE_S dst_BF16;
    if (pstDstV->enType == IVE_IMAGE_TYPE_U16C1 ||
        pstNormGradCtrl->enITCType == IVE_ITC_NORMALIZE) {
      CVI_IVE_CreateImage(pIveHandle, &dst_BF16, IVE_IMAGE_TYPE_BF16C1, pstSrc->u32Width,
                          pstSrc->u32Height);
      CviImg *cpp_dstv = reinterpret_cast<CviImg *>(dst_BF16.tpu_block);
      outputs.emplace_back(cpp_dstv);
      do_free = true;
    } else {
      CviImg *cpp_dstv = reinterpret_cast<CviImg *>(pstDstV->tpu_block);
      outputs.emplace_back(cpp_dstv);
    }
    IveKernel kernel_w =
        createKernel(handle_ctx->rt_handle, npu_num, kernel_size, kernel_size, IVE_KERNEL::SOBEL_X);
    handle_ctx->t_h.t_filter_bf16.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
    handle_ctx->t_h.t_filter_bf16.setKernel(kernel_w);
    ret = handle_ctx->t_h.t_filter_bf16.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs,
                                            outputs);
    kernel_w.img.Free(handle_ctx->rt_handle);
    if (do_free) {
      IVE_ITC_CRTL_S iveItcCtrl;
      iveItcCtrl.enType = pstNormGradCtrl->enITCType;
      ret |= CVI_IVE_ImageTypeConvert(pIveHandle, &dst_BF16, pstDstV, &iveItcCtrl, 0);
      ret |= CVI_SYS_FreeI(pIveHandle, &dst_BF16);
    }
  } else if (pstNormGradCtrl->enOutCtrl == IVE_NORM_GRAD_OUT_CTRL_COMBINE) {
    if (!IsValidImageType(pstDstHV, STRFY(pstDstHV), IVE_IMAGE_TYPE_U16C1, IVE_IMAGE_TYPE_U8C1)) {
      return CVI_FAILURE;
    }

    IVE_IMAGE_S dst_BF16;
    if (pstDstHV->enType == IVE_IMAGE_TYPE_U16C1 ||
        pstNormGradCtrl->enITCType == IVE_ITC_NORMALIZE) {
      CVI_IVE_CreateImage(pIveHandle, &dst_BF16, IVE_IMAGE_TYPE_BF16C1, pstSrc->u32Width,
                          pstSrc->u32Height);
      CviImg *cpp_dsthv = reinterpret_cast<CviImg *>(dst_BF16.tpu_block);
      outputs.emplace_back(cpp_dsthv);
      do_free = true;
    } else {
      CviImg *cpp_dsthv = reinterpret_cast<CviImg *>(pstDstHV->tpu_block);
      outputs.emplace_back(cpp_dsthv);
    }
    IveKernel kernel_w =
        createKernel(handle_ctx->rt_handle, npu_num, kernel_size, kernel_size, IVE_KERNEL::SOBEL_X);
    IveKernel kernel_h =
        createKernel(handle_ctx->rt_handle, npu_num, kernel_size, kernel_size, IVE_KERNEL::SOBEL_Y);

    handle_ctx->t_h.t_sobel.setTblMgr(&handle_ctx->t_h.t_tblmgr);
    handle_ctx->t_h.t_sobel.magDistMethod(pstNormGradCtrl->enDistCtrl);
    handle_ctx->t_h.t_sobel.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
    handle_ctx->t_h.t_sobel.setKernel(kernel_w, kernel_h);
    ret = handle_ctx->t_h.t_sobel.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs, outputs);
    kernel_w.img.Free(handle_ctx->rt_handle);
    kernel_h.img.Free(handle_ctx->rt_handle);
    if (do_free) {
      IVE_ITC_CRTL_S iveItcCtrl;
      iveItcCtrl.enType = pstNormGradCtrl->enITCType;
      ret |= CVI_IVE_ImageTypeConvert(pIveHandle, &dst_BF16, pstDstHV, &iveItcCtrl, 0);
      ret |= CVI_SYS_FreeI(pIveHandle, &dst_BF16);
    }
  } else {
    return ret;
  }
  return ret;
}

CVI_S32 CVI_IVE_Or(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1, IVE_SRC_IMAGE_S *pstSrc2,
                   IVE_DST_IMAGE_S *pstDst, bool bInstant) {
  ScopedTrace t(__PRETTY_FUNCTION__);
  if (!IsValidImageType(pstSrc1, STRFY(pstSrc1), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_U8C3_PLANAR)) {
    return CVI_FAILURE;
  }
  if (!IsValidImageType(pstSrc2, STRFY(pstSrc2), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_U8C3_PLANAR)) {
    return CVI_FAILURE;
  }
  if (!IsValidImageType(pstDst, STRFY(pstDst), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_U8C3_PLANAR)) {
    return CVI_FAILURE;
  }

  IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);
  handle_ctx->t_h.t_or.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
  CviImg *cpp_src1 = reinterpret_cast<CviImg *>(pstSrc1->tpu_block);
  CviImg *cpp_src2 = reinterpret_cast<CviImg *>(pstSrc2->tpu_block);
  CviImg *cpp_dst = reinterpret_cast<CviImg *>(pstDst->tpu_block);
  std::vector<CviImg *> inputs = {cpp_src1, cpp_src2};
  std::vector<CviImg *> outputs = {cpp_dst};

  return handle_ctx->t_h.t_or.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs, outputs);
}

CVI_S32 CVI_IVE_Average(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, float *average,
                        bool bInstant) {
#ifndef CV180X
  ScopedTrace t(__PRETTY_FUNCTION__);
  if (!IsValidImageType(pstSrc, STRFY(pstSrc), IVE_IMAGE_TYPE_U8C1)) {
    return CVI_FAILURE;
  }

  CVI_IVE_BufRequest(pIveHandle, pstSrc);
  uint64_t accumulate = 0;
  neonU8Accumulate(pstSrc->pu8VirAddr[0], pstSrc->u16Stride[0] * pstSrc->u32Height, &accumulate);
  *average = (float)accumulate / (pstSrc->u32Width * pstSrc->u32Height);
#endif
  return CVI_SUCCESS;
}

CVI_S32 CVI_IVE_OrdStatFilter(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
                              IVE_DST_IMAGE_S *pstDst,
                              IVE_ORD_STAT_FILTER_CTRL_S *pstOrdStatFltCtrl, bool bInstant) {
  if (!IsValidImageType(pstSrc, STRFY(pstSrc), IVE_IMAGE_TYPE_U8C1)) {
    return CVI_FAILURE;
  }
  if (!IsValidImageType(pstDst, STRFY(pstDst), IVE_IMAGE_TYPE_U8C1)) {
    return CVI_FAILURE;
  }

  const uint32_t kz = 3;
  const uint32_t pad_sz = kz - 1;
  if ((pstDst->u32Width + pad_sz != pstSrc->u32Width) ||
      (pstDst->u32Height + pad_sz != pstSrc->u32Height)) {
    LOGE("Error, pstDst (width, height) should be pstSrc (width - %u, height - %u).\n", pad_sz,
         pad_sz);
    return CVI_FAILURE;
  }
  IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);
  CviImg *cpp_src = reinterpret_cast<CviImg *>(pstSrc->tpu_block);
  CviImg *cpp_dst = reinterpret_cast<CviImg *>(pstDst->tpu_block);
  std::vector<CviImg *> inputs = {cpp_src};
  std::vector<CviImg *> outputs = {cpp_dst};
  int ret = CVI_SUCCESS;
  if (pstOrdStatFltCtrl->enMode == IVE_ORD_STAT_FILTER_MODE_MAX) {
    handle_ctx->t_h.t_max.setKernelSize(kz);
    handle_ctx->t_h.t_max.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
    ret |= handle_ctx->t_h.t_max.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs, outputs);
  } else if (pstOrdStatFltCtrl->enMode == IVE_ORD_STAT_FILTER_MODE_MIN) {
    handle_ctx->t_h.t_min.setKernelSize(kz);
    handle_ctx->t_h.t_min.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
    ret |= handle_ctx->t_h.t_min.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs, outputs);
  }
  return ret;
}

CVI_S32 CVI_IVE_Sigmoid(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
                        bool bInstant) {
  ScopedTrace t(__PRETTY_FUNCTION__);
  if (!IsValidImageType(pstSrc, STRFY(pstSrc), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_BF16C1)) {
    return CVI_FAILURE;
  }
  if (!IsValidImageType(pstDst, STRFY(pstDst), IVE_IMAGE_TYPE_BF16C1)) {
    return CVI_FAILURE;
  }

  IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);
  handle_ctx->t_h.t_add.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
  CviImg *cpp_src = reinterpret_cast<CviImg *>(pstSrc->tpu_block);
  CviImg *cpp_dst = reinterpret_cast<CviImg *>(pstDst->tpu_block);
  std::vector<CviImg *> inputs = {cpp_src};
  std::vector<CviImg *> outputs = {cpp_dst};
  handle_ctx->t_h.t_sig.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
  return handle_ctx->t_h.t_sig.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs, outputs);
}

CVI_S32 CVI_IVE_SAD(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1, IVE_SRC_IMAGE_S *pstSrc2,
                    IVE_DST_IMAGE_S *pstSad, IVE_DST_IMAGE_S *pstThr, IVE_SAD_CTRL_S *pstSadCtrl,
                    bool bInstant) {
  ScopedTrace t(__PRETTY_FUNCTION__);
  if (pstSrc1->u32Width != pstSrc2->u32Width || pstSrc1->u32Height != pstSrc2->u32Height) {
    LOGE("Two input size must be the same!\n");
    return CVI_FAILURE;
  }
  if (!IsValidImageType(pstSrc1, STRFY(pstSrc1), IVE_IMAGE_TYPE_U8C1)) {
    return CVI_FAILURE;
  }
  if (!IsValidImageType(pstSrc2, STRFY(pstSrc2), IVE_IMAGE_TYPE_U8C1)) {
    return CVI_FAILURE;
  }
  if (!IsValidImageType(pstSad, STRFY(pstSad), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_U16C1,
                        IVE_IMAGE_TYPE_S16C1, IVE_IMAGE_TYPE_BF16C1)) {
    return CVI_FAILURE;
  }
  if (!IsValidImageType(pstThr, STRFY(pstThr), IVE_IMAGE_TYPE_U8C1)) {
    return CVI_FAILURE;
  }

  CVI_U32 window_size = 1;
  switch (pstSadCtrl->enMode) {
    case IVE_SAD_MODE_MB_4X4:
      window_size = 4;
      break;
    case IVE_SAD_MODE_MB_8X8:
      window_size = 8;
      break;
    case IVE_SAD_MODE_MB_16X16:
      window_size = 16;
      break;
    default:
      LOGE("Unsupported SAD mode  %u.\n", pstSadCtrl->enMode);
      return CVI_FAILURE;
      break;
  }
  if (pstSad->u32Width != pstSrc1->u32Width) {
    LOGE("Dst width not match with src! Src: %u, dst: %u.\n", pstSrc1->u32Width, pstSad->u32Width);
    return CVI_FAILURE;
  }
  if (pstSad->u32Height != pstSrc1->u32Height) {
    LOGE("Dst height not match with src! Src: %u, dst: %u.\n", pstSrc1->u32Height,
         pstSad->u32Height);
    return CVI_FAILURE;
  }
  int ret = CVI_SUCCESS;
  bool do_threshold = (pstSadCtrl->enOutCtrl == IVE_SAD_OUT_CTRL_16BIT_BOTH ||
                       pstSadCtrl->enOutCtrl == IVE_SAD_OUT_CTRL_8BIT_BOTH ||
                       pstSadCtrl->enOutCtrl == IVE_SAD_OUT_CTRL_THRESH)
                          ? true
                          : false;
  bool is_output_u8 = (pstSadCtrl->enOutCtrl == IVE_SAD_OUT_CTRL_8BIT_SAD ||
                       pstSadCtrl->enOutCtrl == IVE_SAD_OUT_CTRL_8BIT_BOTH ||
                       pstSadCtrl->enOutCtrl == IVE_SAD_OUT_CTRL_THRESH)
                          ? true
                          : false;
  IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);
  CviImg *cpp_src1 = reinterpret_cast<CviImg *>(pstSrc1->tpu_block);
  CviImg *cpp_src2 = reinterpret_cast<CviImg *>(pstSrc2->tpu_block);
  CviImg *cpp_dst = nullptr;
  std::vector<CviImg *> inputs = {cpp_src1, cpp_src2};
  std::vector<CviImg *> outputs;
  IVE_IMAGE_S dst_BF16;
  if (!is_output_u8) {
    ret = CVI_IVE_CreateImage(pIveHandle, &dst_BF16, IVE_IMAGE_TYPE_BF16C1, pstSad->u32Width,
                              pstSad->u32Height);
    cpp_dst = reinterpret_cast<CviImg *>(dst_BF16.tpu_block);
  } else {
    cpp_dst = reinterpret_cast<CviImg *>(pstSad->tpu_block);
  }
  if (do_threshold) {
    if (pstSad->u32Width != pstThr->u32Width || pstSad->u32Height != pstThr->u32Height) {
      LOGE("Threshold output size must be the same as SAD output!\n");
      return CVI_FAILURE;
    }
    CviImg *thresh_dst = reinterpret_cast<CviImg *>(pstThr->tpu_block);
    if (pstSadCtrl->enOutCtrl == IVE_SAD_OUT_CTRL_THRESH) {
      outputs.emplace_back(thresh_dst);
      handle_ctx->t_h.t_sad.outputThresholdOnly(true);
    } else {
      outputs.emplace_back(cpp_dst);
      outputs.emplace_back(thresh_dst);
    }
  } else {
    outputs.emplace_back(cpp_dst);
  }
  handle_ctx->t_h.t_sad.setTblMgr(&handle_ctx->t_h.t_tblmgr);
  handle_ctx->t_h.t_sad.doThreshold(do_threshold);
  handle_ctx->t_h.t_sad.setThreshold(pstSadCtrl->u16Thr, pstSadCtrl->u8MinVal,
                                     pstSadCtrl->u8MaxVal);
  handle_ctx->t_h.t_sad.setWindowSize(window_size);
  handle_ctx->t_h.t_sad.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
  ret = handle_ctx->t_h.t_sad.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs, outputs);
  if (!is_output_u8) {
    IVE_ITC_CRTL_S iveItcCtrl;
    iveItcCtrl.enType = IVE_ITC_SATURATE;
    ret |= CVI_IVE_ImageTypeConvert(pIveHandle, &dst_BF16, pstSad, &iveItcCtrl, 0);
    ret |= CVI_SYS_FreeI(pIveHandle, &dst_BF16);
  }
  return ret;
}

CVI_S32 CVI_IVE_Sobel(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDstH,
                      IVE_DST_IMAGE_S *pstDstV, IVE_SOBEL_CTRL_S *pstSobelCtrl, bool bInstant) {
  ScopedTrace t(__PRETTY_FUNCTION__);
  if (!IsValidImageType(pstSrc, STRFY(pstSrc), IVE_IMAGE_TYPE_U8C1)) {
    return CVI_FAILURE;
  }

  IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);
  handle_ctx->t_h.t_sobel.setTblMgr(&handle_ctx->t_h.t_tblmgr);
  CviImg *cpp_src = reinterpret_cast<CviImg *>(pstSrc->tpu_block);
  std::vector<CviImg *> inputs = {cpp_src};
  std::vector<CviImg *> outputs;
  uint8_t mask_sz = pstSobelCtrl->u8MaskSize;
  int ret = CVI_FAILURE;
  if (pstSobelCtrl->enOutCtrl == IVE_SOBEL_OUT_CTRL_BOTH) {
    if (!IsValidImageType(pstDstH, STRFY(pstDstH), IVE_IMAGE_TYPE_BF16C1)) {
      return CVI_FAILURE;
    }
    if (!IsValidImageType(pstDstV, STRFY(pstDstV), IVE_IMAGE_TYPE_BF16C1)) {
      return CVI_FAILURE;
    }

    int npu_num = handle_ctx->t_h.t_sobel_gradonly.getNpuNum(handle_ctx->cvk_ctx);
    CviImg *cpp_dsth = reinterpret_cast<CviImg *>(pstDstH->tpu_block);
    CviImg *cpp_dstv = reinterpret_cast<CviImg *>(pstDstV->tpu_block);
    outputs.emplace_back(cpp_dstv);
    outputs.emplace_back(cpp_dsth);
    IveKernel kernel_w =
        createKernel(handle_ctx->rt_handle, npu_num, mask_sz, mask_sz, IVE_KERNEL::SOBEL_X);
    IveKernel kernel_h =
        createKernel(handle_ctx->rt_handle, npu_num, mask_sz, mask_sz, IVE_KERNEL::SOBEL_Y);
    handle_ctx->t_h.t_sobel_gradonly.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
    handle_ctx->t_h.t_sobel_gradonly.setKernel(kernel_w, kernel_h);
    ret = handle_ctx->t_h.t_sobel_gradonly.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs,
                                               outputs);
    kernel_w.img.Free(handle_ctx->rt_handle);
    kernel_h.img.Free(handle_ctx->rt_handle);
  } else if (pstSobelCtrl->enOutCtrl == IVE_SOBEL_OUT_CTRL_HOR) {
    if (!IsValidImageType(pstDstH, STRFY(pstDstH), IVE_IMAGE_TYPE_BF16C1)) {
      return CVI_FAILURE;
    }

    CviImg *cpp_dsth = reinterpret_cast<CviImg *>(pstDstH->tpu_block);
    outputs.emplace_back(cpp_dsth);
    int npu_num = handle_ctx->t_h.t_filter_bf16.getNpuNum(handle_ctx->cvk_ctx);
    IveKernel kernel_h =
        createKernel(handle_ctx->rt_handle, npu_num, mask_sz, mask_sz, IVE_KERNEL::SOBEL_Y);
    handle_ctx->t_h.t_filter_bf16.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
    handle_ctx->t_h.t_filter_bf16.setKernel(kernel_h);
    ret = handle_ctx->t_h.t_filter_bf16.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs,
                                            outputs);
    kernel_h.img.Free(handle_ctx->rt_handle);
  } else if (pstSobelCtrl->enOutCtrl == IVE_SOBEL_OUT_CTRL_VER) {
    if (!IsValidImageType(pstDstV, STRFY(pstDstV), IVE_IMAGE_TYPE_BF16C1)) {
      return CVI_FAILURE;
    }

    CviImg *cpp_dstv = reinterpret_cast<CviImg *>(pstDstV->tpu_block);
    outputs.emplace_back(cpp_dstv);
    int npu_num = handle_ctx->t_h.t_filter_bf16.getNpuNum(handle_ctx->cvk_ctx);
    IveKernel kernel_w =
        createKernel(handle_ctx->rt_handle, npu_num, mask_sz, mask_sz, IVE_KERNEL::SOBEL_X);
    handle_ctx->t_h.t_filter_bf16.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
    handle_ctx->t_h.t_filter_bf16.setKernel(kernel_w);
    ret = handle_ctx->t_h.t_filter_bf16.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs,
                                            outputs);
    kernel_w.img.Free(handle_ctx->rt_handle);
  } else {
    return ret;
  }
  return ret;
}

CVI_S32 CVI_IVE_Sub(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1, IVE_SRC_IMAGE_S *pstSrc2,
                    IVE_DST_IMAGE_S *pstDst, IVE_SUB_CTRL_S *ctrl, bool bInstant) {
  ScopedTrace t(__PRETTY_FUNCTION__);
  if (!IsValidImageType(pstSrc1, STRFY(pstSrc1), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_U8C3_PLANAR)) {
    LOGE("input1 type not support:%d", pstSrc1->enType);
    return CVI_FAILURE;
  }
  if (!IsValidImageType(pstSrc2, STRFY(pstSrc2), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_U8C3_PLANAR)) {
    LOGE("input2 type not support:%d", pstSrc2->enType);
    return CVI_FAILURE;
  }

  int ret = CVI_FAILURE;
  IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);
  if (ctrl->enMode == IVE_SUB_MODE_NORMAL || ctrl->enMode == IVE_SUB_MODE_SHIFT) {
    if (!IsValidImageType(pstDst, STRFY(pstDst), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_S8C1,
                          IVE_IMAGE_TYPE_U8C3_PLANAR)) {
      LOGE("dst type not support:%d", pstDst->enType);
      return CVI_FAILURE;
    }
    handle_ctx->t_h.t_sub.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
    handle_ctx->t_h.t_sub.setSignedOutput(pstDst->enType == IVE_IMAGE_TYPE_S8C1);
    handle_ctx->t_h.t_sub.setRightShiftOneBit(ctrl->enMode == IVE_SUB_MODE_SHIFT);
    CviImg *cpp_src1 = reinterpret_cast<CviImg *>(pstSrc1->tpu_block);
    CviImg *cpp_src2 = reinterpret_cast<CviImg *>(pstSrc2->tpu_block);
    CviImg *cpp_dst = reinterpret_cast<CviImg *>(pstDst->tpu_block);
    std::vector<CviImg *> inputs = {cpp_src1, cpp_src2};
    std::vector<CviImg *> outputs = {cpp_dst};

    ret = handle_ctx->t_h.t_sub.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs, outputs);
  } else if (ctrl->enMode == IVE_SUB_MODE_ABS || ctrl->enMode == IVE_SUB_MODE_ABS_THRESH ||
             ctrl->enMode == IVE_SUB_MODE_ABS_CLIP) {
    if (!IsValidImageType(pstDst, STRFY(pstDst), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_U8C3_PLANAR)) {
      LOGE("dst type not support:%d", pstDst->enType);
      return CVI_FAILURE;
    }

    handle_ctx->t_h.t_sub_abs.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
    handle_ctx->t_h.t_sub_abs.setClipOutput(ctrl->enMode == IVE_SUB_MODE_ABS_CLIP);
    handle_ctx->t_h.t_sub_abs.setBinaryOutput(ctrl->enMode == IVE_SUB_MODE_ABS_THRESH);
    CviImg *cpp_src1 = reinterpret_cast<CviImg *>(pstSrc1->tpu_block);
    CviImg *cpp_src2 = reinterpret_cast<CviImg *>(pstSrc2->tpu_block);
    CviImg *cpp_dst = reinterpret_cast<CviImg *>(pstDst->tpu_block);
    std::vector<CviImg *> inputs = {cpp_src1, cpp_src2};
    std::vector<CviImg *> outputs = {cpp_dst};

    ret =
        handle_ctx->t_h.t_sub_abs.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs, outputs);
  }
  return ret;
}

CVI_S32 CVI_IVE_Thresh(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
                       IVE_THRESH_CTRL_S *ctrl, bool bInstant) {
  ScopedTrace t(__PRETTY_FUNCTION__);
  if (!IsValidImageType(pstSrc, STRFY(pstSrc), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_U8C3_PLANAR)) {
    return CVI_FAILURE;
  }
  if (!IsValidImageType(pstDst, STRFY(pstDst), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_U8C3_PLANAR)) {
    return CVI_FAILURE;
  }

  int ret = CVI_FAILURE;
  IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);
  std::shared_ptr<CviImg> cpp_src;
  std::shared_ptr<CviImg> cpp_dst;
  if (pstSrc->enType == IVE_IMAGE_TYPE_U8C3_PLANAR &&
      pstDst->enType == IVE_IMAGE_TYPE_U8C3_PLANAR) {
    cpp_src = std::shared_ptr<CviImg>(ViewAsU8C1(pstSrc));
    cpp_dst = std::shared_ptr<CviImg>(ViewAsU8C1(pstDst));
  } else {
    cpp_src =
        std::shared_ptr<CviImg>(reinterpret_cast<CviImg *>(pstSrc->tpu_block), [](CviImg *) {});
    cpp_dst =
        std::shared_ptr<CviImg>(reinterpret_cast<CviImg *>(pstDst->tpu_block), [](CviImg *) {});
  }

  if (cpp_src == nullptr || cpp_dst == nullptr) {
    return CVI_FAILURE;
  }

  std::vector<CviImg *> inputs = {cpp_src.get()};
  std::vector<CviImg *> outputs = {cpp_dst.get()};

  if (ctrl->enMode == IVE_THRESH_MODE_BINARY) {
    if (ctrl->u8MinVal == 0 && ctrl->u8MaxVal == 255) {
      handle_ctx->t_h.t_thresh.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
      handle_ctx->t_h.t_thresh.setThreshold(ctrl->u8LowThr);
      ret =
          handle_ctx->t_h.t_thresh.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs, outputs);
    } else {
      handle_ctx->t_h.t_thresh_hl.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
      handle_ctx->t_h.t_thresh_hl.setThreshold(ctrl->u8LowThr, ctrl->u8MinVal, ctrl->u8MaxVal);
      ret = handle_ctx->t_h.t_thresh_hl.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs,
                                            outputs);
    }
  } else if (ctrl->enMode == IVE_THRESH_MODE_SLOPE) {
    handle_ctx->t_h.t_thresh_s.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
    handle_ctx->t_h.t_thresh_s.setThreshold(ctrl->u8LowThr, ctrl->u8MaxVal);
    ret =
        handle_ctx->t_h.t_thresh_s.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs, outputs);
  }
  return ret;
}

CVI_S32 CVI_IVE_Thresh_S16(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
                           IVE_THRESH_S16_CTRL_S *pstThrS16Ctrl, bool bInstant) {
#ifndef CV180X
  if (!IsValidImageType(pstSrc, STRFY(pstSrc), IVE_IMAGE_TYPE_S16C1)) {
    return CVI_FAILURE;
  }

  CVI_IVE_BufRequest(pIveHandle, pstSrc);
  CVI_IVE_BufRequest(pIveHandle, pstDst);
  CviImg *cpp_src = reinterpret_cast<CviImg *>(pstSrc->tpu_block);
  uint64_t data_size = cpp_src->m_tg.stride.n / getFmtSize(cpp_src->m_tg.fmt);
  if (pstThrS16Ctrl->enMode == IVE_THRESH_S16_MODE_S16_TO_S8_MIN_MID_MAX ||
      pstThrS16Ctrl->enMode == IVE_THRESH_S16_MODE_S16_TO_S8_MIN_ORI_MAX) {
    if (!IsValidImageType(pstDst, STRFY(pstDst), IVE_IMAGE_TYPE_S8C1)) {
      return CVI_FAILURE;
    }
    bool is_mmm =
        (pstThrS16Ctrl->enMode == IVE_THRESH_S16_MODE_S16_TO_S8_MIN_MID_MAX) ? true : false;
    neonS162S8ThresholdLH((int16_t *)pstSrc->pu8VirAddr[0], (int8_t *)pstDst->pu8VirAddr[0],
                          data_size, pstThrS16Ctrl->s16LowThr, pstThrS16Ctrl->s16HighThr,
                          pstThrS16Ctrl->un8MinVal.s8Val, pstThrS16Ctrl->un8MidVal.s8Val,
                          pstThrS16Ctrl->un8MaxVal.s8Val, is_mmm);
  } else {
    if (!IsValidImageType(pstDst, STRFY(pstDst), IVE_IMAGE_TYPE_U8C1)) {
      return CVI_FAILURE;
    }
    bool is_mmm =
        (pstThrS16Ctrl->enMode == IVE_THRESH_S16_MODE_S16_TO_U8_MIN_MID_MAX) ? true : false;
    neonS162U8ThresholdLH((int16_t *)pstSrc->pu8VirAddr[0], (uint8_t *)pstDst->pu8VirAddr[0],
                          data_size, pstThrS16Ctrl->s16LowThr, pstThrS16Ctrl->s16HighThr,
                          pstThrS16Ctrl->un8MinVal.u8Val, pstThrS16Ctrl->un8MidVal.u8Val,
                          pstThrS16Ctrl->un8MaxVal.u8Val, is_mmm);
  }
  CVI_IVE_BufFlush(pIveHandle, pstSrc);
  CVI_IVE_BufFlush(pIveHandle, pstDst);
#endif
  return CVI_SUCCESS;
}

CVI_S32 CVI_IVE_Thresh_U16(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
                           IVE_THRESH_U16_CTRL_S *pstThrU16Ctrl, bool bInstant) {
#ifndef CV180X
  if (!IsValidImageType(pstSrc, STRFY(pstSrc), IVE_IMAGE_TYPE_U16C1)) {
    return CVI_FAILURE;
  }
  if (!IsValidImageType(pstDst, STRFY(pstDst), IVE_IMAGE_TYPE_U8C1)) {
    return CVI_FAILURE;
  }

  CVI_IVE_BufRequest(pIveHandle, pstSrc);
  CVI_IVE_BufRequest(pIveHandle, pstDst);
  CviImg *cpp_src = reinterpret_cast<CviImg *>(pstSrc->tpu_block);
  uint64_t data_size = cpp_src->m_tg.stride.n / getFmtSize(cpp_src->m_tg.fmt);
  bool is_mmm = (pstThrU16Ctrl->enMode == IVE_THRESH_U16_MODE_U16_TO_U8_MIN_MID_MAX) ? true : false;
  neonU162U8ThresholdLH((uint16_t *)pstSrc->pu8VirAddr[0], (uint8_t *)pstDst->pu8VirAddr[0],
                        data_size, pstThrU16Ctrl->u16LowThr, pstThrU16Ctrl->u16HighThr,
                        pstThrU16Ctrl->u8MinVal, pstThrU16Ctrl->u8MidVal, pstThrU16Ctrl->u8MaxVal,
                        is_mmm);
  CVI_IVE_BufFlush(pIveHandle, pstSrc);
  CVI_IVE_BufFlush(pIveHandle, pstDst);
#endif
  return CVI_SUCCESS;
}

CVI_S32 CVI_IVE_Xor(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1, IVE_SRC_IMAGE_S *pstSrc2,
                    IVE_DST_IMAGE_S *pstDst, bool bInstant) {
  ScopedTrace t(__PRETTY_FUNCTION__);
  if (!IsValidImageType(pstSrc1, STRFY(pstSrc1), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_U8C3_PLANAR)) {
    return CVI_FAILURE;
  }
  if (!IsValidImageType(pstSrc2, STRFY(pstSrc2), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_U8C3_PLANAR)) {
    return CVI_FAILURE;
  }
  if (!IsValidImageType(pstDst, STRFY(pstDst), IVE_IMAGE_TYPE_U8C1, IVE_IMAGE_TYPE_U8C3_PLANAR)) {
    return CVI_FAILURE;
  }

  IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);
  handle_ctx->t_h.t_xor.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
  CviImg *cpp_src1 = reinterpret_cast<CviImg *>(pstSrc1->tpu_block);
  CviImg *cpp_src2 = reinterpret_cast<CviImg *>(pstSrc2->tpu_block);
  CviImg *cpp_dst = reinterpret_cast<CviImg *>(pstDst->tpu_block);
  std::vector<CviImg *> inputs = {cpp_src1, cpp_src2};
  std::vector<CviImg *> outputs = {cpp_dst};

  return handle_ctx->t_h.t_xor.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs, outputs);
}

// ---------------------------------
// cpu functions
// ---------------------------------

CVI_S32 CVI_IVE_CC(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
                   int *numOfComponents, IVE_CC_CTRL_S *pstCCCtrl, bool bInstant) {
  if (!IsValidImageType(pstSrc, STRFY(pstSrc), IVE_IMAGE_TYPE_U8C1)) {
    return CVI_FAILURE;
  }
  if (!IsValidImageType(pstDst, STRFY(pstDst), IVE_IMAGE_TYPE_U8C1)) {
    return CVI_FAILURE;
  }
  if (pstSrc->u32Width != pstDst->u32Width || pstSrc->u32Height != pstDst->u32Height) {
    LOGE("Src and dst size are not the same.\n");
    return CVI_FAILURE;
  }
  CVI_IVE_BufRequest(pIveHandle, pstSrc);
  CVI_IVE_BufRequest(pIveHandle, pstDst);
  uint8_t *srcPtr = pstSrc->pu8VirAddr[0];
  uint8_t *dstPtr = pstDst->pu8VirAddr[0];
  int32_t width = pstSrc->u32Width;
  int32_t stride = pstSrc->u16Stride[0];
  int32_t height = pstSrc->u32Height;
  uint32_t count = 0;

  struct coord {
    int32_t i;
    int32_t j;
  };
  bool do_eight = pstCCCtrl->enMode == DIRECTION_8 ? true : false;
  std::deque<coord> bb;
  memset(dstPtr, 0, stride * height);
  for (int32_t i = 0; i < height; i++) {
    for (int32_t j = 0; j < width; j++) {
      if (srcPtr[j + i * stride] == 255) {
        count++;
        bb.push_back({i, j});
        srcPtr[j + i * stride] = 0;
        dstPtr[j + i * stride] = count;
        while (!bb.empty()) {
          coord cell = bb.front();
          bb.pop_front();
          if (cell.i < 0 || cell.j < 0 || cell.i >= height || cell.j >= width) {
            continue;
          }
          srcPtr[cell.j + cell.i * stride] = 0;
          dstPtr[cell.j + cell.i * stride] = count;
          auto &&ip1 = cell.i + 1;
          bool is_ip1 = false;
          if (ip1 < height) {
            if (srcPtr[cell.j + ip1 * stride] == 255) {
              bb.push_back({ip1, cell.j});
              srcPtr[cell.j + ip1 * stride] = 0;
              dstPtr[cell.j + ip1 * stride] = count;
            }
            is_ip1 = true;
          }
          auto &&jp1 = cell.j + 1;
          bool is_jp1 = false;
          if (jp1 < width) {
            if (srcPtr[jp1 + cell.i * stride] == 255) {
              bb.push_back({cell.i, jp1});
              srcPtr[jp1 + cell.i * stride] = 0;
              dstPtr[jp1 + cell.i * stride] = count;
            }
            if (do_eight && is_ip1) {
              if (srcPtr[jp1 + ip1 * stride] == 255) {
                bb.push_back({ip1, jp1});
                srcPtr[jp1 + ip1 * stride] = 0;
                dstPtr[jp1 + ip1 * stride] = count;
              }
            }
            is_jp1 = true;
          }
          auto &&im1 = cell.i - 1;
          bool is_im1 = false;
          if (im1 >= 0) {
            if (srcPtr[cell.j + im1 * stride] == 255) {
              bb.push_back({im1, cell.j});
              srcPtr[cell.j + im1 * stride] = 0;
              dstPtr[cell.j + im1 * stride] = count;
            }
            if (do_eight && is_jp1) {
              if (srcPtr[jp1 + im1 * stride] == 255) {
                bb.push_back({im1, jp1});
                srcPtr[jp1 + im1 * stride] = 0;
                dstPtr[jp1 + im1 * stride] = count;
              }
            }
            is_im1 = true;
          }
          auto &&jm1 = cell.j - 1;
          if (jm1 >= 0) {
            if (srcPtr[jm1 + cell.i * stride] == 255) {
              bb.push_back({cell.i, jm1});
              srcPtr[jm1 + cell.i * stride] = 0;
              dstPtr[jm1 + cell.i * stride] = count;
            }
            if (do_eight) {
              if (is_ip1) {
                if (srcPtr[jm1 + ip1 * stride] == 255) {
                  bb.push_back({ip1, jm1});
                  srcPtr[jm1 + ip1 * stride] = 0;
                  dstPtr[jm1 + ip1 * stride] = count;
                }
              }
              if (is_im1) {
                if (srcPtr[jm1 + im1 * stride] == 255) {
                  bb.push_back({im1, jm1});
                  srcPtr[jm1 + im1 * stride] = 0;
                  dstPtr[jm1 + im1 * stride] = count;
                }
              }
            }
          }
        }  // while loop
      }    // if == 255
    }      // for j
  }        // for i
  *numOfComponents = count;
  CVI_IVE_BufFlush(pIveHandle, pstSrc);
  CVI_IVE_BufFlush(pIveHandle, pstDst);
  return CVI_SUCCESS;
}
/**
 * @Param Src gray image (size: wxh)
 * @Param Integral integral image (size: (w+1)x(h+1))
 * @Param Width image width
 * @Param Height image height
 * @Param Stride shift bytes
 */
inline void GetGrayIntegralImage(uint8_t *Src, uint32_t *Integral, int Width, int Height,
                                 int src_stride, int dst_stride) {
  memset(Integral, 0, dst_stride * sizeof(uint32_t));

  for (int Y = 0; Y < Height; Y++) {
    uint8_t *LinePS = Src + Y * src_stride;
    uint32_t *LinePL = Integral + Y * (dst_stride) + 1;
    uint32_t *LinePD = Integral + (Y + 1) * (dst_stride) + 1;
    LinePD[-1] = 0;
    for (int X = 0, Sum = 0; X < Width; X++) {
      Sum += LinePS[X];
      LinePD[X] = LinePL[X] + Sum;
    }
  }
}

/**
 * @param cols image width
 * @param rows image column
 * @param image gray image
 * @param hist buffer for histogram values
 * @param num_bins how many values you want to find frequency
 */

inline int cal_hist(int cols, int rows, uint8_t *image, int src_stride, uint32_t *hist,
                    int num_bins) {
  if (cols < 1 || rows < 1 || num_bins < 1) {
    return (1);
  }
  memset(hist, 0, sizeof(uint32_t) * num_bins);

  for (int Y = 0; Y < rows; Y++) {
    uint8_t *LinePS = image + Y * src_stride;
    for (int X = 0, Sum = 0; X < cols; X++) {
      Sum = LinePS[X];
      hist[Sum]++;
    }
  }

  return (0);
}

/**
 * @param hist frequencies
 * @param eqhist new gray level (newly mapped pixel values)
 * @param nbr_elements total number of pixels
 * @param nbr_bins number of levels
 */

inline int equalize_hist(uint32_t *hist, uint8_t *eqhist, int nbr_elements, int nbr_bins) {
  int curr, i, total;
  if (nbr_elements < 1 || nbr_bins < 1) {
    return (1);
  }
  curr = 0;
  total = nbr_elements;
  // calculating cumulative frequency and new gray levels
  for (i = 0; i < nbr_bins; i++) {
    // cumulative frequency
    curr += hist[i];
    // calculating new gray level after multiplying by
    // maximum gray count which is 255 and dividing by
    // total number of pixels
    eqhist[i] = (uint8_t)round((((float)curr) * 255) / total);
  }
  return (0);
}

inline int histogramEqualisation(int cols, int rows, uint8_t *image, int src_stride, uint8_t *pDst,
                                 int dst_stride) {
  uint32_t hist[256] = {0};
  uint8_t new_gray_level[256] = {0};
  int col, row, total, st;

  st = cal_hist(cols, rows, image, src_stride, hist, 256);
  if (st > 0) {
    return (st);
  }
  total = cols * rows;
  st = equalize_hist(hist, new_gray_level, total, 256);
  if (st > 0) {
    return (st);
  }
  uint8_t *ptr = image;
  for (row = 0; row < rows; row++) {
    for (col = 0; col < cols; col++) {
      pDst[col] = (unsigned char)new_gray_level[ptr[col]];
    }
    pDst += dst_stride;
    ptr += src_stride;
  }
  return st;
}

// main body
CVI_S32 CVI_IVE_Integ(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_MEM_INFO_S *pstDst,
                      IVE_INTEG_CTRL_S *ctrl, bool bInstant) {
  if (pstSrc->enType != IVE_IMAGE_TYPE_U8C1) {
    LOGE("Output only accepts U8C1 image format.\n");
    return CVI_FAILURE;
  }

  CVI_IVE_BufRequest(pIveHandle, pstSrc);

  uint32_t *ptr = (uint32_t *)pstDst->pu8VirAddr;
  int channels = 1;
  int dst_stride = channels * (pstSrc->u32Width + 1);

  GetGrayIntegralImage((uint8_t *)pstSrc->pu8VirAddr[0], ptr, (int)pstSrc->u32Width,
                       (int)pstSrc->u32Height, (int)pstSrc->u16Stride[0], dst_stride);

  CVI_IVE_BufFlush(pIveHandle, pstSrc);
  return CVI_SUCCESS;
}

CVI_S32 CVI_IVE_Hist(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_MEM_INFO_S *pstDst,
                     bool bInstant) {
  if (pstSrc->enType != IVE_IMAGE_TYPE_U8C1) {
    LOGE("Output only accepts U8C1 image format.\n");
    return CVI_FAILURE;
  }

  cal_hist((int)pstSrc->u32Width, (int)pstSrc->u32Height, (uint8_t *)pstSrc->pu8VirAddr[0],
           (int)pstSrc->u16Stride[0], (uint32_t *)pstDst->pu8VirAddr, 256);
  CVI_IVE_BufRequest(pIveHandle, pstSrc);
  return CVI_SUCCESS;
}

CVI_S32 CVI_IVE_EqualizeHist(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
                             IVE_DST_IMAGE_S *pstDst, IVE_EQUALIZE_HIST_CTRL_S *ctrl,
                             bool bInstant) {
  if (pstSrc->enType != IVE_IMAGE_TYPE_U8C1) {
    LOGE("Output only accepts U8C1 image format.\n");
    return CVI_FAILURE;
  }
  CVI_IVE_BufRequest(pIveHandle, pstSrc);

  histogramEqualisation((int)pstSrc->u32Width, (int)pstSrc->u32Height,
                        (uint8_t *)pstSrc->pu8VirAddr[0], (int)pstSrc->u16Stride[0],
                        (uint8_t *)pstDst->pu8VirAddr[0], (int)pstDst->u16Stride[0]);

  CVI_IVE_BufFlush(pIveHandle, pstSrc);

  return CVI_SUCCESS;
}

#ifdef __ARM_ARCH_7A__
uint32_t sum_uint32x4(uint32x4_t vec) {
  uint32x2_t sum_lane = vadd_u32(vget_low_u32(vec), vget_high_u32(vec));
  sum_lane = vpadd_u32(sum_lane, sum_lane);
  uint32_t sum;
  vst1_lane_u32(&sum, sum_lane, 0);
  return sum;
}
#endif
inline float cal_norm_cc(unsigned char *psrc1, unsigned char *psrc2, int srcw, int srch,
                         int stride) {
  uint t1, t2, t3;
  float rtv = 0;
  double d1, d2, d3;

  if (srcw < 1 || srch < 1) {
    return (0.0);
  }
  t1 = 0;
  t2 = 0;
  t3 = 0;

#ifdef __ARM_ARCH_7A__
  int nn = srcw & 0xfffffff8;
  int remain = srcw - nn;
  uint32x4_t mul_acc_1_x = vdupq_n_u32(0);
  uint32x4_t mul_acc_2_x = vdupq_n_u32(0);
  uint32x4_t mul_acc_3_x = vdupq_n_u32(0);
  uint16x8_t mul_ret1 = vdupq_n_u16(0);
  uint16x8_t mul_ret2 = vdupq_n_u16(0);
  uint16x8_t mul_ret3 = vdupq_n_u16(0);

  for (int i = 0; i < srch; i++) {
    for (int j = 0; j < nn; j += 8) {
      int pixel = i * stride + j;
      uint8x8_t vec_1 = vld1_u8(psrc1 + pixel);
      uint8x8_t vec_2 = vld1_u8(psrc2 + pixel);

      mul_ret1 = vmull_u8(vec_1, vec_2);
      mul_ret2 = vmull_u8(vec_1, vec_1);
      mul_ret3 = vmull_u8(vec_2, vec_2);

      mul_acc_1_x =
          vaddq_u32(vaddl_u16(vget_low_u16(mul_ret1), vget_high_u16(mul_ret1)), mul_acc_1_x);
      mul_acc_2_x =
          vaddq_u32(vaddl_u16(vget_low_u16(mul_ret2), vget_high_u16(mul_ret2)), mul_acc_2_x);
      mul_acc_3_x =
          vaddq_u32(vaddl_u16(vget_low_u16(mul_ret3), vget_high_u16(mul_ret3)), mul_acc_3_x);
    }

    if (remain > 0) {
      int shift = i * stride + nn;
      for (int x = 0; x < remain; x++) {
        int pixel = shift + x;
        unsigned char src1 = psrc1[pixel];
        unsigned char src2 = psrc2[pixel];
        t1 += (src1 * src2);
        t2 += (src1 * src1);
        t3 += (src2 * src2);
      }
    }
  }

  t1 += sum_uint32x4(mul_acc_1_x);
  t2 += sum_uint32x4(mul_acc_2_x);
  t3 += sum_uint32x4(mul_acc_3_x);
  if (t2 < 1 || t3 < 1) {
    return (0.0);
  }

#else

  for (int i = 0; i < srch; i++) {
    for (int j = 0; j < srcw; j++) {
      int pixel = i * stride + j;
      unsigned char src1 = psrc1[pixel];
      unsigned char src2 = psrc2[pixel];
      t1 += (src1 * src2);
      t2 += (src1 * src1);
      t3 += (src2 * src2);
    }
  }
  if (t2 < 1 || t3 < 1) {
    return (0.0);
  }

#endif
  d1 = (double)(t1);
  d2 = sqrt((double)t2) * sqrt((double)t3);
  d3 = d1 / (d2 + 1);
  rtv = (float)(d3);

  return rtv;
}

CVI_S32 CVI_IVE_NCC(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1, IVE_SRC_IMAGE_S *pstSrc2,
                    IVE_DST_MEM_INFO_S *pstDst, bool bInstant) {
  if (pstSrc1->enType != IVE_IMAGE_TYPE_U8C1) {
    LOGE("Input 1 only accepts U8C1 image format.\n");
    return CVI_FAILURE;
  }
  if (pstSrc2->enType != IVE_IMAGE_TYPE_U8C1) {
    LOGE("Input 2 only accepts U8C1 image format.\n");
    return CVI_FAILURE;
  }
  if (bInstant) {
    CVI_IVE_BufRequest(pIveHandle, pstSrc1);
    CVI_IVE_BufRequest(pIveHandle, pstSrc2);
  }
  float *ptr = (float *)pstDst->pu8VirAddr;
  float rt =
      cal_norm_cc((uint8_t *)pstSrc1->pu8VirAddr[0], (uint8_t *)pstSrc2->pu8VirAddr[0],
                  (int)pstSrc1->u32Width, (int)pstSrc1->u32Height, (int)pstSrc1->u16Stride[0]);
  ptr[0] = rt;
  if (bInstant) {
    CVI_IVE_BufFlush(pIveHandle, pstSrc1);
    CVI_IVE_BufFlush(pIveHandle, pstSrc2);
  }

  return CVI_SUCCESS;
}

inline void uint16_8bit(uint16_t *in, uint8_t *out, int dim) {
  int i;

  for (i = 0; i < dim; i++) {
    // uint16_t n = in[i];  // because shifting the sign bit invokes UB
    // uint8_t hi = ((n >> 8) & 0xff);
    uint8_t lo = ((in[i] >> 0) & 0xff);
    out[i] = lo;
  }
}

CVI_S32 CVI_IVE_16BitTo8Bit(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
                            IVE_16BIT_TO_8BIT_CTRL_S *ctrl, bool bInstant) {
  if (pstSrc->enType != IVE_IMAGE_TYPE_U16C1) {
    LOGE("Input only accepts U16C1 image format.\n");
    return CVI_FAILURE;
  }
  if (pstDst->enType != IVE_IMAGE_TYPE_U8C1) {
    LOGE("Output only accepts U8C1 image format.\n");
    return CVI_FAILURE;
  }
  CVI_IVE_BufRequest(pIveHandle, pstSrc);
  CVI_IVE_BufRequest(pIveHandle, pstDst);

  int wxh = ((int)pstSrc->u16Stride[0] / 2 * (int)pstSrc->u32Height);
  uint16_8bit((uint16_t *)pstSrc->pu8VirAddr[0], (uint8_t *)pstDst->pu8VirAddr[0], wxh);

  CVI_IVE_BufFlush(pIveHandle, pstSrc);
  CVI_IVE_BufFlush(pIveHandle, pstDst);
  return CVI_SUCCESS;
}

typedef enum _LbpMappingType {
  LbpUniform /**< Uniform local binary patterns. */
} LbpMappingType;

/** @brief Local Binary Pattern extractor */
typedef struct cvLbp_ {
  uint32_t dimension;
  uint32_t mapping[256];
  bool transposed;
} cvLbp;

static void lbp_init_uniform(cvLbp *self) {
  int i, j;

  /* overall number of quantized LBPs */
  self->dimension = 58;

  /* all but selected patterns map to bin 57 (the first bin has index 0) */
  for (i = 0; i < 256; ++i) {
    self->mapping[i] = 57;
  }

  /* the uniform (all zeros or ones) patterns map to bin 56 */
  self->mapping[0x00] = 56;
  self->mapping[0xff] = 56;

  /* 56 uniform patterns */
  for (i = 0; i < 8; ++i) {
    for (j = 1; j <= 7; ++j) {
      int ip;
      int unsigned string;
      if (self->transposed) {
        ip = (-i + 2 - (j - 1) + 16) % 8;
      } else {
        ip = i;
      }

      /* string starting with j ones */
      string = (1 << j) - 1;
      string <<= ip;
      string = (string | (string >> 8)) & 0xff;

      self->mapping[string] = i * 7 + (j - 1);
    }
  }
}
uint32_t lbp_get_dimension(cvLbp *self) { return self->dimension; }

/** @brief Extract LBP features
 ** @param self LBP object.
 ** @param features buffer to write the features to.
 ** @param image image.
 ** @param width image width.
 ** @param height image height.
 ** @param cellSize size of the LBP cells. Note: 32x32
 **
 ** @a features is a  @c numColumns x @c numRows x @c dimension where
 ** @c dimension is the dimension of a LBP feature obtained from ::vl_lbp_get_dimension,
 ** @c numColumns is equal to @c floor(width / cellSize), and similarly
 ** for @c numRows.
 **/

void lbp_process(cvLbp *self, uint8_t *lbpimg, uint8_t *image, uint32_t stride, uint32_t width,
                 uint32_t height) {
  // uint32_t cellSize = 32;
  // uint32_t cwidth = width / cellSize;
  // uint32_t cheight = height / cellSize;
  // uint32_t cstride = cwidth * cheight;
  // uint32_t cdimension = lbp_get_dimension(self);
  int x, y, bin;

#define at(u, v) (*(image + stride * (v) + (u)))
#define to(m, n) (*(lbpimg + stride * (n) + (m)))

  /* clear the output buffer */
  memset(lbpimg, 0, (stride * height));

  /* accumulate pixel-level measurements into cells */
  for (y = 1; y < (signed)height - 1; ++y) {
    // float wy1 = (y + 0.5f) / (float)cellSize - 0.5f;
    // int cy1 = (int)floor(wy1);
    // int cy2 = cy1 + 1;
    // float wy2 = wy1 - (float)cy1;
    // wy1 = 1.0f - wy2;
    // if (cy1 >= (signed)cheight) continue;

    for (x = 1; x < (signed)width - 1; ++x) {
      // float wx1 = (x + 0.5f) / (float)cellSize - 0.5f;
      // int cx1 = (int)floor(wx1);
      // int cx2 = cx1 + 1;
      // float wx2 = wx1 - (float)cx1;
      // wx1 = 1.0f - wx2;
      // if (cx1 >= (signed)cwidth) continue;

      {
        int unsigned bitString = 0;
        uint8_t center = at(x, y);
        if (at(x + 1, y + 0) > center) bitString |= 0x1 << 0; /*  E */
        if (at(x + 1, y + 1) > center) bitString |= 0x1 << 1; /* SE */
        if (at(x + 0, y + 1) > center) bitString |= 0x1 << 2; /* S  */
        if (at(x - 1, y + 1) > center) bitString |= 0x1 << 3; /* SW */
        if (at(x - 1, y + 0) > center) bitString |= 0x1 << 4; /*  W */
        if (at(x - 1, y - 1) > center) bitString |= 0x1 << 5; /* NW */
        if (at(x + 0, y - 1) > center) bitString |= 0x1 << 6; /* N  */
        if (at(x + 1, y - 1) > center) bitString |= 0x1 << 7; /* NE */
        bin = self->mapping[bitString];
        to(x, y) = bin;
      }

    } /* x */
  }   /* y */
}

CVI_S32 CVI_IVE_LBP(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
                    IVE_LBP_CTRL_S *ctrl, bool bInstant) {
  if (pstSrc->enType != IVE_IMAGE_TYPE_U8C1) {
    LOGE("Input only accepts U8C1 image format.\n");
    return CVI_FAILURE;
  }
  if (pstDst->enType != IVE_IMAGE_TYPE_U8C1) {
    LOGE("Output only accepts U8C1 image format.\n");
    return CVI_FAILURE;
  }

  cvLbp self;
  // FIXME: Undefined behavior, give it an initial value first.
  self.transposed = true;
  lbp_init_uniform(&self);

  CVI_IVE_BufRequest(pIveHandle, pstSrc);

  lbp_process(&self, (uint8_t *)pstDst->pu8VirAddr[0], (uint8_t *)pstSrc->pu8VirAddr[0],
              (uint32_t)pstSrc->u16Stride[0], (uint32_t)pstSrc->u32Width, (int)pstSrc->u32Height);

  CVI_IVE_BufFlush(pIveHandle, pstSrc);

  return CVI_SUCCESS;
}

#include "avir/avir.h"

CVI_S32 CVI_IVE_Resize(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
                       IVE_RESIZE_CTRL_S *ctrl, bool bInstant) {
  if (pstSrc->enType != IVE_IMAGE_TYPE_U8C1) {
    LOGE("Input only accepts U8C1 image format.\n");
    return CVI_FAILURE;
  }
  if (pstDst->enType != IVE_IMAGE_TYPE_U8C1) {
    LOGE("Output only accepts U8C1 image format.\n");
    return CVI_FAILURE;
  }

  CVI_IVE_BufRequest(pIveHandle, pstSrc);

  avir::CImageResizer<> ImageResizer(8);
  ImageResizer.resizeImage((uint8_t *)pstSrc->pu8VirAddr[0], (int)pstSrc->u32Width,
                           (int)pstSrc->u32Height, 0, (uint8_t *)pstDst->pu8VirAddr[0],
                           (int)pstDst->u32Width, (int)pstDst->u32Height, 1, 0);

  CVI_IVE_BufFlush(pIveHandle, pstSrc);

  return CVI_SUCCESS;
}

#if 0
#include "cvi_vip.h"
CVI_S32 set_fmt_ex(CVI_S32 fd, CVI_S32 width, CVI_S32 height, CVI_U32 pxlfmt, CVI_U32 csc, CVI_U32 quant)
{
	struct v4l2_format fmt;
	//fmt.type = type;
	fmt.fmt.pix_mp.width = width;
	fmt.fmt.pix_mp.height = height;
	fmt.fmt.pix_mp.pixelformat = pxlfmt;
	fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;
	if (pxlfmt == V4L2_PIX_CVK_FMT_RGBM){
	fmt.fmt.pix_mp.colorspace = V4L2_COLORSPACE_SRGB;
	}else{
	fmt.fmt.pix_mp.colorspace = csc;
	}

	fmt.fmt.pix_mp.quantization = quant;
	fmt.fmt.pix_mp.num_planes = 3;

	if (-1 == ioctl(fd, VIDIOC_TRY_FMT, &fmt)){
		perror("VIDIOC_TRY_FMT");
		//printf("VIDIOC_TRY_FMT");
	}
	if (-1 == ioctl(fd, VIDIOC_S_FMT, &fmt)){
		perror("VIDIOC_S_FMT");
		//printf("VIDIOC_S_FMT");
	}
	return(fmt.fmt.pix.sizeimage);
}


CVI_S32 CVI_IVE_CSC(IVE_HANDLE *pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst
			, IVE_CSC_CTRL_S *pstCscCtrl, CVI_BOOL bInstant)
{
	CVI_U32 srcfmt, dstfmt, srcCSC, dstCSC, srcQuant, dstQuant;
	struct vdev *srcd, *dstd;
	struct buffer buf;
	CVI_S32 ret;
#if 0
	srcd = get_dev_info(VDEV_TYPE_IMG, 0);
	dstd = get_dev_info(VDEV_TYPE_SC, 0);

  srcfmt = V4L2_PIX_CVK_FMT_YUV420M;
  dstfmt = V4L2_PIX_CVK_FMT_RGBM;
	srcCSC = V4L2_COLORSPACE_REC709;
	srcQuant = V4L2_QUANTIZATION_DEFAULT;
	dstCSC = V4L2_COLORSPACE_REC709;
	dstQuant = V4L2_QUANTIZATION_LIM_RANGE;
#endif

#if 0
	switch (pstCscCtrl->enMode) {
  case IVE_CSC_MODE_PIC_BT601_YUV2HSV:
	case IVE_CSC_MODE_VIDEO_BT601_YUV2RGB:
	case IVE_CSC_MODE_PIC_BT601_YUV2RGB:
  case IVE_CSC_MODE_VIDEO_BT709_YUV2RGB:
  case IVE_CSC_MODE_PIC_BT601_YUV2HSV:
	case IVE_CSC_MODE_PIC_BT601_YUV2HSV:
		if (srcfmt == V4L2_PIX_CVK_FMT_RGBM) {
			//CVI_TRACE(CVI_DBG_ERR, CVI_ID_IVE, "Invalid parameters\n");
			return CVI_FAILURE;
		}
		srcCSC = V4L2_COLORSPACE_SMPTE170M;
		srcQuant = V4L2_QUANTIZATION_DEFAULT;
		break;
	case IVE_CSC_MODE_VIDEO_BT709_YUV2RGB:
	case IVE_CSC_MODE_PIC_BT709_YUV2RGB:
	case IVE_CSC_MODE_PIC_BT709_YUV2HSV:
		if (srcfmt == V4L2_PIX_CVK_FMT_RGBM) {
			//CVI_TRACE(CVI_DBG_ERR, CVI_ID_IVE, "Invalid parameters\n");
			return CVI_FAILURE;
		}
		srcCSC = V4L2_COLORSPACE_REC709;
		srcQuant = V4L2_QUANTIZATION_DEFAULT;
		break;
	case IVE_CSC_MODE_VIDEO_BT601_RGB2YUV:
	case IVE_CSC_MODE_VIDEO_BT709_RGB2YUV:
	case IVE_CSC_MODE_PIC_BT601_RGB2YUV:
	case IVE_CSC_MODE_PIC_BT709_RGB2YUV:
		if (srcfmt != V4L2_PIX_CVK_FMT_RGBM) {
			//CVI_TRACE(CVI_DBG_ERR, CVI_ID_IVE, "Invalid parameters\n");
			return CVI_FAILURE;
		}
		srcCSC = V4L2_COLORSPACE_SRGB;
		srcQuant = V4L2_QUANTIZATION_DEFAULT;
		break;
	}
#endif

	//srcCSC = V4L2_COLORSPACE_REC709;
	//srcQuant = V4L2_QUANTIZATION_DEFAULT;

#if 0
	set_fmt_ex(srcd->fd, pstSrc->u32Width, pstSrc->u32Height, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE
		, srcfmt, srcCSC, srcQuant);
	set_fmt_ex(dstd->fd, pstDst->u32Width, pstDst->u32Height, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE
		, dstfmt, dstCSC, dstQuant);

	for (CVI_U8 i = 0; i < 3; ++i) {
		buf.phy_addr[i] = pstSrc->u64PhyAddr[i];
		buf.length[i] = pstSrc->u16Stride[i] * pstSrc->u32Height;
		if (pstSrc->enType == IVE_IMAGE_TYPE_YUV420P && i > 0)
			buf.length[i] >>= 1;
	}
	qbuf(srcd, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, &buf);
	for (CVI_U8 i = 0; i < 3; ++i) {
		buf.phy_addr[i] = pstDst->u64PhyAddr[i];
		buf.length[i] = pstDst->u16Stride[i] * pstDst->u32Height;
		if (pstDst->enType == IVE_IMAGE_TYPE_YUV420P && i > 0)
			buf.length[i] >>= 1;
	}
	qbuf(dstd, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, &buf);


	do {
		fd_set wfds;
		struct timeval tv;

		FD_ZERO(&wfds);
		FD_SET(dstd->fd, &wfds);
		tv.tv_sec = 0;
		tv.tv_usec = 500 * 1000;
		ret = select(dstd->fd + 1, NULL, &wfds, NULL, &tv);
		if (ret == -1) {
			if (errno == EINTR)
				continue;
			//CVI_TRACE(CVI_DBG_ERR, CVI_ID_IVE, "select error\n");
			break;
		}

		if (ret == 0) {
			//CVI_TRACE(CVI_DBG_ERR, CVI_ID_IVE, "select timeout\n");
			ret = CVI_FAILURE;
			break;
		}

		if (FD_ISSET(dstd->fd, &wfds)) {
			ret = CVI_SUCCESS;
			break;
		}
	} while(1);
#endif
	return ret;
}

#else

float max(float a, float b, float c) { return ((a > b) ? (a > c ? a : c) : (b > c ? b : c)); }
float min(float a, float b, float c) { return ((a < b) ? (a < c ? a : c) : (b < c ? b : c)); }
int rgb_to_hsv(float r, float g, float b, float &h, float &s, float &v) {
  // R, G, B values are divided by 255
  // to change the range from 0..255 to 0..1:
  // float h, s, v;
  h = 0;
  s = 0;
  v = 0;

  r /= 255.0;
  g /= 255.0;
  b /= 255.0;
  float cmax = max(r, g, b);       // maximum of r, g, b
  float cmin = min(r, g, b);       // minimum of r, g, b
  float diff = cmax - cmin + 1.0;  // diff of cmax and cmin.
  if (cmax == cmin)
    h = 0;
  else if (cmax == r)
    h = fmod((60 * ((g - b) / diff) + 360), 360.0);
  else if (cmax == g)
    h = fmod((60 * ((b - r) / diff) + 120), 360.0);
  else if (cmax == b)
    h = fmod((60 * ((r - g) / diff) + 240), 360.0);
  // if cmax equal zero
  if (cmax == 0) {
    s = 0;
  } else {
    s = (diff / cmax) * 100;
  }
  // compute v
  v = cmax * 100;
  // printf("h s v=(%f, %f, %f)\n", h, s, v );
  return 0;
}

int rgbToHsv(unsigned char *rgb, unsigned char *hsv, int srcw, int srch, int ch) {
  int i;
  int wxh = srcw * srch;
  float r, g, b, h, s, v;

  for (i = 0; i < wxh; i++) {
    r = rgb[ch * i] / 255.0;
    g = rgb[ch * i + 1] / 255.0;
    b = rgb[ch * i + 2] / 255.0;
    rgb_to_hsv(r, g, b, h, s, v);

    hsv[ch * i] = floor(h);
    hsv[ch * i + 1] = floor(s);
    hsv[ch * i + 2] = floor(v);
  }

  return 0;
}

void rgbToGray(unsigned char *rgb, unsigned char *gray, int stride, int srcw, int srch) {
  int i, j, n, ii;
  uint r, g, b;
  unsigned char one_gray;
  unsigned char *pr, *pg, *pb;
  pr = rgb;
  pg = rgb + srcw * srch;
  pb = rgb + srcw * srch * 2;

  for (i = 0; i < srch; i++) {
    n = 0;
    ii = i * srcw;
    for (j = 0; j < srcw; j++, n++) {
      r = *(pr + ii + j);          // red
      g = *(pg + ii + j);          // green
      b = *(pb + ii + j);          // blue
      one_gray = (r + g + b) / 3;  //(r*19595 + g*38469 + b*7472) >> 16;
      *(gray + ii + n) = one_gray;
    }
  }
}

CVI_S32 CVI_IVE_CSC(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc, IVE_DST_IMAGE_S *pstDst,
                    IVE_CSC_CTRL_S *ctrl, bool bInstant) {
  if (pstSrc->enType != IVE_IMAGE_TYPE_U8C3_PLANAR) {
    LOGE("Input only accepts U8C3_PLANAR image format.\n");
    return CVI_FAILURE;
  }

  CVI_IVE_BufRequest(pIveHandle, pstSrc);
  int strideSrc;  //, strideDst;

  switch (ctrl->enMode) {
    case IVE_CSC_MODE_PIC_RGB2HSV:
      rgbToHsv((uint8_t *)pstSrc->pu8VirAddr[0], (uint8_t *)pstDst->pu8VirAddr[0],
               (int)pstSrc->u32Width, (int)pstSrc->u32Height, 3);
      break;

    case IVE_CSC_MODE_PIC_RGB2GRAY:
      strideSrc = pstSrc->u16Stride[0];
      // strideDst = pstDst->u16Stride[0];
      // printf("strideSrc: %d, strideDat: %d\n", strideSrc, strideDst);
      rgbToGray((uint8_t *)pstSrc->pu8VirAddr[0], (uint8_t *)pstDst->pu8VirAddr[0], strideSrc,
                (int)pstSrc->u32Width, (int)pstSrc->u32Height);
      break;

    default:
      strideSrc = pstSrc->u16Stride[0];
      // strideDst = pstDst->u16Stride[0];
      // printf("strideSrc: %d, strideDat: %d\n", strideSrc, strideDst);
      rgbToGray((uint8_t *)pstSrc->pu8VirAddr[0], (uint8_t *)pstDst->pu8VirAddr[0], strideSrc,
                (int)pstSrc->u32Width, (int)pstSrc->u32Height);

      break;
  }

  CVI_IVE_BufFlush(pIveHandle, pstSrc);

  return CVI_SUCCESS;
}

CVI_S32 CVI_IVE_FilterAndCSC(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc,
                             IVE_SRC_IMAGE_S *pstBuf, IVE_DST_IMAGE_S *pstDst,
                             IVE_FILTER_AND_CSC_CTRL_S *ctrl, bool bInstant) {
  if (pstBuf->enType != IVE_IMAGE_TYPE_U8C3_PLANAR) {
    LOGE("Input only accepts U8C3_PLANAR image format.\n");
    return CVI_FAILURE;
  }

  IVE_FILTER_CTRL_S iveFltCtrl;
  iveFltCtrl.u8MaskSize = 5;
  memcpy(iveFltCtrl.as8Mask, ctrl->as8Mask, 25 * sizeof(CVI_S8));
  iveFltCtrl.u32Norm = 273;
  CVI_IVE_Filter(pIveHandle, pstSrc, pstBuf, &iveFltCtrl, 0);

  memcpy(pstBuf->pu8VirAddr[0], pstSrc->pu8VirAddr[0], pstBuf->u16Stride[0] * pstBuf->u32Height);
  memcpy(pstBuf->pu8VirAddr[1], pstSrc->pu8VirAddr[0], pstBuf->u16Stride[0] * pstBuf->u32Height);
  memcpy(pstBuf->pu8VirAddr[2], pstSrc->pu8VirAddr[0], pstBuf->u16Stride[0] * pstBuf->u32Height);

  CVI_IVE_BufRequest(pIveHandle, pstBuf);
  int strideSrc;  //, strideDst;

  switch (ctrl->enMode) {
    case IVE_CSC_MODE_PIC_RGB2HSV:
      rgbToHsv((uint8_t *)pstBuf->pu8VirAddr[0], (uint8_t *)pstDst->pu8VirAddr[0],
               (int)pstBuf->u32Width, (int)pstBuf->u32Height, 3);
      break;

    case IVE_CSC_MODE_PIC_RGB2GRAY:
      strideSrc = pstBuf->u16Stride[0];
      // strideDst = pstDst->u16Stride[0];
      // printf("strideSrc: %d, strideDat: %d\n", strideSrc, strideDst);
      rgbToGray((uint8_t *)pstBuf->pu8VirAddr[0], (uint8_t *)pstDst->pu8VirAddr[0], strideSrc,
                (int)pstBuf->u32Width, (int)pstBuf->u32Height);
      break;

    default:
      strideSrc = pstBuf->u16Stride[0];
      // strideDst = pstDst->u16Stride[0];
      // printf("strideSrc: %d, strideDat: %d\n", strideSrc, strideDst);
      rgbToGray((uint8_t *)pstBuf->pu8VirAddr[0], (uint8_t *)pstDst->pu8VirAddr[0], strideSrc,
                (int)pstBuf->u32Width, (int)pstBuf->u32Height);

      break;
  }

  CVI_IVE_BufFlush(pIveHandle, pstBuf);

  return CVI_SUCCESS;
}

CVI_S32 CVI_IVE_CMP_S8_BINARY(IVE_HANDLE pIveHandle, IVE_SRC_IMAGE_S *pstSrc1,
                              IVE_SRC_IMAGE_S *pstSrc2, IVE_DST_IMAGE_S *pstDst) {
  if ((pstSrc1->enType != IVE_IMAGE_TYPE_S8C1) || (pstSrc1->enType != pstSrc2->enType) ||
      pstDst->enType != IVE_IMAGE_TYPE_U8C1) {
    LOGE("source1/source2/dst image pixel format do not match,%d,%d,%d!\n", pstSrc1->enType,
         pstSrc2->enType, pstDst->enType);
    return CVI_FAILURE;
  }

  int ret = CVI_FAILURE;
  IVE_HANDLE_CTX *handle_ctx = reinterpret_cast<IVE_HANDLE_CTX *>(pIveHandle);

  std::shared_ptr<CviImg> cpp_src1;
  std::shared_ptr<CviImg> cpp_src2;
  std::shared_ptr<CviImg> cpp_dst;

  cpp_src1 =
      std::shared_ptr<CviImg>(reinterpret_cast<CviImg *>(pstSrc1->tpu_block), [](CviImg *) {});
  cpp_src2 =
      std::shared_ptr<CviImg>(reinterpret_cast<CviImg *>(pstSrc2->tpu_block), [](CviImg *) {});
  cpp_dst = std::shared_ptr<CviImg>(reinterpret_cast<CviImg *>(pstDst->tpu_block), [](CviImg *) {});

  if (cpp_src1 == nullptr || cpp_src2 == nullptr || cpp_dst == nullptr) {
    LOGE("Cannot get tpu block\n");
    return CVI_FAILURE;
  }

  if ((cpp_src1->GetImgHeight() != cpp_src2->GetImgHeight()) ||
      (cpp_src1->GetImgHeight() != cpp_dst->GetImgHeight()) ||
      (cpp_src1->GetImgWidth() != cpp_src2->GetImgWidth()) ||
      (cpp_src1->GetImgWidth() != cpp_dst->GetImgWidth()) ||
      (cpp_src1->GetImgChannel() != cpp_src2->GetImgChannel()) ||
      (cpp_src1->GetImgChannel() != cpp_dst->GetImgChannel())) {
    LOGE("source1/source2/alpha/dst image size do not match!\n");
    return CVI_FAILURE;
  }

  std::vector<CviImg *> inputs = {cpp_src1.get(), cpp_src2.get()};
  std::vector<CviImg *> outputs = {cpp_dst.get()};

  handle_ctx->t_h.t_cmp_sat.init(handle_ctx->rt_handle, handle_ctx->cvk_ctx);
  ret = handle_ctx->t_h.t_cmp_sat.run(handle_ctx->rt_handle, handle_ctx->cvk_ctx, inputs, outputs);
  return ret;
}

CVI_S32 CVI_IVE_Zero(IVE_HANDLE pIveHandle, IVE_DST_IMAGE_S *pstDst) {
  int ret = CVI_IVE_BufRequest(pIveHandle, pstDst);
  CviImg *p_img = reinterpret_cast<CviImg *>(pstDst->tpu_block);
  std::vector<uint32_t> img_coffsets = p_img->GetImgCOffsets();
  for (size_t i = 0; i < img_coffsets.size() - 1; i++) {
    uint32_t plane_size = img_coffsets[i + 1] - img_coffsets[i];
    memset(pstDst->pu8VirAddr[i], 0, plane_size);
  }
  ret |= CVI_IVE_BufFlush(pIveHandle, pstDst);
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
  pstIIDst->u32Width = cpp_img->GetImgWidth();
  pstIIDst->u32Height = cpp_img->GetImgHeight();
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

  CVI_SYS_FreeI(pIveHandle, &src1);
  CVI_SYS_FreeI(pIveHandle, &src2);
  CVI_SYS_FreeI(pIveHandle, &alpha);
  CVI_SYS_FreeI(pIveHandle, &dst);
  return ret;
}
#endif
