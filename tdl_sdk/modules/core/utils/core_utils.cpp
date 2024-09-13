#include "core_utils.hpp"
#include "cvi_tdl_log.hpp"
#ifndef NO_OPENCV
#include "neon_utils.hpp"
#endif
#include <math.h>
#include <string.h>
#include <algorithm>

namespace cvitdl {
void SoftMaxForBuffer(const float *src, float *dst, size_t size) {
  float sum = 0;

  const float max = *std::max_element(src, src + size);

  for (size_t i = 0; i < size; i++) {
    dst[i] = std::exp(src[i] - max);
    sum += dst[i];
  }

  for (size_t i = 0; i < size; i++) {
    dst[i] /= sum;
  }
}

void Dequantize(const int8_t *q_data, float *data, float threshold, size_t size) {
  for (size_t i = 0; i < size; ++i) {
    data[i] = float(q_data[i]) * threshold / 128.0;
  }
}

void DequantizeScale(const int8_t *q_data, float *data, float dequant_scale, size_t size) {
  for (size_t i = 0; i < size; ++i) {
    data[i] = float(q_data[i]) * dequant_scale;
  }
}

void clip_boxes(int width, int height, cvtdl_bbox_t &box) {
  if (box.x1 < 0) {
    box.x1 = 0;
  }
  if (box.y1 < 0) {
    box.y1 = 0;
  }
  if (box.x2 > width - 1) {
    box.x2 = width - 1;
  }
  if (box.y2 > height - 1) {
    box.y2 = height - 1;
  }
}

void NeonQuantizeScale(VIDEO_FRAME_INFO_S *inFrame, const float *qFactor, const float *qMean,
                       VIDEO_FRAME_INFO_S *outFrame) {
#ifdef NO_OPENCV
  LOGE("not supported");
#else
  if (inFrame->stVFrame.enPixelFormat != PIXEL_FORMAT_RGB_888 ||
      outFrame->stVFrame.enPixelFormat != PIXEL_FORMAT_RGB_888_PLANAR) {
    LOGE("Input must be PIXEL_FORMAT_RGB_888. Output must be PIXEL_FORMAT_RGB_888_PLANAR.\n");
    return;
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
  std::vector<cv::Mat> channels;
  for (uint32_t i = 0; i < 3; i++) {
    channels.push_back(cv::Mat(cv::Size(outFrame->stVFrame.u32Width, outFrame->stVFrame.u32Height),
                               CV_8UC1, outFrame->stVFrame.pu8VirAddr[0],
                               outFrame->stVFrame.u32Stride[0]));
  }
  const std::vector<float> mean = {qFactor[0], qFactor[1], qFactor[2]};
  const std::vector<float> factor = {qMean[0], qMean[1], qMean[2]};
  NormalizeAndSplitToU8(image, mean, factor, channels);
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
#endif
}
void mmap_video_frame(VIDEO_FRAME_INFO_S *frame) {
  CVI_U32 f_frame_size =
      frame->stVFrame.u32Length[0] + frame->stVFrame.u32Length[1] + frame->stVFrame.u32Length[2];
  if (frame->stVFrame.pu8VirAddr[0] == NULL) {
    frame->stVFrame.pu8VirAddr[0] =
        (CVI_U8 *)CVI_SYS_Mmap(frame->stVFrame.u64PhyAddr[0], f_frame_size);
    if (frame->stVFrame.u32Length[1] != 0) {
      frame->stVFrame.pu8VirAddr[1] = frame->stVFrame.pu8VirAddr[0] + frame->stVFrame.u32Length[0];
    } else {
      frame->stVFrame.pu8VirAddr[1] = NULL;
    }
    if (frame->stVFrame.u32Length[2] != 0) {
      frame->stVFrame.pu8VirAddr[2] = frame->stVFrame.pu8VirAddr[1] + frame->stVFrame.u32Length[1];
    } else {
      frame->stVFrame.pu8VirAddr[1] = NULL;
    }
  }
}
void unmap_video_frame(VIDEO_FRAME_INFO_S *frame) {
  CVI_U32 f_frame_size =
      frame->stVFrame.u32Length[0] + frame->stVFrame.u32Length[1] + frame->stVFrame.u32Length[2];
  CVI_SYS_Munmap((void *)frame->stVFrame.pu8VirAddr[0], f_frame_size);
  frame->stVFrame.pu8VirAddr[0] = NULL;
  frame->stVFrame.pu8VirAddr[1] = NULL;
  frame->stVFrame.pu8VirAddr[2] = NULL;
}
}  // namespace cvitdl
