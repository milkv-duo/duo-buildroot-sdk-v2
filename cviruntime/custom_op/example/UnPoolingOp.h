/*
* Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
*/
#ifndef UNPOOLING_OP_H_
#define UNPOOLING_OP_H_

#include "tpuc/CustomOp.h"
#include <cvikernel/cvikernel.h>

namespace cvi {

class UnPoolingOp : public CustomOp {
public:
  UnPoolingOp(OpParam &param) : CustomOp(param) {}

  void interpretFp32(std::vector<std::shared_ptr<std::vector<float>>> &operand_tensors,
                     std::vector<std::vector<int64_t>> &operand_shapes,
                     std::shared_ptr<std::vector<float>> &result_tensor,
                     std::vector<int64_t> &result_shape);
  void interpretInt8(std::vector<std::shared_ptr<std::vector<float>>> &operand_tensors,
                     std::vector<std::vector<int64_t>> &operand_shapes,
                     std::shared_ptr<std::vector<float>> &result_tensor,
                     std::vector<int64_t> &result_shape);
  void quantizeInt8();
  void codeGenInt8(void *ctx,
                   std::vector<std::vector<int64_t>> &operand_shapes,
                   std::vector<uint64_t> &operand_gaddrs,
                   std::vector<int64_t> &result_shape, uint64_t result_gaddr,
                   int layer_id);

private:
  void alloc_lmem(cvk_context_t *ctx, uint32_t tiling_c, uint32_t tiling_h,
    uint32_t input_c, uint32_t input_h, uint32_t input_w,
    uint32_t output_c, uint32_t output_h, uint32_t output_w,
    cvk_fmt_t fmt, int eu_align, cvk_tl_t &tl_ifmap, cvk_tl_t &tl_working,
    cvk_tl_t &tl_ofmap, cvk_tl_t &tl_mask);
  void tdma_load(cvk_context_t *ctx, cvk_tl_t *tlp, uint64_t ga_src, cvk_tg_stride_t stride,
                 int n_pos, int c_pos, int h_pos);
  void unpooling_compute(cvk_context_t *ctx, uint32_t layer_id, int scale_h, int scale_w,
    cvk_tl_t *tl_ifmap, cvk_tl_t *tl_working, cvk_tl_t *tl_mask, cvk_tl_t *tl_ofmap);
  void tdma_store(cvk_context_t *ctx, cvk_tl_t *tlp, uint64_t ga_dst, cvk_tg_stride_t stride,
    uint32_t n_pos, uint32_t c_pos, uint32_t h_pos, uint32_t crop_h, uint32_t crop_w);
  void unpooling_codegen(cvk_context_t *ctx, uint32_t layer_id,
                        uint64_t data_gaddr, uint64_t mask_gaddr, uint64_t output_gaddr,
                        int input_n, int input_c, int input_h, int input_w,
                        int scale, int unpool_h, int unpool_w);

  void unpooling(std::vector<std::shared_ptr<std::vector<float>>> &operand_tensors,
      std::vector<std::vector<int64_t>> &operand_shapes,
      std::shared_ptr<std::vector<float>> &result_tensor,
      std::vector<int64_t> &result_shape) {
    int in = operand_shapes[0][0];
    int ic = operand_shapes[0][1];
    int ih = operand_shapes[0][2];
    int iw = operand_shapes[0][3];

    int oh = result_shape[2];
    int ow = result_shape[3];

    float *data = operand_tensors[0]->data();
    float *mask = operand_tensors[1]->data();
    float *output = result_tensor->data();
    auto scale = param.get<int>("scale");
    auto unpool_h = param.get<int>("unpool_h");
    auto unpool_w = param.get<int>("unpool_w");

    assert(oh == unpool_h);
    assert(ow == unpool_w);

    int sh = ih * scale;
    int sw = iw * scale;
    // always use float to store int8 value
    std::vector<float> tmp_out(in * ic * sh * sw);

    for (int n = 0; n < in; n++) {
      for (int c = 0; c < ic; c++) {
        for (int h = 0; h < sh; h++) {
          for (int w = 0; w < sw; w++) {
            int isw = w / scale;
            int ish = h / scale;
            int out_idx = ((n * ic + c) * sh + h) * sw + w;
            int in_idx = ((n * ic + c) * ih + ish) * iw + isw;
            tmp_out[out_idx] = data[in_idx] * mask[out_idx];
          }
        }
      }
    }

    for (int n = 0; n < in; n++) {
      for (int c = 0; c < ic; c++) {
        for (int h = 0; h < oh; h++) {
          for (int w = 0; w < ow; w++) {
            int out_idx = ((n * ic + c) * oh + h) * ow + w;
            int in_idx = ((n * ic + c) * sh + h) * sw + w;
            output[out_idx] = tmp_out[in_idx];
          }
        }
      }
    }
  }
};

} // namespace cvi
#endif
