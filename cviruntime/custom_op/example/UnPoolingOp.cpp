/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 */
#include "UnPoolingOp.h"
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

void UnPoolingOp::interpretFp32(
    std::vector<std::shared_ptr<std::vector<float>>> &operand_tensors,
    std::vector<std::vector<int64_t>> &operand_shapes,
    std::shared_ptr<std::vector<float>> &result_tensor,
    std::vector<int64_t> &result_shape) {
  unpooling(operand_tensors, operand_shapes, result_tensor, result_shape);
}

void UnPoolingOp::interpretInt8(
    std::vector<std::shared_ptr<std::vector<float>>> &operand_tensors,
    std::vector<std::vector<int64_t>> &operand_shapes,
    std::shared_ptr<std::vector<float>> &result_tensor,
    std::vector<int64_t> &result_shape) {
  unpooling(operand_tensors, operand_shapes, result_tensor, result_shape);
}

void UnPoolingOp::quantizeInt8() {
  // support per-tensor only for now
  setOpQuantPerchannel(false);
  // use rshift and INT8 multiplier
  setOpQuantParamType("RSHIFT_AND_M_I8");

  // quantization
  float threshold_x = getPrevOpThreshold();
  float threshold_y = getOpThreshold();
  std::cout << "threshold_y = " << std::to_string(threshold_y)
            << ", threshold_x = " << std::to_string(threshold_x) << "\n";
}

void UnPoolingOp::codeGenInt8(void *ctx,
                              std::vector<std::vector<int64_t>> &operand_shapes,
                              std::vector<uint64_t> &operand_gaddrs,
                              std::vector<int64_t> &result_shape,
                              uint64_t result_gaddr, int layer_id) {
  int n = operand_shapes[0][0];
  int c = operand_shapes[0][1];
  int h = operand_shapes[0][2];
  int w = operand_shapes[0][3];
  uint64_t data_gaddr = operand_gaddrs[0];
  uint64_t mask_gaddr = operand_gaddrs[1];
  uint64_t ga_output = result_gaddr;

  int scale = param.get<int>("scale");
  int unpool_h = param.get<int>("unpool_h");
  int unpool_w = param.get<int>("unpool_w");

  unpooling_codegen((cvk_context_t *)ctx, // ctx
                    layer_id,             // layer_id
                    data_gaddr,           // data_gaddr
                    mask_gaddr,           // mask_gaddr
                    ga_output,            // output_gaddr
                    n, c, h, w,              // input shape
                    scale, unpool_h, unpool_w);
}

void UnPoolingOp::alloc_lmem(cvk_context_t *ctx, uint32_t tiling_c, uint32_t tiling_h,
    uint32_t input_c, uint32_t input_h, uint32_t input_w,
    uint32_t output_c, uint32_t output_h, uint32_t output_w,
    cvk_fmt_t fmt, int eu_align, cvk_tl_t &tl_ifmap, cvk_tl_t &tl_working,
    cvk_tl_t &tl_mask, cvk_tl_t &tl_ofmap) {
  uint32_t tl_offset = 0;
  ctx->ops->lmem_init_tensor(ctx, &tl_ifmap, {1, tiling_c, tiling_h, input_w}, fmt,
                       eu_align);
  tl_ifmap.start_address = tl_offset;
  tl_offset += ctx->ops->lmem_tensor_to_size(ctx, tl_ifmap.shape, tl_ifmap.fmt,
                                       tl_ifmap.eu_align);

  ctx->ops->lmem_init_tensor(ctx, &tl_working, {1, tiling_c, tiling_h, output_w}, fmt,
                       eu_align);
  tl_working.start_address = tl_offset;
  tl_offset += ctx->ops->lmem_tensor_to_size(ctx, tl_working.shape, tl_working.fmt,
                                       tl_working.eu_align);

  uint32_t tiling_oh = tiling_h * (output_h / input_h);
  ctx->ops->lmem_init_tensor(ctx, &tl_mask, {1, tiling_c, tiling_oh, output_w}, fmt,
                       eu_align);
  tl_mask.start_address = tl_offset;
  tl_offset += ctx->ops->lmem_tensor_to_size(ctx, tl_mask.shape, tl_mask.fmt,
                                        tl_mask.eu_align);

  ctx->ops->lmem_init_tensor(ctx, &tl_ofmap, {1, tiling_c, tiling_oh, output_w}, fmt,
                       eu_align);
  tl_ofmap.start_address = tl_offset;
}

