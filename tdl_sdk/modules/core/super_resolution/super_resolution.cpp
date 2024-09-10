#include "super_resolution.hpp"
#include <core/core/cvtdl_errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <cmath>
#include <error_msg.hpp>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <vector>
#include "coco_utils.hpp"
#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem.h"
#include "core/cvi_tdl_types_mem_internal.h"
#include "core/utils/vpss_helper.h"
#include "core_utils.hpp"
#include "cvi_sys.h"

#define R_SCALE (0.003922)
#define G_SCALE (0.003922)
#define B_SCALE (0.003922)
#define R_MEAN (0)
#define G_MEAN (0)
#define B_MEAN (0)

namespace cvitdl {

SuperResolution::SuperResolution() : Core(CVI_MEM_DEVICE) {
  m_preprocess_param[0].factor[0] = R_SCALE;
  m_preprocess_param[0].factor[1] = G_SCALE;
  m_preprocess_param[0].factor[2] = B_SCALE;
  m_preprocess_param[0].mean[0] = R_MEAN;
  m_preprocess_param[0].mean[1] = G_MEAN;
  m_preprocess_param[0].mean[2] = B_MEAN;
}

SuperResolution::~SuperResolution() {}

int SuperResolution::inference(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_sr_feature *srfeature) {
  std::vector<VIDEO_FRAME_INFO_S *> frames = {srcFrame};
  printf("SuperResolution running!\n");
  int ret = run(frames);

  srfeature->dstframe = (VIDEO_FRAME_INFO_S *)malloc(sizeof(VIDEO_FRAME_INFO_S));
  if (srfeature->dstframe != nullptr) {
    outputParser(srfeature->dstframe);
  } else {
    printf("Memory allocation for dstframe failed\n");
    LOGE("Memory allocation for dstframe failed\n");
    return -1;
  }
  model_timer_.TicToc("post");
  return CVI_TDL_SUCCESS;
}

void SuperResolution::outputParser(VIDEO_FRAME_INFO_S *dstFrame) {
  float *out = getOutputRawPtr<float>(0);
  CVI_SHAPE output_shape = getOutputShape(0);
  int outShapeC = output_shape.dim[1];
  int outWidth = output_shape.dim[3];
  int outHeight = output_shape.dim[2];

  PIXEL_FORMAT_E format = PIXEL_FORMAT_RGB_888_PLANAR;
  if (CREATE_ION_HELPER(dstFrame, outWidth, outHeight, format, "cvitdl/image") != CVI_SUCCESS) {
    LOGE("alloc ion failed, imgwidth:%d,imgheight:%d\n", outWidth, outHeight);
    return;
  }

  VIDEO_FRAME_S *vFrame = &dstFrame->stVFrame;
  for (int h = 0; h < outHeight; ++h) {
    for (int w = 0; w < outWidth; ++w) {
      for (int c = 0; c < outShapeC; ++c) {
        int index = (c * outHeight * outWidth) + (h * outWidth) + w;
        float value = out[index];
        u_char pixel_value = static_cast<u_char>(std::min(std::max(value * 255.0f, 0.0f), 255.0f));
        int cvChannel = c == 0 ? 2 : (c == 2 ? 0 : 1);  // BGR to RGB conversion if needed
        if (cvChannel == 0) {
          vFrame->pu8VirAddr[0][w + h * vFrame->u32Stride[2]] = pixel_value;
        } else if (cvChannel == 1) {
          vFrame->pu8VirAddr[1][w + h * vFrame->u32Stride[1]] = pixel_value;
        } else {
          vFrame->pu8VirAddr[2][w + h * vFrame->u32Stride[0]] = pixel_value;
        }
      }
    }
  }
}
}  // namespace cvitdl
