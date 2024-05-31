#ifndef __SAMPLES_MT_MODEL_HPP
#define __SAMPLES_MT_MODEL_HPP

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <opencv2/opencv.hpp>
#include "cviruntime.h"
#include "cnpy.h"

typedef uint16_t bf16_t;

class VocoderModel {
public:
  VocoderModel(const char *model_file);
  VocoderModel(CVI_MODEL_HANDLE main_model, int32_t pidx);
  ~VocoderModel() {
    if (model) {
      CVI_NN_CleanupModel(model);
    }
  }

  float* run(float *data, int32_t src_size, int32_t &out_size);

public:
  CVI_MODEL_HANDLE model = nullptr;
  CVI_TENSOR *src;
  CVI_TENSOR *output;

private:
  CVI_TENSOR *input_tensors;
  CVI_TENSOR *output_tensors;
  int32_t input_num;
  int32_t output_num;
};

class Vocoder {
public:
  Vocoder(const char *model_file);
  ~Vocoder();
  float *run(float *data, int32_t src_size, int32_t &out_size);

private:
  VocoderModel *vc_100;
  VocoderModel *vc_200;
  VocoderModel *vc_300;
  VocoderModel *vc_400;
  VocoderModel *vc_500;
  VocoderModel *vc_600;
};

#endif