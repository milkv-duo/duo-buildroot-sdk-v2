/*
* Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
*/
#ifndef LEAKY_RELU_OP_H_
#define LEAKY_RELU_OP_H_

#include "tpuc/CustomOp.h"
#include <cvikernel/cvikernel.h>

namespace cvi {

class LeakyReluOp : public CustomOp {
public:
  LeakyReluOp(OpParam &param) : CustomOp(param) {}

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
  void tdma_load(cvk_context_t *ctx, cvk_tl_t *tlp, uint64_t ga_src);
  void tdma_store(cvk_context_t *ctx, cvk_tl_t *tlp, uint64_t ga_dst);
  void leakyrelu_kernel(cvk_context_t *ctx, int layer_id, cvk_tl_t &bottom,
                        cvk_tl_t &relu, cvk_tl_t &neg, int GT_right_shift_width,
                        int LE_right_shift_width, int GT_scale, int LE_scale);
  void leakyrelu_codegen(cvk_context_t *ctx, uint32_t layer_id, uint64_t input_gaddr,
                         uint64_t output_gaddr, int input_n, int input_c, int input_h,
                         int input_w, int GT_right_shift_width, int LE_right_shift_width,
                         int GT_scale, int LE_scale);
};

} // namespace cvi
#endif
