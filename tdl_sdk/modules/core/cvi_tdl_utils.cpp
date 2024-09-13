#include "core/cvi_tdl_utils.h"
#include "cvi_tdl_core_internal.hpp"

#include "core/cvi_tdl_core.h"
#include "core/cvi_tdl_types_mem_internal.h"
#include "core/error_msg.hpp"
#include "core/utils/vpss_helper.h"
#include "utils/core_utils.hpp"

#ifndef NO_OPENCV
#include "utils/face_utils.hpp"
#include "utils/image_utils.hpp"
#endif
#include <string>

/** NOTE: If turn on DO_ALIGN_STRIDE, we can not copy the data from cv::Mat directly. */
/** TODO: If ALIGN is not necessary in TDL SDK, remove it in the future. */
#define DO_ALIGN_STRIDE
#ifdef DO_ALIGN_STRIDE
#define GET_TDL_IMAGE_STRIDE(x) (ALIGN((x), DEFAULT_ALIGN))
#else
#define GET_TDL_IMAGE_STRIDE(x) (x)
#endif

CVI_S32 CVI_TDL_Dequantize(const int8_t *quantizedData, float *data, const uint32_t bufferSize,
                           const float dequantizeThreshold) {
  cvitdl::Dequantize(quantizedData, data, dequantizeThreshold, bufferSize);
  return CVI_TDL_SUCCESS;
}
CVI_S32 CVI_TDL_SoftMax(const float *inputBuffer, float *outputBuffer, const uint32_t bufferSize) {
  cvitdl::SoftMaxForBuffer(inputBuffer, outputBuffer, bufferSize);
  return CVI_TDL_SUCCESS;
}

