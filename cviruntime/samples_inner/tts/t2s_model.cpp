#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "cviruntime.h"
#include "t2s_model.hpp"

namespace t2s {

static inline bf16_t BF16(float val) {
  return ((bf16_t *)(&val))[1];
}

static inline float FP32(bf16_t val) {
  float ret = 0;
  ((bf16_t *)(&ret))[1] = val;
  return ret;
}

static void fill_mask(CVI_TENSOR *tensor, int32_t text_sz) {
  bf16_t *text_mask = (bf16_t *)CVI_NN_TensorPtr(tensor);
  size_t mask_sz = CVI_NN_TensorCount(tensor);
  bf16_t zero = BF16(0);
  bf16_t one = BF16(1);
  for (size_t i = 0; i < text_sz; i++) {
    text_mask[i] = one;
  }
  for (size_t i = text_sz; i < mask_sz; i++) {
    text_mask[i] = zero;
  }
}

T2SEncoder::T2SEncoder(const char *model_file) {
  int ret = CVI_NN_RegisterModel(model_file, &model);
  if (ret != CVI_RC_SUCCESS) {
    printf("CVI_NN_RegisterModel failed, err %d\n", ret);
    exit(1);
  }
  ret = CVI_NN_GetInputOutputTensors(model, &input_tensors, &input_num,
                                                             &output_tensors, &output_num);
  if (ret != CVI_RC_SUCCESS) {
    printf("CVI_NN_GetInputOutputTensors failed, err %d\n", ret);
    exit(1);
  }
  assert(input_num == 4);
  assert(output_num == 2);
  durations = new int32_t[MAX_TEXT_SIZE];
  hiddens = (bf16_t *)CVI_NN_TensorPtr(&output_tensors[0]);
}

int32_t T2SEncoder::run(uint16_t *text, int32_t text_sz, uint16_t *lang, uint16_t *speaker) {
  CVI_NN_SetTensorPtr(&input_tensors[0], text);
  CVI_NN_SetTensorPtr(&input_tensors[2], lang);
  CVI_NN_SetTensorPtr(&input_tensors[3], speaker);
  fill_mask(&input_tensors[1], text_sz);
  // run inference
  CVI_NN_Forward(model, input_tensors, input_num,
                             output_tensors, output_num);
  return regulate_durations(text_sz);
}

int32_t T2SEncoder::regulate_durations(int32_t text_sz) {
  bf16_t *ptr = (bf16_t *)CVI_NN_TensorPtr(&output_tensors[1]);
  int32_t total_duration = 0;
  for (int32_t i = 0; i < text_sz; i++) {
    float d = (int32_t)std::round(FP32(ptr[i]));
    durations[i] = (d <= 0) ? 1 : d;
    total_duration += durations[i];
  }
  for (int32_t i = text_sz; i < MAX_TEXT_SIZE; i++) {
    durations[i] = 0;
  }
  return total_duration;
}

T2SDecoder::T2SDecoder(const char *model_file) {
  int ret = CVI_NN_RegisterModel(model_file, &model);
  if (ret != CVI_RC_SUCCESS) {
    printf("CVI_NN_RegisterModel failed, err %d\n", ret);
    exit(1);
  }
  ret = CVI_NN_GetInputOutputTensors(model, &input_tensors, &input_num,
                                     &output_tensors, &output_num);
  if (ret != CVI_RC_SUCCESS) {
    printf("CVI_NN_GetInputOutputTensors failed, err %d\n", ret);
    exit(1);
  }
  assert(input_num == 4);
  assert(output_num == 1);
}

void T2SDecoder::expand_hidden_states(CVI_TENSOR *tensor, bf16_t *src, int32_t*durations) {
  auto dst = (bf16_t *)CVI_NN_TensorPtr(tensor);
  // expand encoding
  int offset_dst = 0;
  int offset_src = 0;
  for (int i = 0; i < MAX_TEXT_SIZE; i++) {
    for (int j = 0; j < durations[i]; j++) {
      memcpy(dst + offset_dst, src + offset_src, 256 * sizeof(bf16_t));
      offset_dst += 256;
    }
    offset_src += 256;
  }
  memset(dst + offset_dst, 0,  (200 * 256 - offset_dst) * sizeof(bf16_t));
}

float* T2SDecoder::run(bf16_t *hidden_states, int32_t duration,  int32_t *durations) {
  expand_hidden_states(&input_tensors[0], hidden_states, durations);
  fill_mask(&input_tensors[1], duration);
  fill_mask(&input_tensors[2], duration * 2);
  fill_mask(&input_tensors[3], duration * 4);
  // run inference
  CVI_NN_Forward(model, input_tensors, input_num,
                 output_tensors, output_num);
  return (float *)CVI_NN_TensorPtr(&output_tensors[0]);
}

}


