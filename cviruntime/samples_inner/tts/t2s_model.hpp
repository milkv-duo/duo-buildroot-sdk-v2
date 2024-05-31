#ifndef __SAMPLES_TTS_MODEL_HPP
#define __SAMPLES_TTS_MODEL_HPP

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <opencv2/opencv.hpp>
#include "cviruntime.h"
#include "cnpy.h"

namespace t2s {

#define MAX_TEXT_SIZE 200
#define MAX_DECODE_LEN 800
typedef uint16_t bf16_t;

class T2SEncoder {
public:
  T2SEncoder(const char  *cvimodel);
  ~T2SEncoder() {
    if (model)
      CVI_NN_CleanupModel(model);
    if (durations)
      delete[] durations;
  }

  // forward and get total durations and squence.
  int32_t run(uint16_t *text, int32_t text_sz, uint16_t *lang, uint16_t *speaker);

private:
  int32_t regulate_durations(int32_t text_sz);

public:
  CVI_MODEL_HANDLE model = nullptr;
  bf16_t *hiddens;
  int32_t *durations;

private:
  CVI_TENSOR *input_tensors;
  CVI_TENSOR *output_tensors;
  int32_t input_num;
  int32_t output_num;
};

class T2SDecoder {
public:
  T2SDecoder(const char *model_file);
  ~T2SDecoder() {
    if (model) {
      CVI_NN_CleanupModel(model);
    }
  }

  float* run(bf16_t *hidden_states, int32_t duration,  int32_t *durations);

private:
  void expand_hidden_states(CVI_TENSOR *tensor, bf16_t *hidden_states, int32_t *durations);

private:
  CVI_MODEL_HANDLE model = nullptr;
  CVI_TENSOR *input_tensors;
  CVI_TENSOR *output_tensors;
  int32_t input_num;
  int32_t output_num;
};


class T2SModel {
public:
  T2SModel(const char *enc_cvimodel, const char *dec_cvimodel) {
    encoder = new T2SEncoder(enc_cvimodel);
    decoder = new T2SDecoder(dec_cvimodel);
  }

  ~T2SModel() {
    if (encoder)
      delete encoder;
    if (decoder)
      delete decoder;
  }

  float* run(uint16_t *text, int32_t text_sz, uint16_t *lang, uint16_t *speaker, int32_t &duration) {
    duration = encoder->run(text, text_sz, lang, speaker);
    auto mel_out = decoder->run(encoder->hiddens, duration, encoder->durations);
    duration  *= 4;
    return mel_out; // shape is (1x80x800)
  }

private:
  T2SEncoder *encoder = nullptr;
  T2SDecoder *decoder = nullptr;
};

}

#endif