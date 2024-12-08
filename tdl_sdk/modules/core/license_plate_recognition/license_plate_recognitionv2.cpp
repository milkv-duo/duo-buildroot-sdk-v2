
#include "license_plate_recognitionv2.hpp"
#include <iostream>
#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem.h"
#include "core/face/cvtdl_face_types.h"
#include "rescale_utils.hpp"
#include "opencv2/imgproc.hpp"
#include "core/utils/vpss_helper.h"
#define SCALE (1 / 128.)
#define MEAN (127.5 / 128.)

#define OUTPUT_NAME_PROBABILITY "output_ReduceMean_f32"

namespace cvitdl {

std::vector<std::string> CHARS = {
         "京", "沪", "津", "渝", "冀", "晋", "蒙", "辽", "吉", "黑",
         "苏", "浙", "皖", "闽", "赣", "鲁", "豫", "鄂", "湘", "粤",
         "桂", "琼", "川", "贵", "云", "藏", "陕", "甘", "青", "宁",
         "新", "学", "警", "港", "澳", "挂", "使", "领", "民", "深",
         "危", "险", "空",
         "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
         "A", "B", "C", "D", "E", "F", "G", "H", "J", "K",
         "L", "M", "N", "P", "Q", "R", "S", "T", "U", "V",
         "W", "X", "Y", "Z", "I", "O", "-"};
LicensePlateRecognitionV2::LicensePlateRecognitionV2(): LicensePlateRecognitionBase(CVI_MEM_DEVICE){
  for (int i = 0; i < 3; i++) {
    m_preprocess_param[0].factor[i] = SCALE;
    m_preprocess_param[0].mean[i] = MEAN;
  }
  m_preprocess_param[0].resize_method = VPSS_SCALE_COEF_BILINEAR;
  m_preprocess_param[0].format = PIXEL_FORMAT_BGR_888_PLANAR;
  m_preprocess_param[0].keep_aspect_ratio = false;
}

void LicensePlateRecognitionV2::greedy_decode(float *prebs,cvtdl_vehicle_meta *v_meta) {
  CVI_SHAPE shape = getOutputShape(0);
  // 80，18
  int rows = shape.dim[1];
  int cols = shape.dim[2];
  int index[cols];
  // argmax index
  for (int i = 0; i < cols; i++) {
    float max = prebs[i];
    int maxIndex = 0;
    for (int j = 0; j < rows; j++) {
      if (prebs[i + j * cols] > max) {
        max = prebs[i + j * cols];
        maxIndex = j;
      }
    }
    index[i] = maxIndex;
  }
  std::vector<int> no_repeat_blank_label;
  uint32_t pre_c = index[0];
  if (pre_c != CHARS.size() - 1) {
    no_repeat_blank_label.push_back(pre_c);
  }
  for (int i = 0; i < cols; i++) {
    uint32_t c = index[i];
    if ((pre_c == c) || (c == CHARS.size() - 1)) {
      if (c == CHARS.size() - 1) {
        pre_c = c;
      }
      continue;
    }
    no_repeat_blank_label.push_back(c);
    pre_c = c;
  }
  std::string lb;
  for (int k : no_repeat_blank_label) {
    lb += CHARS[k];
  }
  strncpy(v_meta->license_char, lb.c_str(), sizeof(v_meta->license_char));
}

void BufferRGBPacked2PlanarCopy(const uint8_t *buffer, uint32_t width, uint32_t height,
                                       uint32_t stride, VIDEO_FRAME_INFO_S *frame, bool invert) {
  VIDEO_FRAME_S *vFrame = &frame->stVFrame;
  if (invert) {
    for (uint32_t j = 0; j < height; j++) {
      const uint8_t *ptr = buffer + j * stride;
      for (uint32_t i = 0; i < width; i++) {
        const uint8_t *ptr_pxl = i * 3 + ptr;
        vFrame->pu8VirAddr[0][i + j * vFrame->u32Stride[0]] = ptr_pxl[2];
        vFrame->pu8VirAddr[1][i + j * vFrame->u32Stride[1]] = ptr_pxl[1];
        vFrame->pu8VirAddr[2][i + j * vFrame->u32Stride[2]] = ptr_pxl[0];
      }
    }
  } else {
    for (uint32_t j = 0; j < height; j++) {
      const uint8_t *ptr = buffer + j * stride;
      for (uint32_t i = 0; i < width; i++) {
        const uint8_t *ptr_pxl = i * 3 + ptr;
        vFrame->pu8VirAddr[0][i + j * vFrame->u32Stride[0]] = ptr_pxl[0];
        vFrame->pu8VirAddr[1][i + j * vFrame->u32Stride[1]] = ptr_pxl[1];
        vFrame->pu8VirAddr[2][i + j * vFrame->u32Stride[2]] = ptr_pxl[2];
      }
    }
  }
}
int LicensePlateRecognitionV2::inference(VIDEO_FRAME_INFO_S *frame, cvtdl_object_t *vehicle_plate_meta) {
  if (frame->stVFrame.enPixelFormat != PIXEL_FORMAT_RGB_888 &&
      frame->stVFrame.enPixelFormat != PIXEL_FORMAT_BGR_888) {
    LOGE(
        "Error: pixel format not match PIXEL_FORMAT_RGB_888 or PIXEL_FORMAT_BGR");
    return CVI_TDL_ERR_INVALID_ARGS;
  }
  frame->stVFrame.pu8VirAddr[0] =
      (CVI_U8 *)CVI_SYS_Mmap(frame->stVFrame.u64PhyAddr[0], frame->stVFrame.u32Length[0]);
  cv::Mat src(frame->stVFrame.u32Height, frame->stVFrame.u32Width, CV_8UC3,
                  frame->stVFrame.pu8VirAddr[0], frame->stVFrame.u32Stride[0]);    
  for (size_t n = 0; n < vehicle_plate_meta->size; n++) {
    cvtdl_vehicle_meta *v_meta = vehicle_plate_meta->info[n].vehicle_properity;
    cv::Point2f srcTri[3];
    srcTri[0] = cv::Point2f(v_meta->license_pts.x[0], v_meta->license_pts.y[0]);
    srcTri[1] = cv::Point2f(v_meta->license_pts.x[1], v_meta->license_pts.y[1]);
    srcTri[2] = cv::Point2f(v_meta->license_pts.x[3], v_meta->license_pts.y[3]);
    cv::Point2f dstTri[3];
    dstTri[0] = cv::Point2f(0.f, 0.f);
    dstTri[1] = cv::Point2f(94.f, 0.f); // x = 93 since the width is 94 pixels
    dstTri[2] = cv::Point2f(0.f, 24.f); // y = 23 since the height is 24 pixels
    cv::Mat warpMat = cv::getAffineTransform(srcTri, dstTri);
    cv::Mat dst;
    cv::warpAffine(src, dst, warpMat, cv::Size(94, 24));
    VIDEO_FRAME_INFO_S align_frame;
    if (CREATE_ION_HELPER(&align_frame, dst.cols, dst.rows, PIXEL_FORMAT_RGB_888_PLANAR, "cvitdl/image") != CVI_TDL_SUCCESS) {
      printf("alloc ion failed, imgwidth:%d,imgheight:%d\n", dst.cols, dst.rows);
    }
    BufferRGBPacked2PlanarCopy(dst.data, dst.cols, dst.rows, dst.step, &align_frame, true);
    std::vector<VIDEO_FRAME_INFO_S *> frames = {&align_frame};
    int ret = run(frames);
    if (ret != CVI_SUCCESS) {
      return ret;
    }
    float *out = getOutputRawPtr<float>(0);
    greedy_decode(out,v_meta);
    if (align_frame.stVFrame.u64PhyAddr[0] != 0) {
      CVI_SYS_IonFree(align_frame.stVFrame.u64PhyAddr[0], align_frame.stVFrame.pu8VirAddr[0]);
      align_frame.stVFrame.u64PhyAddr[0] = (CVI_U64)0;
      align_frame.stVFrame.u64PhyAddr[1] = (CVI_U64)0;
      align_frame.stVFrame.u64PhyAddr[2] = (CVI_U64)0;
      align_frame.stVFrame.pu8VirAddr[0] = NULL;
      align_frame.stVFrame.pu8VirAddr[1] = NULL;
      align_frame.stVFrame.pu8VirAddr[2] = NULL;
    }
  }
  CVI_SYS_Munmap((void *)frame->stVFrame.pu8VirAddr[0], frame->stVFrame.u32Length[0]);
  return CVI_SUCCESS;
}
}  // namespace cvitdl