template <typename T, typename U>
inline CVI_S32 CVI_TDL_NMS(const T *input, T *nms, const float threshold, const char method) {
  if (method != 'u' && method != 'm') {
    LOGE("Unsupported NMS method. Only supports u or m");
    return CVI_TDL_FAILURE;
  }
  std::vector<U> bboxes;
  std::vector<U> bboxes_nms;
  for (uint32_t i = 0; i < input->size; i++) {
    bboxes.push_back(input->info[i]);
  }
  cvitdl::NonMaximumSuppression(bboxes, bboxes_nms, threshold, method);
  CVI_TDL_Free(nms);
  nms->size = bboxes.size();
  nms->width = input->width;
  nms->height = input->height;
  nms->info = (U *)malloc(nms->size * sizeof(U));
  for (unsigned int i = 0; i < nms->size; i++) {
    CVI_TDL_CopyInfoCpp(&bboxes_nms[i], &nms->info[i]);
  }
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_FaceNMS(const cvtdl_face_t *face, cvtdl_face_t *faceNMS, const float threshold,
                        const char method) {
  return CVI_TDL_NMS<cvtdl_face_t, cvtdl_face_info_t>(face, faceNMS, threshold, method);
}

CVI_S32 CVI_TDL_ObjectNMS(const cvtdl_object_t *obj, cvtdl_object_t *objNMS, const float threshold,
                          const char method) {
  return CVI_TDL_NMS<cvtdl_object_t, cvtdl_object_info_t>(obj, objNMS, threshold, method);
}

CVI_S32 CVI_TDL_FaceAlignment(VIDEO_FRAME_INFO_S *inFrame, const uint32_t metaWidth,
                              const uint32_t metaHeight, const cvtdl_face_info_t *info,
                              VIDEO_FRAME_INFO_S *outFrame, const bool enableGDC) {
#ifdef NO_OPENCV
  return CVI_TDL_FAILURE;
#else
  if (enableGDC) {
    if (inFrame->stVFrame.enPixelFormat != PIXEL_FORMAT_RGB_888_PLANAR &&
        inFrame->stVFrame.enPixelFormat != PIXEL_FORMAT_YUV_PLANAR_420) {
      LOGE(
          "Supported format are PIXEL_FORMAT_RGB_888_PLANAR, PIXEL_FORMAT_YUV_PLANAR_420. Current: "
          "%x\n",
          inFrame->stVFrame.enPixelFormat);
      return CVI_TDL_FAILURE;
    }
    cvtdl_face_info_t face_info = cvitdl::info_rescale_c(
        metaWidth, metaHeight, inFrame->stVFrame.u32Width, inFrame->stVFrame.u32Height, *info);
    cvitdl::face_align_gdc(inFrame, outFrame, face_info);
  } else {
    if (inFrame->stVFrame.enPixelFormat != PIXEL_FORMAT_RGB_888) {
      LOGE("Supported format is PIXEL_FORMAT_RGB_888. Current: %x\n",
           inFrame->stVFrame.enPixelFormat);
      return CVI_TDL_FAILURE;
    }
    bool do_unmap_in = false, do_unmap_out = false;
    if (inFrame->stVFrame.pu8VirAddr[0] == NULL) {
      inFrame->stVFrame.pu8VirAddr[0] =
          (CVI_U8 *)CVI_SYS_Mmap(inFrame->stVFrame.u64PhyAddr[0], inFrame->stVFrame.u32Length[0]);
      do_unmap_in = true;
    }
    if (outFrame->stVFrame.pu8VirAddr[0] == NULL) {
      outFrame->stVFrame.pu8VirAddr[0] =
          (CVI_U8 *)CVI_SYS_Mmap(outFrame->stVFrame.u64PhyAddr[0], outFrame->stVFrame.u32Length[0]);
      do_unmap_out = true;
    }
    cv::Mat image(cv::Size(inFrame->stVFrame.u32Width, inFrame->stVFrame.u32Height), CV_8UC3,
                  inFrame->stVFrame.pu8VirAddr[0], inFrame->stVFrame.u32Stride[0]);
    cv::Mat warp_image(cv::Size(outFrame->stVFrame.u32Width, outFrame->stVFrame.u32Height),
                       image.type(), outFrame->stVFrame.pu8VirAddr[0],
                       outFrame->stVFrame.u32Stride[0]);
    cvtdl_face_info_t face_info = cvitdl::info_rescale_c(
        metaWidth, metaHeight, inFrame->stVFrame.u32Width, inFrame->stVFrame.u32Height, *info);
    cvitdl::face_align(image, warp_image, face_info);
    CVI_SYS_IonFlushCache(outFrame->stVFrame.u64PhyAddr[0], outFrame->stVFrame.pu8VirAddr[0],
                          outFrame->stVFrame.u32Length[0]);
    if (do_unmap_in) {
      CVI_SYS_Munmap((void *)inFrame->stVFrame.pu8VirAddr[0], inFrame->stVFrame.u32Length[0]);
      inFrame->stVFrame.pu8VirAddr[0] = NULL;
    }
    if (do_unmap_out) {
      CVI_SYS_Munmap((void *)outFrame->stVFrame.pu8VirAddr[0], outFrame->stVFrame.u32Length[0]);
      outFrame->stVFrame.pu8VirAddr[0] = NULL;
    }
  }
  return CVI_TDL_SUCCESS;
#endif
}

CVI_S32 CVI_TDL_Face_Quality_Laplacian(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_face_info_t *face_info,
                                       float *score) {
#ifdef NO_OPENCV
  return CVI_TDL_FAILURE;
#else
  return cvitdl::face_quality_laplacian(srcFrame, face_info, score);
#endif
}

CVI_S32 CVI_TDL_CreateImage(cvtdl_image_t *image, uint32_t height, uint32_t width,
                            PIXEL_FORMAT_E fmt) {
  if (fmt != PIXEL_FORMAT_RGB_888 && fmt != PIXEL_FORMAT_RGB_888_PLANAR &&
      fmt != PIXEL_FORMAT_NV21 && fmt != PIXEL_FORMAT_YUV_PLANAR_420) {
    LOGE("Pixel format [%d] is not supported.\n", fmt);
    return CVI_TDL_ERR_INVALID_ARGS;
  }
  if (image->pix[0] != NULL) {
    LOGE("destination image is not empty.");
    return CVI_TDL_ERR_INVALID_ARGS;
  }
  image->pix_format = fmt;
  image->height = height;
  image->width = width;

  /* NOTE: Refer to vpss_helper.h*/
  switch (fmt) {
    case PIXEL_FORMAT_RGB_888: {
      image->stride[0] = GET_TDL_IMAGE_STRIDE(image->width * 3);
      image->stride[1] = 0;
      image->stride[2] = 0;
      image->length[0] = image->stride[0] * image->height;
      image->length[1] = 0;
      image->length[2] = 0;
    } break;
    case PIXEL_FORMAT_RGB_888_PLANAR: {
      image->stride[0] = GET_TDL_IMAGE_STRIDE(image->width);
      image->stride[1] = GET_TDL_IMAGE_STRIDE(image->width);
      image->stride[2] = GET_TDL_IMAGE_STRIDE(image->width);
      image->length[0] = image->stride[0] * image->height;
      image->length[1] = image->stride[1] * image->height;
      image->length[2] = image->stride[2] * image->height;
    } break;
    case PIXEL_FORMAT_NV21: {
      image->stride[0] = GET_TDL_IMAGE_STRIDE(image->width);
      image->stride[1] = GET_TDL_IMAGE_STRIDE(image->width);
      image->stride[2] = 0;
      image->length[0] = image->stride[0] * image->height;
      image->length[1] = image->stride[0] * (image->height >> 1);
      image->length[2] = 0;
    } break;
    case PIXEL_FORMAT_YUV_PLANAR_420: {
      image->stride[0] = GET_TDL_IMAGE_STRIDE(image->width);
      image->stride[1] = GET_TDL_IMAGE_STRIDE(image->width >> 1);
      image->stride[2] = GET_TDL_IMAGE_STRIDE(image->width >> 1);
      image->length[0] = image->stride[0] * image->height;
      image->length[1] = image->stride[1] * (image->height >> 1);
      image->length[2] = image->stride[2] * (image->height >> 1);
    } break;
    default:
      LOGE("Currently unsupported format %u\n", fmt);
      return CVI_TDL_ERR_INVALID_ARGS;
  }

  uint32_t image_size = image->length[0] + image->length[1] + image->length[2];
  image->pix[0] = (uint8_t *)malloc(image_size);
  memset(image->pix[0], 0, image_size);
  switch (fmt) {
    case PIXEL_FORMAT_RGB_888: {
      image->pix[1] = NULL;
      image->pix[2] = NULL;
    } break;
    case PIXEL_FORMAT_RGB_888_PLANAR: {
      image->pix[1] = image->pix[0] + image->length[0];
      image->pix[2] = image->pix[1] + image->length[1];
    } break;
    case PIXEL_FORMAT_NV21: {
      image->pix[1] = image->pix[0] + image->length[0];
      image->pix[2] = NULL;
    } break;
    case PIXEL_FORMAT_YUV_PLANAR_420: {
      image->pix[1] = image->pix[0] + image->length[0];
      image->pix[2] = image->pix[1] + image->length[1];
    } break;
    default:
      LOGE("Currently unsupported format %u\n", fmt);
      return CVI_TDL_ERR_INVALID_ARGS;
  }

#if 0
  LOGI("[create image] format[%d], height[%u], width[%u], stride[%u][%u][%u], length[%u][%u][%u]\n",
       image->pix_format, image->height, image->width, image->stride[0], image->stride[1],
       image->stride[2], image->length[0], image->length[1], image->length[2]);
#endif

  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_CreateImageFromVideoFrameSize(const VIDEO_FRAME_INFO_S *p_crop_frame,
                                              cvtdl_image_t *image) {
  PIXEL_FORMAT_E fmt = p_crop_frame->stVFrame.enPixelFormat;
  if (fmt != PIXEL_FORMAT_RGB_888) {
    LOGE("Pixel format [%d] is not supported.\n", fmt);
    return CVI_TDL_ERR_INVALID_ARGS;
  }
  if (image->pix[0] != NULL) {
    LOGE("destination image is not empty.");
    return CVI_TDL_ERR_INVALID_ARGS;
  }

  memset(image, 0, sizeof(cvtdl_image_t));
  image->pix_format = fmt;
  image->height = p_crop_frame->stVFrame.u32Width;
  image->width = p_crop_frame->stVFrame.u32Height;
  image->stride[0] = p_crop_frame->stVFrame.u32Stride[0];
  image->length[0] = p_crop_frame->stVFrame.u32Length[0];
  image->pix[0] = (uint8_t *)malloc(image->length[0]);

  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_CreateImageFromVideoFrame(const VIDEO_FRAME_INFO_S *p_src_frame,
                                          cvtdl_image_t *image) {
  PIXEL_FORMAT_E fmt = p_src_frame->stVFrame.enPixelFormat;

  if (fmt != PIXEL_FORMAT_RGB_888 && fmt != PIXEL_FORMAT_RGB_888_PLANAR &&
      fmt != PIXEL_FORMAT_NV21 && fmt != PIXEL_FORMAT_YUV_PLANAR_420) {
    LOGE("Pixel format [%d] is not supported.\n", fmt);
    return CVI_TDL_ERR_INVALID_ARGS;
  }
  if (image->pix[0] != NULL) {
    LOGE("destination image is not empty.");
    return CVI_TDL_ERR_INVALID_ARGS;
  }
  memset(image, 0, sizeof(cvtdl_image_t));
  image->pix_format = fmt;
  image->height = p_src_frame->stVFrame.u32Height;
  image->width = p_src_frame->stVFrame.u32Width;

  /* NOTE: Refer to vpss_helper.h*/
  int num_plane = 0;
  if (fmt == PIXEL_FORMAT_RGB_888) {
    num_plane = 1;
  } else if (fmt == PIXEL_FORMAT_RGB_888_PLANAR) {
    num_plane = 3;
  } else if (fmt == PIXEL_FORMAT_NV21) {
    num_plane = 2;
  } else if (fmt == PIXEL_FORMAT_YUV_PLANAR_420) {
    num_plane = 3;
  }
  for (int i = 0; i < num_plane; i++) {
    image->stride[i] = p_src_frame->stVFrame.u32Stride[i];
    image->length[i] = p_src_frame->stVFrame.u32Length[i];
  }
  uint32_t image_size = image->length[0] + image->length[1] + image->length[2];
  image->pix[0] = (uint8_t *)malloc(image_size);
  for (int i = 1; i < num_plane; i++) {
    image->pix[i] = image->pix[i - 1] + image->length[i - 1];
  }

#if 1
  printf(
      "[create image] format[%d], height[%u], width[%u], stride[%u][%u][%u], length[%u][%u][%u]\n",
      image->pix_format, image->height, image->width, image->stride[0], image->stride[1],
      image->stride[2], image->length[0], image->length[1], image->length[2]);
#endif

  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_EstimateImageSize(uint64_t *size, uint32_t height, uint32_t width,
                                  PIXEL_FORMAT_E fmt) {
  *size = 0;
  switch (fmt) {
    case PIXEL_FORMAT_RGB_888:
    case PIXEL_FORMAT_RGB_888_PLANAR: {
      *size += GET_TDL_IMAGE_STRIDE(width * 3) * height;
    } break;
    case PIXEL_FORMAT_YUV_PLANAR_420:
    case PIXEL_FORMAT_NV21: {
      *size += GET_TDL_IMAGE_STRIDE(width) * height;
      *size += GET_TDL_IMAGE_STRIDE(width) * (height >> 1);
    } break;
    default:
      LOGE("Currently unsupported format %u\n", fmt);
      return CVI_TDL_ERR_INVALID_ARGS;
  }
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_LoadBinImage(const char *filepath, VIDEO_FRAME_INFO_S *frame,
                             PIXEL_FORMAT_E format) {
  // if(frame->stVFrame.enPixelFormat != PIXE)
  if (format != PIXEL_FORMAT_RGB_888_PLANAR) {
    LOGE("Pixel format [%d] is not supported.\n", format);
    return CVI_FAILURE;
  }

  FILE *fp = fopen(filepath, "rb");
  if (fp == nullptr) {
    LOGE("failed to open: %s.\n", filepath);
    return CVI_FAILURE;
  }
  int width = 0;
  int height = 0;

  fseek(fp, 0, SEEK_END);
  long len = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  fread(&width, sizeof(uint32_t), 1, fp);
  fread(&height, sizeof(uint32_t), 1, fp);

  long left_len = len - 8;
  uint8_t *p_data = new uint8_t[left_len];
  fread(p_data, left_len, 1, fp);
  fclose(fp);
  LOGI("load bin image,width:%d,height:%d,imagesize:%ld\n", width, height, left_len);

  if (CREATE_ION_HELPER(frame, width, height, format, "cvitdl/image") != CVI_TDL_SUCCESS) {
    LOGE("alloc ion failed.\n");
    delete[] p_data;
    return CVI_TDL_FAILURE;
  }

  int ret = CVI_SUCCESS;
  int len_per_plane = left_len / 3;
  for (int i = 0; i < 3; i++) {
    const uint8_t *p_plane_src = p_data + i * len_per_plane;
    memcpy(frame->stVFrame.pu8VirAddr[i], p_plane_src, len_per_plane);
  }
  delete[] p_data;
  return ret;
}
CVI_S32 CVI_TDL_DestroyImage(VIDEO_FRAME_INFO_S *frame) {
  CVI_S32 ret = CVI_SUCCESS;
  if (frame->stVFrame.u64PhyAddr[0] != 0) {
    ret = CVI_SYS_IonFree(frame->stVFrame.u64PhyAddr[0], frame->stVFrame.pu8VirAddr[0]);
    frame->stVFrame.u64PhyAddr[0] = (CVI_U64)0;
    frame->stVFrame.u64PhyAddr[1] = (CVI_U64)0;
    frame->stVFrame.u64PhyAddr[2] = (CVI_U64)0;
    frame->stVFrame.pu8VirAddr[0] = NULL;
    frame->stVFrame.pu8VirAddr[1] = NULL;
    frame->stVFrame.pu8VirAddr[2] = NULL;
  }
  return ret;
}

CVI_S32 CVI_TDL_DumpVpssFrame(const char *filepath, VIDEO_FRAME_INFO_S *frame) {
  VIDEO_FRAME_S *pstVFSrc = &frame->stVFrame;
  int channels = 1;
  int num_pixels = 1;
  switch (pstVFSrc->enPixelFormat) {
    case PIXEL_FORMAT_YUV_400: {
      channels = 1;
    } break;
    case PIXEL_FORMAT_RGB_888_PLANAR: {
      channels = 3;
    } break;
    case PIXEL_FORMAT_RGB_888: {
      channels = 1;
      num_pixels = 3;
    } break;
    case PIXEL_FORMAT_NV21: {
      channels = 2;
    } break;
    default: {
      LOGE("Unsupported conversion type: %u.\n", pstVFSrc->enPixelFormat);
      return CVI_FAILURE;
    } break;
  }

  FILE *fp = fopen(filepath, "wb");
  if (fp == nullptr) {
    LOGE("failed to open: %s.\n", filepath);
    return CVI_FAILURE;
  }
  uint32_t width = pstVFSrc->u32Width;
  uint32_t height = pstVFSrc->u32Height;
  fwrite(&width, sizeof(uint32_t), 1, fp);
  fwrite(&height, sizeof(uint32_t), 1, fp);
  uint32_t pix_format = (uint32_t)pstVFSrc->enPixelFormat;
  fwrite(&pix_format, sizeof(pix_format), 1, fp);
  for (int c = 0; c < channels; c++) {
    uint8_t *ptr_plane = pstVFSrc->pu8VirAddr[c];
    if (width * num_pixels == pstVFSrc->u32Stride[c]) {
      fwrite(pstVFSrc->pu8VirAddr[c], pstVFSrc->u32Stride[c] * height, 1, fp);
    } else {
      for (uint32_t r = 0; r < height; r++) {
        fwrite(ptr_plane, width * num_pixels, 1, fp);
        ptr_plane += pstVFSrc->u32Stride[c];
      }
    }
  }
  fclose(fp);
  return CVI_SUCCESS;
}
CVI_S32 CVI_TDL_DumpImage(const char *filepath, cvtdl_image_t *image) {
  int channels = 1;
  int num_pixels = 1;
  switch (image->pix_format) {
    case PIXEL_FORMAT_YUV_400: {
      channels = 1;
    } break;
    case PIXEL_FORMAT_RGB_888_PLANAR: {
      channels = 3;
    } break;
    case PIXEL_FORMAT_RGB_888: {
      channels = 1;
      num_pixels = 3;
    } break;
    case PIXEL_FORMAT_NV21: {
      channels = 2;
    } break;
    default: {
      LOGE("Unsupported conversion type: %u.\n", image->pix_format);
      return CVI_FAILURE;
    } break;
  }

  FILE *fp = fopen(filepath, "wb");
  if (fp == nullptr) {
    LOGE("failed to open: %s.\n", filepath);
    return CVI_FAILURE;
  }
  uint32_t width = image->width;
  uint32_t height = image->height;
  fwrite(&width, sizeof(uint32_t), 1, fp);
  fwrite(&height, sizeof(uint32_t), 1, fp);
  uint32_t pix_format = (uint32_t)image->pix_format;
  fwrite(&pix_format, sizeof(pix_format), 1, fp);
  for (int c = 0; c < channels; c++) {
    uint8_t *ptr_plane = image->pix[c];
    if (width * num_pixels == image->stride[c]) {
      fwrite(ptr_plane, image->stride[c] * height, 1, fp);
    } else {
      for (uint32_t r = 0; r < height; r++) {
        fwrite(ptr_plane, width * num_pixels, 1, fp);
        ptr_plane += image->stride[c];
      }
    }
  }
  fclose(fp);
  return CVI_SUCCESS;
}
CVI_S32 CVI_TDL_StbReadImage(const char *filepath, VIDEO_FRAME_INFO_S *frame,
                             PIXEL_FORMAT_E format) {
  // int desiredNChannels = -1;
  // switch (enType) {
  //   case IVE_IMAGE_TYPE_S8C1:
  //   case IVE_IMAGE_TYPE_U8C1:
  //     desiredNChannels = 1;
  //     break;
  //   case IVE_IMAGE_TYPE_S8C3_PLANAR:
  //   case IVE_IMAGE_TYPE_U8C3_PLANAR:
  //     desiredNChannels = 3;
  //     break;
  //   case IVE_IMAGE_TYPE_S8C3_PACKAGE:
  //   case IVE_IMAGE_TYPE_U8C3_PACKAGE:
  //     desiredNChannels = STBI_rgb;
  //     break;
  //   default:
  //     LOGE("Not support channel %s.\n", cviIveImgEnTypeStr[enType]);
  //     break;
  // }
  // LOGI("to read image:%s,type:%d,channels:%d", filename, enType, desiredNChannels);
  // int width, height, nChannels;
  // stbi_uc *stbi_data = stbi_load(filename, &width, &height, &nChannels, desiredNChannels);
  // if (stbi_data == nullptr) {
  //   LOGE("Image %s read failed.\n", filename);
  //   return img;
  // }
  // LOGI("to create cviimage,channels, width, height: %d %d %d\n", desiredNChannels, width,
  // height); CVI_IVE_CreateImage(pIveHandle, &img, enType, width, height); LOGI("desiredNChannels,
  // width, height: %d %d %d\n", desiredNChannels, width, height); if (enType ==
  // IVE_IMAGE_TYPE_U8C3_PLANAR || enType == IVE_IMAGE_TYPE_S8C3_PLANAR) {
  //   for (size_t i = 0; i < (size_t)height; i++) {
  //     for (size_t j = 0; j < (size_t)width; j++) {
  //       size_t stb_idx = (i * width + j) * 3;
  //       size_t img_idx = (i * img.u16Stride[0] + j);
  //       img.pu8VirAddr[0][img_idx] = stbi_data[stb_idx];
  //       img.pu8VirAddr[1][img_idx] = stbi_data[stb_idx + 1];
  //       img.pu8VirAddr[2][img_idx] = stbi_data[stb_idx + 2];
  //     }
  //   }
  // } else {
  //   if (invertPackage &&
  //       (enType == IVE_IMAGE_TYPE_U8C3_PACKAGE || enType == IVE_IMAGE_TYPE_S8C3_PACKAGE)) {
  //     for (size_t i = 0; i < (size_t)height; i++) {
  //       uint32_t stb_stride = i * width * 3;
  //       uint32_t image_stride = (i * img.u16Stride[0]);
  //       for (size_t j = 0; j < (size_t)width; j++) {
  //         uint32_t stb_idx = stb_stride + (j * 3);
  //         uint32_t img_idx = image_stride + (j * 3);
  //         img.pu8VirAddr[0][img_idx] = stbi_data[stb_idx + 2];
  //         img.pu8VirAddr[0][img_idx + 1] = stbi_data[stb_idx + 1];
  //         img.pu8VirAddr[0][img_idx + 2] = stbi_data[stb_idx];
  //       }
  //     }
  //   } else {
  //     stbi_uc *ptr = stbi_data;
  //     for (size_t j = 0; j < (size_t)height; j++) {
  //       memcpy(img.pu8VirAddr[0] + (j * img.u16Stride[0]), ptr, width * desiredNChannels);
  //       ptr += width * desiredNChannels;
  //     }
  //   }
  // }
  // CVI_IVE_BufFlush(pIveHandle, &img);
  // stbi_image_free(stbi_data);
  // return img;
  return 0;
}