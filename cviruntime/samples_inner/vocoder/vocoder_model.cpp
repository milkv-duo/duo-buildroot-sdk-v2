#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <opencv2/opencv.hpp>
#include "cviruntime.h"
#include "cnpy.h"
#include "vocoder_model.hpp"

VocoderModel::VocoderModel(const char *model_file) {
  int ret = CVI_NN_RegisterModel(model_file, &model);
  if (ret != CVI_RC_SUCCESS) {
    printf("CVI_NN_RegisterModel failed, err %d\n", ret);
    exit(1);
  }
  CVI_NN_SetConfig(model, OPTION_BATCH_SIZE, 1);
  CVI_NN_SetConfig(model, OPTION_PROGRAM_INDEX, 5);
  ret = CVI_NN_GetInputOutputTensors(model, &input_tensors, &input_num,
                                     &output_tensors, &output_num);
  if (ret != CVI_RC_SUCCESS) {
    printf("CVI_NN_GetInputOutputTensors failed, err %d\n", ret);
    exit(1);
  }
  src = &input_tensors[0];
  output = &output_tensors[0];
  std::cout << "Encoder- tensors:" << src->name
            << ", " << output->name << "\n";
}

VocoderModel::VocoderModel(CVI_MODEL_HANDLE main_model, int32_t pidx) {
  int ret = CVI_NN_CloneModel(main_model, &model);
  if (ret != CVI_RC_SUCCESS) {
    printf("CVI_NN_RegisterModel failed, err %d\n", ret);
    exit(1);
  }
  CVI_NN_SetConfig(model, OPTION_BATCH_SIZE, 1);
  CVI_NN_SetConfig(model, OPTION_PROGRAM_INDEX, pidx);
  ret = CVI_NN_GetInputOutputTensors(model, &input_tensors, &input_num,
                                     &output_tensors, &output_num);
  if (ret != CVI_RC_SUCCESS) {
    printf("CVI_NN_GetInputOutputTensors failed, err %d\n", ret);
    exit(1);
  }
  src = &input_tensors[0];
  output = &output_tensors[0];
  std::cout << "Encoder- tensors:" << src->name
            << ", " << output->name << "\n";
}

float *VocoderModel::run(float *data, int32_t src_size, int32_t &out_size) {
  // fill src_seq to tensor 0
  assert(CVI_NN_TensorCount(src) == src_size);
  CVI_NN_SetTensorPtr(src, data);
  // memcpy(CVI_NN_TensorPtr(src), data, CVI_NN_TensorSize(src));
  // run inference
  CVI_NN_Forward(model, input_tensors, input_num,
                 output_tensors, output_num);
  out_size = CVI_NN_TensorCount(output);
  return (float *)CVI_NN_TensorPtr(output);
}

Vocoder::Vocoder(const char *model_file) {
  vc_600 = new VocoderModel(model_file);
  assert(vc_600);
  vc_500 = new VocoderModel(vc_600->model, 4);
  assert(vc_500);
  vc_400 = new VocoderModel(vc_600->model, 3);
  assert(vc_400);
  vc_300 = new VocoderModel(vc_600->model, 2);
  assert(vc_300);
  vc_200 = new VocoderModel(vc_600->model, 1);
  assert(vc_200);
  vc_100 = new VocoderModel(vc_600->model, 0);
  assert(vc_100);
}

Vocoder::~Vocoder() {
  delete vc_100;
  delete vc_200;
  delete vc_300;
  delete vc_400;
  delete vc_500;
  delete vc_600;
}

float *Vocoder::run(float *data, int32_t src_size,
    int32_t &out_size) {
  VocoderModel *m;
  switch(src_size) {
    case 80 * 100:
      m = vc_100;
      break;
    case 80 * 200:
      m = vc_200;
      break;
    case 80 * 300:
      m = vc_300;
      break;
    case 80 * 400:
      m = vc_400;
      break;
    case 80 * 500:
      m = vc_500;
      break;
    case 80 * 600:
      m = vc_600;
      break;
    default:
      assert(0);
  }
  return m->run(data, src_size, out_size);
}
