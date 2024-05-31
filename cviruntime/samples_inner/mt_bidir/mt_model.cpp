#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <opencv2/opencv.hpp>
#include "cviruntime.h"
#include "cnpy.h"
#include "mt_model.hpp"

static bf16_t mask_val() {
  float val = -50;
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  return ((bf16_t *)(&val))[0];
#else
  return ((bf16_t *)(&val))[1];
#endif
}

static inline float BF16(const bf16_t & data) {
  float data_f32 = 0.0f;
  uint16_t *p_data_bf16 = (uint16_t*)(&data_f32);
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  p_data_bf16[0] = data;
#else
  p_data_bf16[1] = data;
#endif
  return data_f32;
}

Encoder::Encoder(const char *model_file) {
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
  assert(input_num == 2);
  assert(output_num == 1);
  src_seq = &input_tensors[0];
  src_mask = &input_tensors[1];
  enc_output = &output_tensors[0];
  for (int i = 0; i < input_num; i++) {
    std::cout << "input => " << input_tensors[i].name << "\n";
  }
}

Encoder::Encoder(CVI_MODEL_HANDLE main_model) {
  int ret = CVI_NN_CloneModel(main_model, &model);
  if (ret != CVI_RC_SUCCESS) {
    printf("CVI_NN_RegisterModel failed, err %d\n", ret);
    exit(1);
  }
  CVI_NN_SetConfig(model, OPTION_PROGRAM_INDEX, 1);
  ret = CVI_NN_GetInputOutputTensors(model, &input_tensors, &input_num,
                                     &output_tensors, &output_num);
  if (ret != CVI_RC_SUCCESS) {
    printf("CVI_NN_GetInputOutputTensors failed, err %d\n", ret);
    exit(1);
  }
  assert(input_num == 2);
  assert(output_num == 1);
  src_seq = &input_tensors[0];
  src_mask = &input_tensors[1];
  enc_output = &output_tensors[0];
  for (int i = 0; i < input_num; i++) {
    std::cout << "input => " << input_tensors[i].name << "\n";
  }
}

void Encoder::gen_src_mask(int16_t *seq, int32_t size) {
  auto ptr = (bf16_t *)CVI_NN_TensorPtr(src_mask);
  assert(CVI_NN_TensorCount(src_mask) == size);
  auto filled_val = mask_val();
  for (int i = 0; i < size; i++) {
    ptr[i] = (seq[i] == 0) ? filled_val : 0;
  }
}

bf16_t *Encoder::get_mask() {
  return (bf16_t *)CVI_NN_TensorPtr(src_mask);
}

bf16_t* Encoder::run(int16_t *seq, int32_t size) {
  // fill src_seq to tensor 0
  CVI_NN_SetTensorPtr(src_seq, seq);
  // generate src mask to tensor 1
  gen_src_mask(seq, size);

  // run inference
  CVI_NN_Forward(model, input_tensors, input_num,
                 output_tensors, output_num);

  return (bf16_t *)CVI_NN_TensorPtr(enc_output);
}

Decoder::Decoder(CVI_MODEL_HANDLE main_model, int32_t program_index, int32_t max_step)
  : max_step(max_step) {
  int ret = CVI_NN_CloneModel(main_model, &model);
  if (ret != CVI_RC_SUCCESS) {
    printf("CVI_NN_RegisterModel failed, err %d\n", ret);
    exit(1);
  }
  CVI_NN_SetConfig(model, OPTION_PROGRAM_INDEX, program_index);
  ret = CVI_NN_GetInputOutputTensors(model, &input_tensors, &input_num,
                                     &output_tensors, &output_num);
  if (ret != CVI_RC_SUCCESS) {
    printf("CVI_NN_GetInputOutputTensors failed, err %d\n", ret);
    exit(1);
  }
  assert(input_num == 5);
  assert(output_num == 1);
  trg_seq = &input_tensors[0];
  enc_output = &input_tensors[1];
  src_mask = &input_tensors[2];
  trg_mask = &input_tensors[3];
  trg_step = &input_tensors[4];
  dec_output = &output_tensors[0];
  width = dec_output->shape.dim[1];
  for (int i = 0; i < input_num; i++) {
    std::cout << "input => " << input_tensors[i].name << "\n";
  }
  std::cout << max_step << "- Decoder: tensors: "
            << trg_seq->name << ", "
            << trg_mask->name << ", "
            << enc_output->name << ", "
            << src_mask->name << ", "
            << trg_step->name << ", "
            << dec_output->name << ", width:"
            << width << "\n";
  // generate default trg mask
  gen_trg_mask();
  is_fix8b = (dec_output->fmt != CVI_FMT_BF16);
}

void Decoder::gen_trg_mask() {
  auto filled_val = mask_val();
  auto ptr = (bf16_t *)CVI_NN_TensorPtr(trg_mask);
  for (int i = 0; i < max_step; i++) {
    for (int j = 0; j < max_step; j++) {
      ptr[i * max_step + j] = (j > i) ? filled_val : 0;
    }
  }
}

int16_t Decoder::argmax_int8() {
  auto ptr = (int8_t *)CVI_NN_TensorPtr(dec_output);
  int idx = 0;
  auto max_value = ptr[0];
  for (int j = 1; j < width; j++) {
    auto val = ptr[j];
    if (val > max_value) {
      idx = j;
      max_value = val;
    }
  }
  return idx;
}

int16_t Decoder::argmax_bf16() {
  auto ptr = (bf16_t *)CVI_NN_TensorPtr(dec_output);
  int idx = 0;
  auto max_value = BF16(ptr[0]);
  for (int j = 1; j < width; j++) {
    auto val = BF16(ptr[j]);
    if (val > max_value) {
      idx = j;
      max_value = val;
    }
  }
  return idx;
}

int16_t Decoder::argmax() {
  if (is_fix8b) {
    return argmax_int8();
  } else {
    return argmax_bf16();
  }
}

int16_t Decoder::run(int16_t * step, int16_t *seq, bf16_t *enc, bf16_t *mask) {
  // fill data to input tensor
  CVI_NN_SetTensorPtr(trg_seq, seq);
  CVI_NN_SetTensorPtr(enc_output, enc);
  CVI_NN_SetTensorPtr(src_mask, mask);
  CVI_NN_SetTensorPtr(trg_step, step);
  // run inference
  CVI_NN_Forward(model, input_tensors, input_num,
                 output_tensors, output_num);
  return argmax();
}

void MTrans::run(int16_t *seq, int32_t seq_sz, int16_t *gen_seq, int32_t gen_seq_sz) {
  // clean gen_seq array.
  memset(gen_seq, 0, gen_seq_sz * sizeof(int16_t));
  auto enc_output = encoder->run(seq, seq_sz);
  auto src_mask = encoder->get_mask();

  uint32_t best_idx = SOS_IDX;
  gen_seq[0] = best_idx;

  for (int16_t step = 1; step < seq_sz; step++) {
    int16_t idx = step - 1;
    if (step <= 1) {
      best_idx = decoder_1->run(&idx, gen_seq, enc_output, src_mask);
    } else if (step <= 10) {
      best_idx = decoder_10->run(&idx, gen_seq, enc_output, src_mask);
    } else if (step <= 20) {
      best_idx = decoder_20->run(&idx, gen_seq, enc_output, src_mask);
    } else if (step <= 30) {
      best_idx = decoder_30->run(&idx, gen_seq, enc_output, src_mask);
    } else {
      best_idx = decoder_39->run(&idx, gen_seq, enc_output, src_mask);
    }
    gen_seq[step] = best_idx;
    if (best_idx == EOS_IDX) {
      break;
    }
  }
}
