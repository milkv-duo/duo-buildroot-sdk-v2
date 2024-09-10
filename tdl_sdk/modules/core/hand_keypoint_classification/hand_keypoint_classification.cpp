#include "hand_keypoint_classification.hpp"
#include <cmath>
#include <iostream>
#include "core/core/cvtdl_errno.h"
#include "cvi_tdl_log.hpp"
using namespace cvitdl;

HandKeypointClassification::HandKeypointClassification() : Core(CVI_MEM_SYSTEM) {}

HandKeypointClassification::~HandKeypointClassification() {}

int HandKeypointClassification::inference(VIDEO_FRAME_INFO_S *stOutFrame,
                                          cvtdl_handpose21_meta_t *handpose) {
  float *temp_buffer = reinterpret_cast<float *>(stOutFrame->stVFrame.pu8VirAddr[0]);
  const TensorInfo &tinfo = getInputTensorInfo(0);
  float qscale = tinfo.qscale;

#ifdef CV186X

  uint8_t *input_ptr = tinfo.get<uint8_t>();
  for (int i = 0; i < 42; i++) {
    float temp_float = qscale * temp_buffer[i];
    if (temp_float < 0)
      input_ptr[i] = 0;
    else if (temp_float > 255)
      input_ptr[i] = 255;
    else
      input_ptr[i] = (uint8_t)std::round(temp_float);
  }

#else

  int8_t *input_ptr = tinfo.get<int8_t>();
  for (int i = 0; i < 42; i++) {
    float temp_float = qscale * temp_buffer[i];
    if (temp_float < -128)
      input_ptr[i] = -128;
    else if (temp_float > 127)
      input_ptr[i] = 128;
    else
      input_ptr[i] = (int8_t)std::round(temp_float);
  }
#endif

  std::vector<VIDEO_FRAME_INFO_S *> frames = {stOutFrame};
  int ret = run(frames);

  if (ret != CVI_TDL_SUCCESS) {
    LOGW("hand keypoint classification inference failed\n");
    return ret;
  }
  const TensorInfo &oinfo = getOutputTensorInfo(0);
  int num_cls = oinfo.shape.dim[1];

  float *out_data = getOutputRawPtr<float>(oinfo.tensor_name);
  int max_index = -1;
  float max_score = -1;
  for (int k = 0; k < num_cls; k++) {
    if (out_data[k] > max_score) {
      max_score = out_data[k];
      max_index = k;
    }
  }
  handpose->label = max_index;
  handpose->score = max_score;
  return CVI_SUCCESS;
}