void UnPoolingOp::tdma_load(cvk_context_t *ctx, cvk_tl_t *tlp, uint64_t ga_src,
                            cvk_tg_stride_t stride, int32_t n_pos, int32_t c_pos, int32_t h_pos) {
  cvk_tg_t ts_data;
  ts_data.base_reg_index = NEURON_MEMORY;
  ts_data.fmt = tlp->fmt;
  ts_data.start_address = ga_src + stride.n * n_pos + stride.c * c_pos + stride.h * h_pos;
  ts_data.shape = {tlp->shape.n, tlp->shape.c, tlp->shape.h, tlp->shape.w};
  ts_data.stride = stride;

  cvk_tdma_g2l_tensor_copy_param_t p1;
  p1.src = &ts_data;
  p1.dst = tlp;
  ctx->ops->tdma_g2l_tensor_copy(ctx, &p1);
}

void UnPoolingOp::unpooling_compute(
    cvk_context_t *ctx, uint32_t layer_id, int scale_h, int scale_w, 
    cvk_tl_t *tl_ifmap, cvk_tl_t *tl_working, cvk_tl_t *tl_mask, cvk_tl_t *tl_ofmap) {

  cvk_tl_stride_t tl_ifmap_fake_stride = {0, tl_ifmap->stride.c, tl_ifmap->stride.h, tl_ifmap->stride.w};
  cvk_tl_t tl_ifmap_fake = {0};
  tl_ifmap_fake.start_address = tl_ifmap->start_address;
  tl_ifmap_fake.fmt = tl_ifmap->fmt;
  tl_ifmap_fake.shape = {scale_w, tl_ifmap->shape.c, tl_ifmap->shape.h, tl_ifmap->shape.w};
  tl_ifmap_fake.stride = tl_ifmap_fake_stride;
  tl_ifmap_fake.eu_align = tl_ifmap->eu_align;

  cvk_tl_stride_t tl_working_fake_stride = {
     tl_working->stride.w, tl_working->stride.c,
     tl_working->stride.h, tl_working->stride.w * scale_w};
  cvk_tl_t tl_working_fake = {0};
  tl_working_fake.start_address = tl_working->start_address;
  tl_working_fake.fmt = tl_working->fmt;
  tl_working_fake.shape = {scale_w, tl_ifmap->shape.c, tl_ifmap->shape.h, tl_ifmap->shape.w};
  tl_working_fake.stride = tl_working_fake_stride;
  tl_working_fake.eu_align = tl_working->eu_align;

  cvk_tiu_copy_param_t param = {0};
  param.dst = &tl_working_fake;
  param.src =  &tl_ifmap_fake;
  param.layer_id = layer_id;
  ctx->ops->tiu_copy(ctx, &param);

  cvk_tl_stride_t tl_working_fake2_stride = {0, tl_working->stride.c, tl_working->stride.h, tl_working->stride.w};
  cvk_tl_t tl_working_fake2 = {0};
  tl_working_fake2.start_address = tl_working->start_address;
  tl_working_fake2.fmt = tl_working->fmt;
  tl_working_fake2.shape = {scale_h, tl_ofmap->shape.c, tl_ifmap->shape.h, tl_ofmap->shape.w};
  tl_working_fake2.stride = tl_working_fake2_stride;
  tl_working_fake2.eu_align = tl_working->eu_align;

  cvk_tl_stride_t tl_ofmap_fake_stride = {tl_ofmap->stride.h, tl_ofmap->stride.c, tl_ofmap->stride.h * scale_h, tl_ofmap->stride.w};
  cvk_tl_t tl_ofmap_fake = {0};
  tl_ofmap_fake.start_address = tl_ofmap->start_address;
  tl_ofmap_fake.fmt = tl_ofmap->fmt;
  tl_ofmap_fake.shape =  {scale_h, tl_ofmap->shape.c, tl_ifmap->shape.h, tl_ofmap->shape.w};
  tl_ofmap_fake.stride = tl_ofmap_fake_stride;
  tl_ofmap_fake.eu_align = tl_ofmap->eu_align;

  cvk_tiu_copy_param_t param2 = {0};
  param2.dst = &tl_ofmap_fake;
  param2.src =  &tl_working_fake2;
  param2.layer_id = layer_id;
  ctx->ops->tiu_copy(ctx, &param2);

  cvk_tiu_mul_param_t param3 = {0};
  param3.res_high = nullptr;
  param3.res_low = tl_ofmap;
  param3.a = tl_ofmap;
  param3.b_is_const = 0;
  param3.b = tl_mask;
  param3.layer_id = layer_id;
  param3.rshift_bits = 0;
  param3.relu_enable = 0;
  ctx->ops->tiu_mul(ctx, &param3);
}

