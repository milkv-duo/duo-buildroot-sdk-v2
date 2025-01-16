#include "occlusion_classification.hpp"
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <error_msg.hpp>
#include <numeric>
#include <string>
#include <vector>
#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem_internal.h"
#include "core/face/cvtdl_face_types.h"
#include "core/object/cvtdl_object_types.h"
#include "core/utils/vpss_helper.h"
#include "core_utils.hpp"
#include "cvi_comm.h"
#include "misc.hpp"
#define OCCULUSION_CLASSIFICATION_FACTOR (float)(1 / 255.0)
#define OCCULUSION_CLASSIFICATION_MEAN (0.0)

namespace cvitdl {

OcclusionClassification::OcclusionClassification() : Core(CVI_MEM_DEVICE) {
  for (uint32_t i = 0; i < 3; i++) {
    m_preprocess_param[0].factor[i] = OCCULUSION_CLASSIFICATION_FACTOR;
    m_preprocess_param[0].mean[i] = OCCULUSION_CLASSIFICATION_MEAN;
  }
  m_preprocess_param[0].format = PIXEL_FORMAT_RGB_888_PLANAR;
  m_preprocess_param[0].keep_aspect_ratio = false;
  m_preprocess_param[0].use_crop = false;
#ifndef __CV186X__  
  m_preprocess_param[0].resize_method = VPSS_SCALE_COEF_OPENCV_BILINEAR;
#endif
}
OcclusionClassification::~OcclusionClassification() {}

int OcclusionClassification::inference(VIDEO_FRAME_INFO_S *frame,
                                       cvtdl_class_meta_t *occlusion_classification_meta) {
  std::vector<VIDEO_FRAME_INFO_S *> frames = {frame};
  int ret = run(frames);
  if (ret != CVI_TDL_SUCCESS) {
    return ret;
  }

  float *score = getOutputRawPtr<float>(0);
  double sum = score[0] + score[1];
  double k = 1 / sum;

  score[0] = k * score[0];
  score[1] = k * score[1];

  occlusion_classification_meta->score[0] = score[0];
  occlusion_classification_meta->score[1] = score[1];

  return CVI_TDL_SUCCESS;
}

}  // namespace cvitdl