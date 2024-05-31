/*
* Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
*/
#include "LeakyReluOp.h"
#include "QuantHelper.h"
#include <cvikernel/cvikernel.h>

#define NPU_SHIFT 5
#define EU_SHIFT 4
#define NPU_NUM (1 << NPU_SHIFT)
#define EU_NUM (1 << EU_SHIFT)
#define LOCAL_MEM_SIZE (1 << 15)
#define NEURON_MEMORY 0
#define WEIGHT_MEMORY 1

namespace cvi {

void LeakyReluOp::interpretFp32(
    std::vector<std::shared_ptr<std::vector<float>>> &operand_tensors,
    std::vector<std::vector<int64_t>> &operand_shapes,
    std::shared_ptr<std::vector<float>> &result_tensor,
    std::vector<int64_t> &result_shape) {
  int n = operand_shapes[0][0];
  int c = operand_shapes[0][1];
  int h = operand_shapes[0][2];
  int w = operand_shapes[0][3];
  auto input = operand_tensors[0]->data();
  auto output = result_tensor->data();
  auto negative_slope = param.get<float>("negative_slope");

  for (int i = 0; i < (int)operand_tensors[0]->size(); ++i) {
    if (input[i] >= 0) {
      output[i] = input[i];
    } else {
      output[i] = negative_slope * input[i];
    }
  }
}

void LeakyReluOp::interpretInt8(
    std::vector<std::shared_ptr<std::vector<float>>> &operand_tensors,
    std::vector<std::vector<int64_t>> &operand_shapes,
    std::shared_ptr<std::vector<float>> &result_tensor,
    std::vector<int64_t> &result_shape) {
  int n = operand_shapes[0][0];
  int c = operand_shapes[0][1];
  int h = operand_shapes[0][2];
  int w = operand_shapes[0][3];

  auto input = operand_tensors[0]->data();
  auto quant_pos_rshift =
      param.has("rshift_pos") ? (float)param.get<int8_t>("rshift_pos") : 0.0f;
  auto quant_pos_multiplier =
      param.has("m_i8_pos") ? (float)param.get<int8_t>("m_i8_pos") : 0.0f;
  auto quant_neg_rshift = (float)param.get<int8_t>("rshift_neg");
  auto quant_neg_multiplier = (float)param.get<int8_t>("m_i8_neg");

  auto output = result_tensor->data();

  // rshift and saturate on output
  for (int i = 0; i < (int)operand_tensors[0]->size(); ++i) {
    if (input[i] > 0) {
      if (quant_pos_multiplier != 0.0f) {
        output[i] = (float)applyMultiplierAndRShiftAndSaturateInt8(
            input[i], (uint32_t)quant_pos_rshift, quant_pos_multiplier, false);
      } else {
        output[i] = input[i];
      }
    } else {
      output[i] = (float)applyMultiplierAndRShiftAndSaturateInt8(
          input[i], (uint32_t)quant_neg_rshift, quant_neg_multiplier, false);
    }
  }
}

void LeakyReluOp::quantizeInt8() {
  // support per-tensor only for now
  setOpQuantPerchannel(false);
  // use rshift and INT8 multiplier
  setOpQuantParamType("RSHIFT_AND_M_I8");

  float negative_slope = param.get<float>("negative_slope");
  std::cout << "  negative_slope: " << std::to_string(negative_slope) << "\n";

  // create tensors for rshift and multiplier
  float rshift_pos = 0;
  float multiplier_pos = 0;
  float rshift_neg = 0;
  float multiplier_neg = 0;

  // quantization
  float threshold_x = getPrevOpThreshold();
  float threshold_y = getOpThreshold();
  std::cout << "threshold_y = " << std::to_string(threshold_y)
            << ", threshold_x = " << std::to_string(threshold_x) << "\n";

  // positive
  double qscale_pos = threshold_x / threshold_y;
  if (fabs(threshold_x - threshold_y) < 1e-5 * std::min(threshold_x, threshold_y)) {
    // no positive scale
    rshift_pos = 0;
    multiplier_pos = 0;
    std::cout << "  Positive: no_scale\n";
  } else {
    uint32_t uint_multiplier_pos;
    rshift_pos =
        (float)findRShiftAndMultiplierFromQScale(qscale_pos, &uint_multiplier_pos, false);
    multiplier_pos = (float)uint_multiplier_pos;
    std::cout << "  Positive: ";
    std::cout << "  [multiplier : rshift] = [" << std::to_string(multiplier_pos) << " : "
              << std::to_string(rshift_pos) << "]\n";
  }
  // negative
  float qscale_neg = fabs(qscale_pos * negative_slope);
  uint32_t uint_multiplier_neg = 0;
  rshift_neg =
      (float)findRShiftAndMultiplierFromQScale(qscale_neg, &uint_multiplier_neg, false);
  multiplier_neg = (float)uint_multiplier_neg;
  std::cout << "  Negative: ";
  std::cout << "  [multiplier : rshift] = [" << std::to_string(multiplier_neg) << " : "
            << std::to_string(rshift_neg) << "]\n";

  bool do_pos_scale = (multiplier_pos != 0.0) ? true : false;
  if (do_pos_scale) {
    param.put<int8_t>("rshift_pos", static_cast<int8_t>(rshift_pos));
    param.put<int8_t>("m_i8_pos", static_cast<int8_t>(multiplier_pos));
  }
  param.put<int8_t>("rshift_neg", static_cast<int8_t>(rshift_neg));
  param.put<int8_t>("m_i8_neg", static_cast<int8_t>(multiplier_neg));
}

void LeakyReluOp::codeGenInt8(void *ctx,
                              std::vector<std::vector<int64_t>> &operand_shapes,
                              std::vector<uint64_t> &operand_gaddrs,
                              std::vector<int64_t> &result_shape, uint64_t result_gaddr,
                              int layer_id) {
  auto pos_rshift = param.has("rshift_pos") ? param.get<int8_t>("rshift_pos") : 0;
  auto pos_m_i8 = param.has("m_i8_pos") ? param.get<int8_t>("m_i8_pos") : 0;
  auto neg_rshift = param.has("rshift_neg") ? param.get<int8_t>("rshift_neg") : 0;
  auto neg_m_i8 = param.has("m_i8_neg") ? param.get<int8_t>("m_i8_neg") : 0;
  assert(neg_m_i8);

  int n = operand_shapes[0][0];
  int c = operand_shapes[0][1];
  int h = operand_shapes[0][2];
  int w = operand_shapes[0][3];
  uint64_t operand_gaddr = operand_gaddrs[0];
  uint64_t ga_output = result_gaddr;

  leakyrelu_codegen((cvk_context_t *)ctx,           // ctx
                    layer_id,      // layer_id
                    operand_gaddr, // input_gaddr
                    result_gaddr,  // output_gaddr
                    n,             // input_n
                    c,             // input_c
                    h,             // input_h
                    w,             // input_w
                    pos_rshift,    // GT_right_shift_width
                    neg_rshift,    // LE_right_shift_width
                    pos_m_i8,      // GT_scale
                    neg_m_i8       // LE_scale
  );
}

void LeakyReluOp::tdma_load(cvk_context_t *ctx, cvk_tl_t *tlp, uint64_t ga_src) {
  cvk_tg_t ts_data;
  ts_data.base_reg_index = NEURON_MEMORY;
  ts_data.fmt = tlp->fmt;
  ts_data.start_address = ga_src;
  ts_data.shape = {tlp->shape.n, tlp->shape.c, tlp->shape.h, tlp->shape.w};
  ts_data.stride = ctx->ops->tg_default_stride(ctx, ts_data.shape, ts_data.fmt);

  cvk_tdma_g2l_tensor_copy_param_t p1;
  p1.src = &ts_data;
  p1.dst = tlp;
  ctx->ops->tdma_g2l_tensor_copy(ctx, &p1);
}

void LeakyReluOp::tdma_store(cvk_context_t *ctx, cvk_tl_t *tlp, uint64_t ga_dst) {
  cvk_tg_t ts_data;
  ts_data.base_reg_index = NEURON_MEMORY;
  ts_data.fmt = tlp->fmt;
  ts_data.start_address = ga_dst;
  ts_data.shape = {tlp->shape.n, tlp->shape.c, tlp->shape.h, tlp->shape.w};
  ts_data.stride = ctx->ops->tg_default_stride(ctx, ts_data.shape, ts_data.fmt);

  cvk_tdma_l2g_tensor_copy_param_t p1;
  p1.src = tlp;
  p1.dst = &ts_data;
  ctx->ops->tdma_l2g_tensor_copy(ctx, &p1);
}

void LeakyReluOp::leakyrelu_kernel(cvk_context_t *ctx, int layer_id, cvk_tl_t &bottom,
                                   cvk_tl_t &relu, cvk_tl_t &neg,
                                   int GT_right_shift_width, int LE_right_shift_width,
                                   int GT_scale, int LE_scale) {
  bool isIgnorePosPart = (GT_scale == 0);
  bool isSlopeSmallerThanOne = ((LE_scale >> LE_right_shift_width) == 0);

  if (isIgnorePosPart) {
    cvk_tiu_mul_param_t p4;
    p4.res_high = nullptr;
    p4.res_low = &relu;
    p4.a = &bottom;
    p4.b_const.val = LE_scale;
    p4.b_const.is_signed = true;
    p4.b_is_const = 1;
    p4.rshift_bits = LE_right_shift_width;
    p4.layer_id = layer_id;
    p4.relu_enable = 0;
    ctx->ops->tiu_mul(ctx, &p4);

    if (isSlopeSmallerThanOne) {
      cvk_tiu_max_param_t p1;
      p1.max = &bottom;
      p1.a = &bottom;
      p1.b = &relu;
      p1.b_is_const = 0;
      p1.layer_id = layer_id;
      ctx->ops->tiu_max(ctx, &p1);
    } else {
      cvk_tiu_min_param_t p1;
      p1.min = &bottom;
      p1.a = &bottom;
      p1.b = &relu;
      p1.b_is_const = 0;
      p1.layer_id = layer_id;
      ctx->ops->tiu_min(ctx, &p1);
    }
  } else {
    // 0. relu = relu(bottom)
    cvk_tiu_max_param_t p13;
    p13.max = &relu;
    p13.a = &bottom;
    p13.b_is_const = 1;
    p13.b_const.is_signed = 1;
    p13.b_const.val = 0;
    p13.layer_id = layer_id;
    ctx->ops->tiu_max(ctx, &p13);

    // 1. relu = (relu * GT_scale) >> GT_right_shift_width
    cvk_tiu_mul_param_t p;
    p.res_high = nullptr;
    p.res_low = &relu;
    p.a = &relu;
    p.b_const.val = GT_scale;
    p.b_const.is_signed = true;
    p.b_is_const = 1;
    p.rshift_bits = GT_right_shift_width;
    p.layer_id = layer_id;
    p.relu_enable = 0;
    ctx->ops->tiu_mul(ctx, &p);

    // 2. neg = neg(0, botom)
    cvk_tiu_min_param_t p7;
    p7.min = &neg;
    p7.a = &bottom;
    p7.b_is_const = 1;
    p7.b_const.val = 0;
    p7.b_const.is_signed = 1;
    p7.layer_id = layer_id;
    ctx->ops->tiu_min(ctx, &p7);

    // 3. neg (n,c,h,w) = (neg(n,c,h,w) * slope) >> LE_right_shift_width
    cvk_tiu_mul_param_t p8;
    p8.res_high = nullptr;
    p8.res_low = &neg;
    p8.a = &neg;
    p8.b_const.val = LE_scale;
    p8.b_const.is_signed = true;
    p8.b_is_const = 1;
    p8.rshift_bits = LE_right_shift_width;
    p8.layer_id = layer_id;
    p8.relu_enable = 0;
    ctx->ops->tiu_mul(ctx, &p8);

    // 4. bottom = or relu, neg
    cvk_tiu_or_int8_param_t p9;
    p9.res = &bottom;
    p9.a = &relu;
    p9.b = &neg;
    p9.layer_id = layer_id;
    ctx->ops->tiu_or_int8(ctx, &p9);
  }
}

void LeakyReluOp::leakyrelu_codegen(cvk_context_t *ctx, uint32_t layer_id,
                                    uint64_t input_gaddr, uint64_t output_gaddr,
                                    int input_n, int input_c, int input_h, int input_w,
                                    int GT_right_shift_width, int LE_right_shift_width,
                                    int GT_scale, int LE_scale) {
  printf("leakyrelu_codegen:\n"
         "  layer_id %d\n"
         "  input_gddr: %lx, output_gaddr: %lx\n"
         "  input (%d, %d, %d, %d)\n"
         "  GT_scale:%d, LE_scale:%d\n"
         "  GT_right_shift_width:%d, LE_right_shift_width:%d\n",
         layer_id, input_gaddr, output_gaddr, input_n, input_c, input_h, input_w,
         GT_scale, LE_scale, GT_right_shift_width, LE_right_shift_width);

  // Split input based on local memory
  uint32_t total_eu = NPU_NUM * EU_NUM;
  uint32_t lane_size = LOCAL_MEM_SIZE;
  uint32_t total_mem_size = NPU_NUM * LOCAL_MEM_SIZE;
  uint32_t max_N = (1 << 12) - 1; // 1880v2: 12 bit
  uint32_t max_W = (1 << 12) - 1; // 1880v2: 12 bit
  uint32_t count = input_n * input_c * input_h * input_w;
  uint32_t tiled_N = count / total_eu / 3; // 3 blobs
  tiled_N = (tiled_N > max_N) ? max_N : tiled_N;

  // local tensor shape(tiled_N, npu_num, 1, eu_num)
  cvk_tl_shape_t tl_shape = {tiled_N, static_cast<uint32_t>(NPU_NUM), 1,
                             static_cast<uint32_t>(EU_NUM)};
  cvk_tl_stride_t tl_stride = ctx->ops->tl_default_stride(ctx, tl_shape, CVK_FMT_I8, 1);

  // Find max tiled_N
  uint32_t required_size = 0;
  do {
    tl_shape.n = tiled_N;
    tl_stride = ctx->ops->tl_default_stride(ctx, tl_shape, CVK_FMT_I8, 1);
    required_size = 3 * tl_shape.n * tl_stride.n; // 3 blobs

    if (required_size <= lane_size) {
      break;
    }

  } while (--tiled_N);

  printf("  tiled_bottom shape (%d, %d, %d, %d), stride (%d, %d, %d, %d)\n"
         "  required_size %d kB/lane\n",
         tl_shape.n, tl_shape.c, tl_shape.h, tl_shape.w, tl_stride.n, tl_stride.c,
         tl_stride.h, tl_stride.w, required_size / 1024);

  assert(tiled_N);
  if (!tiled_N) {
    return;
  }

  // Tiled local memory layout:
  //   tiled bottom/result
  //   tiled relu
  //   tiled neg

  // Tiled bottom
  required_size /= 3; // for 3 blobs
  cvk_tl_t tl_tiled_bottom;
  tl_tiled_bottom.start_address = 0;
  tl_tiled_bottom.fmt = CVK_FMT_I8;
  tl_tiled_bottom.shape = tl_shape;
  tl_tiled_bottom.stride = tl_stride;

  // Tiled relu
  cvk_tl_t tl_tiled_relu = tl_tiled_bottom;
  tl_tiled_relu.start_address = tl_tiled_bottom.start_address + required_size;

  // Tiled neg
  cvk_tl_t tl_tiled_neg = tl_tiled_bottom;
  tl_tiled_neg.start_address = tl_tiled_relu.start_address + required_size;

  // In unit of tiled_N * npu_num * eu_num
  uint32_t global_input_offset = 0;
  for (uint32_t i = 0; i < (count / total_eu / tiled_N); i++) {
    // Load as a chunk of contiguous memory in global memory, not use global
    // shape/stride Local memory use tensor shape to maximize eu utilization.
    tdma_load(ctx, &tl_tiled_bottom, input_gaddr + global_input_offset);
    leakyrelu_kernel(ctx, layer_id, tl_tiled_bottom, tl_tiled_relu, tl_tiled_neg,
                     GT_right_shift_width, LE_right_shift_width, GT_scale, LE_scale);
    // Store bottom as a chunk of contiguous memory, not use global shape/stride
    tdma_store(ctx, &tl_tiled_bottom, output_gaddr + global_input_offset);

    // Next input offset
    global_input_offset += tiled_N * total_eu;

  } // for (uint32_t i = 0; i < (count/total_eu/tiled_N); i++)

  // Remaining count, in unit of npu_num * eu_num
  if (global_input_offset < count) {
    uint32_t tiled_W = (count - global_input_offset) / NPU_NUM;
    tiled_N = 1;
    do {
      tl_shape.n = tiled_N;
      tl_shape.w = tiled_W;
      tl_stride = ctx->ops->tl_default_stride(ctx, tl_shape, CVK_FMT_I8, 1);
      required_size = 3 * tl_shape.n * tl_stride.n; // 3 blobs

      if (required_size <= lane_size && (tiled_W <= max_W)) {
        break;
      } else {
        tiled_W /= 2;
        tiled_N *= 2;
      }
    } while (true); // Magic number for 2^12 -1 - 32

    if ((count - global_input_offset) % NPU_NUM != 0) {
      std::cout << "Remaining size should align npu_num, or die";
      assert(0);
    }

    // Update shape, stride
    tl_shape.n = tiled_N;
    tl_shape.w = tiled_W;
    tl_stride = ctx->ops->tl_default_stride(ctx, tl_shape, CVK_FMT_I8, 1);
    required_size = tl_shape.n * tl_stride.n;

    printf("  tiled_bottom shape (%d, %d, %d, %d), stride (%d, %d, %d, %d)\n"
           "  required_size %d kB/lane\n",
           tl_shape.n, tl_shape.c, tl_shape.h, tl_shape.w, tl_stride.n, tl_stride.c,
           tl_stride.h, tl_stride.w, required_size / 1024);

    // Tiled bottom
    tl_tiled_bottom.shape = tl_shape;
    tl_tiled_bottom.stride = tl_stride;

    // Tiled bottom with precise stride
    cvk_tl_t tl_tiled_bottom_precise_stride = tl_tiled_bottom;
    tl_tiled_bottom_precise_stride.stride = {
        static_cast<uint32_t>(tl_shape.h * tl_shape.w * sizeof(uint8_t)),
        static_cast<uint32_t>(tl_shape.h * tl_shape.w * sizeof(uint8_t)),
        static_cast<uint32_t>(tl_shape.w * sizeof(uint8_t)), sizeof(uint8_t)};

    printf("  tiled_bottom_precise shape (%d, %d, %d, %d), stride (%d, %d, %d, %d)\n"
           "  required_size %d kB/lane\n",
           tl_shape.n, tl_shape.c, tl_shape.h, tl_shape.w,
           tl_tiled_bottom_precise_stride.stride.n,
           tl_tiled_bottom_precise_stride.stride.c,
           tl_tiled_bottom_precise_stride.stride.h,
           tl_tiled_bottom_precise_stride.stride.w, required_size / 1024);

    // Tiled relu
    tl_tiled_relu = tl_tiled_bottom;
    tl_tiled_relu.start_address = tl_tiled_bottom.start_address + required_size;

    // Tiled neg
    tl_tiled_neg = tl_tiled_bottom;
    tl_tiled_neg.start_address = tl_tiled_relu.start_address + required_size;

    // Load as a chunk of contiguous memory in global memory, not use global
    // shape/stride Local memory use tensor shape to maximize eu utilization.

    tdma_load(ctx, &tl_tiled_bottom, input_gaddr + global_input_offset);

    leakyrelu_kernel(ctx, layer_id, tl_tiled_bottom, tl_tiled_relu, tl_tiled_neg,
                     GT_right_shift_width, LE_right_shift_width, GT_scale, LE_scale);

    // Store bottom as a chunk of contiguous memory, not use global shape/stride
    tdma_store(ctx, &tl_tiled_bottom, output_gaddr + global_input_offset);

    global_input_offset += tl_tiled_bottom_precise_stride.shape.n *
                           tl_tiled_bottom_precise_stride.stride.n * NPU_NUM;
  }

  // Remaining count, in unit of eu_num
  if (global_input_offset != count) {
    printf("global_input_offset != count (%d != %d)/n", global_input_offset, count);
    assert(0);
  }
}

RegisterCustomOp(leaky_relu, LeakyReluOp);

} // namespace cvi