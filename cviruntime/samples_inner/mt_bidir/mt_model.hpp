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

#define SOS_IDX 1
#define LEXICON_SIZE 16002
#define PAD_IDX 0
#define SOS_IDX 1
#define EOS_IDX 2
#define INFER_FIX_LEN 40
typedef uint16_t bf16_t;

class Encoder
{
public:
  Encoder(const char *model_file);
  Encoder(CVI_MODEL_HANDLE model);
  ~Encoder()
  {
    if (model)
    {
      CVI_NN_CleanupModel(model);
    }
  }

  bf16_t *run(int16_t *seq, int32_t size);
  bf16_t *get_mask();

public:
  CVI_MODEL_HANDLE model = nullptr;
  CVI_TENSOR *src_seq;
  CVI_TENSOR *src_mask;
  CVI_TENSOR *enc_output;

private:
  void gen_src_mask(int16_t *src_seq, int32_t size);

  CVI_TENSOR *input_tensors;
  CVI_TENSOR *output_tensors;
  int32_t input_num;
  int32_t output_num;
};

class Decoder
{
public:
  Decoder(CVI_MODEL_HANDLE model, int32_t program_idx, int32_t max_step);
  ~Decoder()
  {
    if (model)
    {
      CVI_NN_CleanupModel(model);
    }
  }

  int16_t run(int16_t *step, int16_t *seq, bf16_t *enc, bf16_t *mask);

public:
  CVI_TENSOR *trg_seq;
  CVI_TENSOR *trg_mask;
  CVI_TENSOR *trg_step;
  CVI_TENSOR *enc_output;
  CVI_TENSOR *src_mask;
  CVI_TENSOR *dec_output;
  int32_t max_step;
  int32_t width;

private:
  void gen_trg_mask();
  int16_t argmax();
  int16_t argmax_int8();
  int16_t argmax_bf16();
  CVI_MODEL_HANDLE model = nullptr;
  CVI_TENSOR *input_tensors;
  CVI_TENSOR *output_tensors;
  int32_t input_num;
  int32_t output_num;
  bool is_fix8b;
};

class MTrans
{
public:
  MTrans(const char *cvimodel)
  {
    encoder = new Encoder(cvimodel);
    decoder_1 = new Decoder(encoder->model, 1, 1);
    decoder_10 = new Decoder(encoder->model, 2, 10);
    decoder_20 = new Decoder(encoder->model, 3, 20);
    decoder_30 = new Decoder(encoder->model, 4, 30);
    decoder_39 = new Decoder(encoder->model, 5, 39);
  }

  ~MTrans()
  {
    delete encoder;
    delete decoder_1;
    delete decoder_10;
    delete decoder_20;
    delete decoder_30;
    delete decoder_39;
  }

  void run(int16_t *seq, int32_t seq_sz,
           int16_t *gen_seq, int32_t gen_seq_sz);

private:
  Encoder *encoder;
  Decoder *decoder_1;
  Decoder *decoder_10;
  Decoder *decoder_20;
  Decoder *decoder_30;
  Decoder *decoder_39;
};

#endif