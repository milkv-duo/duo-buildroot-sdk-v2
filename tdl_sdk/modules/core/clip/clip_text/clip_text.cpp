#include "clip_text.hpp"
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

namespace cvitdl {

Clip_Text::Clip_Text() : Core(CVI_MEM_SYSTEM) {}

Clip_Text::~Clip_Text() {}

int Clip_Text::inference(VIDEO_FRAME_INFO_S *frame, cvtdl_clip_feature *clip_feature) {
  int32_t *temp_buffer = reinterpret_cast<int32_t *>(frame->stVFrame.pu8VirAddr[0]);
  const TensorInfo &tinfo = getInputTensorInfo(0);
  int32_t *input_ptr = tinfo.get<int32_t>();

  memcpy(input_ptr, temp_buffer, 77 * sizeof(int32_t));

  std::vector<VIDEO_FRAME_INFO_S *> frames = {frame};

  int ret = run(frames);
  if (ret != CVI_TDL_SUCCESS) {
    std::cout << "cvi_tdl clip inference run is fail!\n";
    return ret;
  }

  float *out_feature = getOutputRawPtr<float>(0);

  CVI_SHAPE output_shape = getOutputShape(0);
  clip_feature->feature_dim = output_shape.dim[1];
  clip_feature->out_feature = (float *)malloc(clip_feature->feature_dim * sizeof(float));
  memcpy(clip_feature->out_feature, out_feature, clip_feature->feature_dim * sizeof(float));

  model_timer_.TicToc("post");
  return CVI_TDL_SUCCESS;
}
}  // namespace cvitdl