void UnPoolingOp::tdma_store(cvk_context_t *ctx, cvk_tl_t *tlp,
                             uint64_t ga_dst, cvk_tg_stride_t stride,
                             uint32_t n_pos, uint32_t c_pos, uint32_t h_pos,
                             uint32_t crop_h, uint32_t crop_w) {
  cvk_tl_t tl_src;
  tl_src.start_address = tlp->start_address;
  tl_src.fmt = tlp->fmt;
  tl_src.shape = {tlp->shape.n, tlp->shape.c, tlp->shape.h - crop_h, tlp->shape.w - crop_w};
  tl_src.stride = tlp->stride;

  cvk_tg_t tg_dst;
  tg_dst.base_reg_index = NEURON_MEMORY;
  tg_dst.fmt = tlp->fmt;
  tg_dst.start_address = ga_dst + stride.n * n_pos + stride.c * c_pos + stride.h * h_pos;
  tg_dst.shape = {tlp->shape.n, tlp->shape.c, tlp->shape.h - crop_h, tlp->shape.w - crop_w};
  tg_dst.stride = stride;

  cvk_tdma_l2g_tensor_copy_param_t p1;
  p1.src = &tl_src;
  p1.dst = &tg_dst;
  ctx->ops->tdma_l2g_tensor_copy(ctx, &p1);
}

