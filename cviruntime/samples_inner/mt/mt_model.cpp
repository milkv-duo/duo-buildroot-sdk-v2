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

/*
static void store_result(std::string name, CVI_TENSOR *tensor) {
  std::vector<size_t> shape = {
      (size_t)tensor->shape.dim[0], (size_t)tensor->shape.dim[1],
      (size_t)tensor->shape.dim[2], (size_t)tensor->shape.dim[3]};
  cnpy::npz_t npz;
  cnpy::npz_add_array<uint16_t>(npz, tensor->name,
       (uint16_t *)CVI_NN_TensorPtr(tensor), shape);
  cnpy::npz_save_all(name, npz);
}
*/

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

  /*
  printf("src_seq:");
  for (int i = 0; i < (int)CVI_NN_TensorCount(src_seq); i++) {
    printf("%d ", ((int16_t *)CVI_NN_TensorPtr(src_seq))[i]);
  }
  printf("\n");
  printf("src_mask:");
  for (int i = 0; i < (int)CVI_NN_TensorCount(src_mask); i++) {
    printf("%d ", ((int16_t *)CVI_NN_TensorPtr(src_mask))[i]);
  }
  printf("\n");
  */
  // run inference
  CVI_NN_Forward(model, input_tensors, input_num,
                 output_tensors, output_num);
  //store_result("xx_enc_output.npz", enc_output);

  return (bf16_t *)CVI_NN_TensorPtr(enc_output);
}

Decoder::Decoder(CVI_MODEL_HANDLE main_model, int32_t max_step)
  : max_step(max_step) {
  int ret = CVI_NN_CloneModel(main_model, &model);
  if (ret != CVI_RC_SUCCESS) {
    printf("CVI_NN_RegisterModel failed, err %d\n", ret);
    exit(1);
  }
  CVI_NN_SetConfig(model, OPTION_BATCH_SIZE, 1);
  switch(max_step) {
    case 0:
      CVI_NN_SetConfig(model, OPTION_PROGRAM_INDEX, 1);
      break;
    case 10:
      CVI_NN_SetConfig(model, OPTION_PROGRAM_INDEX, 2);
      break;
    case 20:
      CVI_NN_SetConfig(model, OPTION_PROGRAM_INDEX, 3);
      break;
    case 30:
      CVI_NN_SetConfig(model, OPTION_PROGRAM_INDEX, 4);
      break;
    case 39:
      CVI_NN_SetConfig(model, OPTION_PROGRAM_INDEX, 5);
      break;
  }
  ret = CVI_NN_GetInputOutputTensors(model, &input_tensors, &input_num,
                                     &output_tensors, &output_num);
  if (ret != CVI_RC_SUCCESS) {
    printf("CVI_NN_GetInputOutputTensors failed, err %d\n", ret);
    exit(1);
  }
  assert(input_num == 4);
  assert(output_num == 1);
  trg_seq = &input_tensors[0];
  enc_output = &input_tensors[1];
  src_mask = &input_tensors[2];
  trg_mask = &input_tensors[3];
  dec_output = &output_tensors[0];
  width = dec_output->shape.dim[2];
  for (int i = 0; i < input_num; i++) {
    std::cout << "input => " << input_tensors[i].name << "\n";
  }
  std::cout << max_step << "- Decoder: tensors: "
            << trg_seq->name << ", "
            << trg_mask->name << ", "
            << enc_output->name << ", "
            << src_mask->name << ", "
            << dec_output->name << ", width:"
            << width << "\n";
  // generate default trg mask
  gen_trg_mask();
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

int16_t Decoder::argmax(int step) {
  step = (step == 0) ? 0 : (step - 1);
  auto ptr = (int8_t *)CVI_NN_TensorPtr(dec_output);
  ptr += step * width;
  int idx = 0;
  int8_t max_value = 0;
  for (int j = 0; j < width; j++) {
    int8_t val = ptr[j];
    if (val < 0) {
      continue;
    }
    if (val > max_value) {
      idx = j;
      max_value = val;
    }
  }
  return idx;
}

int16_t Decoder::run(int step, int16_t *seq, bf16_t *enc, bf16_t *mask) {
  // fill data to input tensor
  CVI_NN_SetTensorPtr(trg_seq, seq);
  CVI_NN_SetTensorPtr(enc_output, enc);
  CVI_NN_SetTensorPtr(src_mask, mask);
  // run inference
  CVI_NN_Forward(model, input_tensors, input_num,
                 output_tensors, output_num);
  // std::string name = "xx_decode_" + std::to_string(step) + "_out.npz";
  // store_result(name, dec_output);
  return argmax(step);
}

void MTrans::run(int16_t *seq, int32_t seq_sz, int16_t *gen_seq, int32_t gen_seq_sz) {
  // clean gen_seq array.
  memset(gen_seq, 0, gen_seq_sz * sizeof(int16_t));

  auto enc_output = encoder->run(seq, seq_sz);
  auto src_mask = encoder->get_mask();

  int16_t trg_seq = 1;
  auto best_idx = decoder_0->run(0, &trg_seq, enc_output, src_mask);
  gen_seq[0] = SOS_IDX;
  gen_seq[1] = best_idx;

  int seq_len = 0;
  for (int step = 2; step < INFER_FIX_LEN; step++) {
    if (step <= 10) {
      best_idx = decoder_10->run(step, gen_seq, enc_output, src_mask);
    } else if (step <= 20) {
      best_idx = decoder_20->run(step, gen_seq, enc_output, src_mask);
    } else if (step <= 30) {
      best_idx = decoder_30->run(step, gen_seq, enc_output, src_mask);
    } else {
      best_idx = decoder_39->run(step, gen_seq, enc_output, src_mask);
    }
    gen_seq[step] = best_idx;
    seq_len = step + 1;
    // if (gen_seq[39] == EOS_IDX) {
    if (best_idx == EOS_IDX) {
      break;
    }
  }
}