void UnPoolingOp::unpooling_codegen(cvk_context_t *ctx, uint32_t layer_id,
                                    uint64_t data_gaddr, uint64_t mask_gaddr, uint64_t output_gaddr,
                                    int input_n, int input_c, int input_h, int input_w,
                                    int scale, int unpool_h, int unpool_w) {
  printf("unpooling_codegen:\n"
         "  layer_id %d\n"
         "  data_gddr: %lx, mask_gaddr: %lx, output_gaddr: %lx\n"
         "  input (%d, %d, %d, %d)\n"
         "  scale:%d, unpool_h:%d, unpool_w:%d\n",
         layer_id, data_gaddr, mask_gaddr, output_gaddr, input_n, input_c, input_h,
         input_w, scale, unpool_h, unpool_w);

  // Split input based on local memory
  uint32_t total_eu = NPU_NUM * EU_NUM;
  uint32_t lane_size = LOCAL_MEM_SIZE;
  uint32_t total_mem_size = NPU_NUM * LOCAL_MEM_SIZE;
  uint32_t max_N = (1 << 12) - 1; // 1880v2: 12 bit
  uint32_t max_W = (1 << 12) - 1; // 1880v2: 12 bit
  uint32_t count = input_n * input_c * input_h * input_w;

  uint32_t output_c = input_c;
  uint32_t output_h = input_h * scale;
  uint32_t output_w = input_w * scale;

  uint32_t n_step = 1;
  uint32_t c_step = 0;
  uint32_t h_step = 0;

  h_step = input_h;
  uint32_t h_factor = scale;

  for (; h_step > 0; --h_step) {
    uint32_t total_size;
    for (c_step = input_c; c_step >= (uint32_t)NPU_NUM ; --c_step) {
      cvk_tl_shape_t tiled_ifmap_shape = {1, c_step, h_step, input_w};
      uint32_t tiled_ifmap_size =
          ctx->ops->lmem_tensor_to_size(ctx, tiled_ifmap_shape, CVK_FMT_I8, 0);

      cvk_tl_shape_t tiled_working_shape = {1, c_step, h_step, output_w};
      uint32_t tiled_working_size =
          ctx->ops->lmem_tensor_to_size(ctx, tiled_working_shape, CVK_FMT_I8, 0);

      cvk_tl_shape_t tiled_ofmap_shape = {1, c_step, h_step * h_factor, output_w};
      uint32_t tiled_ofmap_size =
          ctx->ops->lmem_tensor_to_size(ctx, tiled_ofmap_shape, CVK_FMT_I8, 0);

      total_size = tiled_ifmap_size + tiled_working_size + tiled_ofmap_size * 2;
      if (total_size <= static_cast<uint32_t>(LOCAL_MEM_SIZE))
        break;
    }
    if (total_size <= static_cast<uint32_t>(LOCAL_MEM_SIZE))
      break;
  }

  printf("tiling: c_step %d, h_step %d\n", c_step, h_step);
  assert(c_step && h_step && "Expect valid tiling");

  cvk_tg_stride_t ifmap_stride = {
    input_c * input_h * input_w,
    input_h * input_w,
    input_w};
  cvk_tg_stride_t mask_stride = {
    output_c * output_h * output_w,
    output_h * output_w,
    output_w};
  cvk_tg_stride_t output_stride = {
    output_c * unpool_h * unpool_w,
    unpool_h * unpool_w,
    unpool_w};

  uint64_t output_offset = 0;
  uint32_t crop_h = 0;
  uint32_t crop_w = 0;
  for (uint32_t n_pos = 0; n_pos < input_n; n_pos += n_step) {
    for (uint32_t c_pos = 0; c_pos < input_c; c_pos += c_step) {
      uint32_t tiling_c = std::min(input_c - c_pos, c_step);
      for (uint32_t h_pos = 0; h_pos < input_h; h_pos += h_step) {
        uint32_t tiling_h = std::min(input_h - h_pos, h_step);

        cvk_tl_t tl_ifmap, tl_ofmap, tl_mask, tl_working;
        alloc_lmem(ctx, tiling_c, tiling_h, input_c, input_h, input_w, output_c,
                                    output_h, output_w, CVK_FMT_I8, 0, tl_ifmap, tl_working,
                                    tl_mask, tl_ofmap);

        tdma_load(ctx, &tl_ifmap, data_gaddr, ifmap_stride, n_pos, c_pos, h_pos);
        tdma_load(ctx, &tl_mask, mask_gaddr, mask_stride, n_pos, c_pos, h_pos * scale);

        unpooling_compute(ctx, layer_id, scale, scale, &tl_ifmap, &tl_working, &tl_mask, &tl_ofmap);

        uint32_t oh_pos = h_pos * scale;
        crop_w = output_w - unpool_w;
        if (oh_pos + tiling_h * scale > unpool_h) {
          crop_h = oh_pos + tiling_h * scale - unpool_h;
        } else {
          crop_h = 0;
        }
        tdma_store(ctx, &tl_ofmap, output_gaddr, output_stride, n_pos, c_pos, h_pos * scale, crop_h, crop_w);
      }
    }
  }
}

RegisterCustomOp(unpooling, UnPoolingOp);

} // namespace cvi